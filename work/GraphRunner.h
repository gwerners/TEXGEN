#pragma once
// GraphRunner — evaluates graph snapshots on a worker thread so heavy
// graphs never freeze the UI. The UI serializes the graph to JSON and
// requests a run; the worker evaluates it with GraphEval (independent
// core instances, no shared state) and hands back the per-node output
// textures. GL uploads stay on the main thread.
//
// Two request flavors:
//  - request():     full rebuild + run (load, structural edits).
//  - requestNode(): param-only edit. The worker keeps its GraphEval
//    alive between jobs, so it re-runs just the changed node first
//    (fast feedback while dragging a slider), then walks the node's
//    downstream cone one node at a time, streaming each result to
//    pollStream() as it lands. A newer request interrupts the cascade
//    between nodes; the interrupted dirty ids carry over so nothing
//    stays stale. Topology mismatches fall back to a full run.

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "gentexture.hpp"
#include <nlohmann/json.hpp>

class GraphRunner {
public:
  struct Result {
    std::map<int, std::vector<GenTexture>> outputs; // node id -> outputs
    GenTexture finalOutput;
    bool hasFinal = false;
    bool ok = false;
  };
  struct NodeResult {
    int nodeId = -1;
    std::vector<GenTexture> outputs;
  };

  GraphRunner();
  ~GraphRunner();

  // Queue a full-rebuild snapshot (replaces any queued job).
  void request(nlohmann::json project);

  // Queue an incremental re-run of changedId (+ downstream cone when
  // propagate). Coalesces with a queued incremental job by unioning
  // the dirty ids; a queued full job stays full.
  void requestNode(nlohmann::json project, int changedId, bool propagate);

  // True while a run is executing or queued.
  bool busy() const { return m_busy.load(); }

  // Evaluation progress of the current run (nodes done / total).
  int progressDone() const { return m_progDone.load(); }
  int progressTotal() const { return m_progTotal.load(); }

  // Moves a finished full result (or an incremental run's final
  // output) out; returns false if none is ready.
  bool poll(Result &out);

  // Drains the per-node results streamed by incremental runs.
  void pollStream(std::vector<NodeResult> &out);

private:
  struct Job {
    nlohmann::json project;
    std::set<int> dirty; // changed node ids (ignored when full)
    bool full = true;
    bool propagate = true;
  };

  void loop();

  std::thread m_thread;
  mutable std::mutex m_mx;
  std::condition_variable m_cv;
  Job m_pending;
  bool m_hasPending = false;
  Result m_done;
  bool m_hasDone = false;
  std::deque<NodeResult> m_stream;
  bool m_quit = false;
  std::atomic<bool> m_busy{false};
  std::atomic<int> m_progDone{0};
  std::atomic<int> m_progTotal{0};
};
