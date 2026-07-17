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
    m_pending = std::move(project);
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

void GraphRunner::loop() {
  for (;;) {
    nlohmann::json job;
    {
      std::unique_lock<std::mutex> lk(m_mx);
      m_cv.wait(lk, [&] { return m_quit || m_hasPending; });
      if (m_quit)
        return;
      job = std::move(m_pending);
      m_hasPending = false;
    }

    Result res;
    {
      GraphEval ev;
      bool loaded = ev.load(job);
      bool ran = loaded && ev.run();
      if (!loaded)
        fprintf(stderr, "GraphRunner: load failed\n");
      else if (!ran)
        fprintf(stderr, "GraphRunner: run failed\n");
      if (ran) {
        res.ok = true;
        for (auto& nj : job["nodes"]) {
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
