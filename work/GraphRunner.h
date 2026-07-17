#pragma once
// GraphRunner — evaluates graph snapshots on a worker thread so heavy
// graphs never freeze the UI. The UI serializes the graph to JSON and
// requests a run; the worker evaluates it with GraphEval (independent
// core instances, no shared state) and hands back the per-node output
// textures. Requests coalesce: while a run is in flight only the
// latest queued snapshot is kept. GL uploads stay on the main thread.

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>
#include "gentexture.hpp"

class GraphRunner {
 public:
  struct Result {
    std::map<int, std::vector<GenTexture>> outputs;  // node id -> outputs
    GenTexture finalOutput;
    bool hasFinal = false;
    bool ok = false;
  };

  GraphRunner();
  ~GraphRunner();

  // Queue a snapshot for evaluation (replaces any queued one).
  void request(nlohmann::json project);

  // True while a run is executing or queued.
  bool busy() const { return m_busy.load(); }

  // Moves a finished result out; returns false if none is ready.
  bool poll(Result& out);

 private:
  void loop();

  std::thread m_thread;
  mutable std::mutex m_mx;
  std::condition_variable m_cv;
  nlohmann::json m_pending;
  bool m_hasPending = false;
  Result m_done;
  bool m_hasDone = false;
  bool m_quit = false;
  std::atomic<bool> m_busy{false};
};
