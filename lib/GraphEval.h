#pragma once
// GraphEval — reusable JSON-graph evaluator over the CoreNodeRegistry.
// Used by headlessEvaluate() and by SubgraphCoreNode (nested graphs).
// No UI dependencies.

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CoreNode.h"
#include <nlohmann/json.hpp>

class GraphEval {
public:
  // Instantiate cores from the registry and load params.
  // Remote nodes are applied (their link values patch target params).
  bool load(const nlohmann::json &project);

  // Register an external texture as the "Out" output of a virtual node.
  // Use negative ids to avoid clashing with real nodes.
  void injectInput(int virtualId, GenTexture *tex);

  // Add a connection (in addition to the ones from the project JSON).
  void addConnection(int fromId, const std::string &fromSlot, int toId,
                     const std::string &toSlot);

  // Topological sort + execute. "Output" nodes are not executed (they
  // write files); the texture feeding the last one is remembered instead.
  // The optional progress callback fires after each executed node with
  // (done, total) — used by async runners to report evaluation state.
  bool run(const std::function<void(int, int)> &progress = {});

  // Output texture of a node's slot after run() (nullptr if absent).
  GenTexture *outputOf(int nodeId, const std::string &slot);

  // Texture feeding the last "Output" node (nullptr if none).
  GenTexture *finalOutput();

  // --- incremental evaluation (interactive editors) ---
  // After a full run(), a param-only edit can be applied in place and
  // just the affected nodes re-executed against the cached upstream
  // outputs — no full graph rebuild.

  // Reload every node's params from a snapshot with the same topology
  // (same node ids and types). Returns false on a mismatch, in which
  // case the caller must fall back to load() + run().
  bool updateAllParams(const nlohmann::json &project);

  // Re-execute a single node using the cached outputs of its inputs.
  bool runNode(int nodeId);

  // All loaded nodes in execution (topological) order.
  std::vector<int> executionOrder();

  // Nodes strictly downstream of nodeId, in execution order.
  std::vector<int> downstreamOf(int nodeId);

  // Cached outputs of a node (nullptr if unknown).
  const std::vector<GenTexture> *nodeOutputs(int nodeId) const;

private:
  void execNode(int id);
  struct ENode {
    std::unique_ptr<CoreNode> core;
    std::vector<std::string> inSlots;
    std::vector<std::string> outSlots;
    std::vector<GenTexture> outputs;
  };
  struct Conn {
    int fromId;
    std::string fromSlot;
    int toId;
    std::string toSlot;
  };

  std::map<int, ENode> m_nodes;
  std::map<int, GenTexture *> m_injected;
  std::vector<Conn> m_conns;
  int m_outputSrcId = -1;
  std::string m_outputSrcSlot;
};

// Patch Remote node link values onto their target nodes' params.
// Pure JSON transform (used by GraphEval::load and the C exporter).
nlohmann::json applyRemotes(const nlohmann::json &project);

// Recursively replace Subgraph nodes with their inner graphs, remapping
// ids and rewiring boundary connections. Pure JSON transform (used by
// the C exporter; headless/GUI evaluate subgraphs natively).
nlohmann::json flattenSubgraphs(const nlohmann::json &project);
