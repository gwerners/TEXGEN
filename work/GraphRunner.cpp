#include "GraphRunner.h"

#include "CoreNodeRegistry.h"
#include "GraphEval.h"

#include <cstdio>

GraphRunner::GraphRunner() {
  m_thread = std::thread(&GraphRunner::loop, this);
}

GraphRunner::~GraphRunner() {
  {
    std::lock_guard<std::mutex> lk(m_mx);
    m_quit = true;
  }
  m_cv.notify_all();
  if (m_thread.joinable())
    m_thread.join();
}

void GraphRunner::request(nlohmann::json project) {
  {
    std::lock_guard<std::mutex> lk(m_mx);
    m_pending = Job{};
    m_pending.project = std::move(project);
    m_hasPending = true;
    m_busy = true;
  }
  m_cv.notify_all();
}

void GraphRunner::requestNode(nlohmann::json project,
                              int changedId,
                              bool propagate) {
  {
    std::lock_guard<std::mutex> lk(m_mx);
    if (!m_hasPending) {
      m_pending = Job{};
      m_pending.full = false;
    }
    m_pending.project = std::move(project);
    if (!m_pending.full) {
      m_pending.dirty.insert(changedId);
      m_pending.propagate = propagate;
    }
    m_hasPending = true;
    m_busy = true;
  }
  m_cv.notify_all();
}

bool GraphRunner::poll(Result& out) {
  std::lock_guard<std::mutex> lk(m_mx);
  if (!m_hasDone)
    return false;
  out = std::move(m_done);
  m_done = Result{};
  m_hasDone = false;
  return true;
}

void GraphRunner::pollStream(std::vector<NodeResult>& out) {
  std::lock_guard<std::mutex> lk(m_mx);
  while (!m_stream.empty()) {
    out.push_back(std::move(m_stream.front()));
    m_stream.pop_front();
  }
}

// Node ids/types + connections; params excluded, so a param-only edit
// keeps the signature and stays on the incremental path.
static std::string topoSig(const nlohmann::json& p) {
  std::string s;
  if (p.contains("nodes"))
    for (auto& n : p["nodes"]) {
      s += std::to_string(n.value("id", -1));
      s += ':';
      s += n.value("typeName", std::string());
      s += ';';
    }
  s += '|';
  if (p.contains("connections"))
    s += p["connections"].dump();
  return s;
}

void GraphRunner::loop() {
  GraphEval ev;
  bool evValid = false;
  std::string sig;
  std::set<int> carryDirty;  // dirty ids from an interrupted cascade

  for (;;) {
    Job job;
    {
      std::unique_lock<std::mutex> lk(m_mx);
      m_cv.wait(lk, [&] { return m_quit || m_hasPending; });
      if (m_quit)
        return;
      job = std::move(m_pending);
      m_pending = Job{};
      m_hasPending = false;
    }

    m_progDone = 0;
    m_progTotal = 0;

    if (!job.full) {
      job.dirty.insert(carryDirty.begin(), carryDirty.end());
      carryDirty.clear();
    }

    const bool incremental = !job.full && evValid &&
                             topoSig(job.project) == sig &&
                             ev.updateAllParams(job.project);
    if (incremental) {
      // Changed nodes first, then their downstream cone, one node at
      // a time — each result streams to the UI as soon as it exists,
      // and a newer request interrupts between nodes.
      std::set<int> cone(job.dirty);
      if (job.propagate)
        for (int d : job.dirty)
          for (int n : ev.downstreamOf(d))
            cone.insert(n);
      std::vector<int> order;
      for (int id : ev.executionOrder())
        if (cone.count(id))
          order.push_back(id);

      m_progTotal = (int)order.size();
      bool interrupted = false;
      for (int id : order) {
        {
          std::lock_guard<std::mutex> lk(m_mx);
          if (m_hasPending) {
            interrupted = true;
          }
        }
        if (interrupted)
          break;
        if (!ev.runNode(id))
          continue;
        NodeResult nr;
        nr.nodeId = id;
        if (const auto* outs = ev.nodeOutputs(id))
          nr.outputs = *outs;
        {
          std::lock_guard<std::mutex> lk(m_mx);
          m_stream.push_back(std::move(nr));
        }
        m_progDone = m_progDone + 1;
      }

      if (interrupted) {
        // the replacing job re-runs these (and their cones)
        carryDirty = job.dirty;
        continue;
      }

      Result res;
      res.ok = true;
      GenTexture* fin = ev.finalOutput();
      if (fin && fin->Data) {
        res.finalOutput = *fin;
        res.hasFinal = true;
      }
      {
        std::lock_guard<std::mutex> lk(m_mx);
        m_done = std::move(res);
        m_hasDone = true;
        if (!m_hasPending)
          m_busy = false;
      }
      continue;
    }

    // full rebuild + run
    Result res;
    ev = GraphEval();
    bool loaded = ev.load(job.project);
    bool ran = loaded && ev.run([this](int done, int total) {
      m_progDone = done;
      m_progTotal = total;
    });
    if (!loaded)
      fprintf(stderr, "GraphRunner: load failed\n");
    else if (!ran)
      fprintf(stderr, "GraphRunner: run failed\n");
    evValid = ran;
    sig = ran ? topoSig(job.project) : std::string();
    carryDirty.clear();
    if (ran) {
      res.ok = true;
      for (auto& nj : job.project["nodes"]) {
        int id = nj.value("id", -1);
        std::string type = nj.value("typeName", std::string());
        auto core = getCoreNodeRegistry().create(type);
        if (!core)
          continue;
        auto outs = core->outputSlotNames();
        std::vector<GenTexture> texs(outs.size());
        for (size_t i = 0; i < outs.size(); i++) {
          GenTexture* t = ev.outputOf(id, outs[i]);
          if (t && t->Data)
            texs[i] = *t;
        }
        res.outputs.emplace(id, std::move(texs));
      }
      GenTexture* fin = ev.finalOutput();
      if (fin && fin->Data) {
        res.finalOutput = *fin;
        res.hasFinal = true;
      }
    }

    {
      std::lock_guard<std::mutex> lk(m_mx);
      m_done = std::move(res);
      m_hasDone = true;
      if (!m_hasPending)
        m_busy = false;
    }
  }
}
