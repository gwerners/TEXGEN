#pragma once
// GraphEval — reusable JSON-graph evaluator over the CoreNodeRegistry.
// Used by headlessEvaluate() and by SubgraphCoreNode (nested graphs).
// No UI dependencies.

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include "CoreNode.h"

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
  bool run();

  // Output texture of a node's slot after run() (nullptr if absent).
  GenTexture *outputOf(int nodeId, const std::string &slot);

  // Texture feeding the last "Output" node (nullptr if none).
  GenTexture *finalOutput();

 private:
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
