// HeadlessEval — generic JSON-project evaluation via the CoreNodeRegistry.
// Every registered node type is supported; the logic executed here is the
// exact same CoreNode::execute() code the editor runs.
#include "HeadlessEval.h"

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "CoreNodeRegistry.h"

namespace {

struct EvalNode {
  std::unique_ptr<CoreNode> core;
  std::vector<std::string> inputSlots;
  std::vector<std::string> outputSlots;
  std::vector<GenTexture> outputs;
};

// Find source (nodeId, slotName) for a given input slot, or -1.
int findSource(const nlohmann::json &conns, int nodeId,
               const std::string &slot, std::string &fromSlot) {
  for (auto &c : conns) {
    if (c["toId"].get<int>() == nodeId &&
        c["toSlot"].get<std::string>() == slot) {
      fromSlot = c["fromSlot"].get<std::string>();
      return c["fromId"].get<int>();
    }
  }
  return -1;
}

} // namespace

bool headlessEvaluate(const nlohmann::json &project, GenTexture &output) {
  auto &nodes = project["nodes"];
  auto &conns = project["connections"];
  auto &registry = getCoreNodeRegistry();

  // Instantiate all nodes from the registry and load their params.
  std::map<int, EvalNode> graph;
  for (auto &n : nodes) {
    int id = n["id"].get<int>();
    std::string type = n["typeName"].get<std::string>();
    auto core = registry.create(type);
    if (!core)
      continue; // unknown/legacy type — skip (matches editor behavior)
    core->id = id;
    core->loadParams(n.value("params", nlohmann::json::object()));
    EvalNode en;
    en.inputSlots = core->inputSlotNames();
    en.outputSlots = core->outputSlotNames();
    en.core = std::move(core);
    graph[id] = std::move(en);
  }

  // Topological sort (Kahn's).
  std::map<int, int> inDeg;
  std::map<int, std::vector<int>> adj;
  for (auto &[id, _] : graph) {
    inDeg[id] = 0;
    adj[id] = {};
  }
  for (auto &c : conns) {
    int from = c["fromId"].get<int>();
    int to = c["toId"].get<int>();
    if (!graph.count(from) || !graph.count(to))
      continue;
    adj[from].push_back(to);
    inDeg[to]++;
  }
  std::vector<int> sorted;
  std::queue<int> q;
  for (auto &[id, d] : inDeg)
    if (d == 0)
      q.push(id);
  while (!q.empty()) {
    int cur = q.front();
    q.pop();
    sorted.push_back(cur);
    for (int next : adj[cur])
      if (--inDeg[next] == 0)
        q.push(next);
  }
  for (auto &[id, _] : graph) {
    bool found = false;
    for (int s : sorted)
      if (s == id) {
        found = true;
        break;
      }
    if (!found)
      sorted.push_back(id); // cycle fallback — still evaluate
  }

  // Execute in order. "Output" nodes are not executed (they write files);
  // instead we remember what feeds them, matching previous behavior.
  int outputSrcId = -1;
  std::string outputSrcSlot;
  for (int id : sorted) {
    auto it = graph.find(id);
    if (it == graph.end())
      continue;
    EvalNode &en = it->second;

    if (en.core->typeName() == "Output") {
      std::string fromSlot;
      int src = findSource(conns, id, "In", fromSlot);
      if (src >= 0) {
        outputSrcId = src;
        outputSrcSlot = fromSlot;
      }
      continue;
    }

    std::vector<GenTexture *> inputs(en.inputSlots.size(), nullptr);
    for (size_t i = 0; i < en.inputSlots.size(); i++) {
      std::string fromSlot;
      int src = findSource(conns, id, en.inputSlots[i], fromSlot);
      if (src < 0)
        continue;
      auto sit = graph.find(src);
      if (sit == graph.end())
        continue;
      EvalNode &sn = sit->second;
      size_t slotIdx = 0;
      for (size_t k = 0; k < sn.outputSlots.size(); k++)
        if (sn.outputSlots[k] == fromSlot) {
          slotIdx = k;
          break;
        }
      if (slotIdx < sn.outputs.size() && sn.outputs[slotIdx].Data)
        inputs[i] = &sn.outputs[slotIdx];
    }
    en.core->execute(inputs, en.outputs);
  }

  if (outputSrcId >= 0) {
    auto it = graph.find(outputSrcId);
    if (it != graph.end()) {
      EvalNode &sn = it->second;
      size_t slotIdx = 0;
      for (size_t k = 0; k < sn.outputSlots.size(); k++)
        if (sn.outputSlots[k] == outputSrcSlot) {
          slotIdx = k;
          break;
        }
      if (slotIdx < sn.outputs.size() && sn.outputs[slotIdx].Data) {
        output = sn.outputs[slotIdx];
        return true;
      }
    }
  }
  return false;
}
