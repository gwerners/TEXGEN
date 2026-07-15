#pragma once
// GraphCoreNodes — structural node types: Subgraph (nested graph) and
// Remote (exposed parameters). Pure logic, no UI dependencies.
#include <string>
#include <vector>
#include "CoreNode.h"
#include "gentexture.hpp"

// ============================================================
// SubgraphCoreNode — evaluates an inner node graph.
// Ports are fixed at load time; port names are stable storage so the
// editor can point slot titles at them.
// ============================================================
class SubgraphCoreNode : public CoreNode {
 public:
  struct InPort {
    std::string name;
    // inner (nodeId, slotName) destinations fed by this port
    std::vector<std::pair<int, std::string>> targets;
  };
  struct OutPort {
    std::string name;
    int id = -1;         // inner node id
    std::string slot;    // inner output slot
  };

  SubgraphCoreNode();
  std::string typeName() const override { return "Subgraph"; }
  std::string displayTitle() const override { return m_title; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  std::string m_title;
  nlohmann::json m_graph; // {nodes, connections}
  std::vector<InPort> m_inputs;
  std::vector<OutPort> m_outputs;
};

// ============================================================
// RemoteCoreNode — a panel of values that drive parameters of other
// nodes. Evaluation-side application happens in GraphEval (headless)
// and NodeGraph (editor); the node itself processes nothing.
// ============================================================
class RemoteCoreNode : public CoreNode {
 public:
  struct Link {
    std::string label;
    int nodeId = -1;
    std::string param;
    float minV = 0.0f;
    float maxV = 1.0f;
    float value = 0.5f;
  };

  RemoteCoreNode();
  std::string typeName() const override { return "Remote"; }
  std::vector<std::string> inputSlotNames() const override { return {}; }
  std::vector<std::string> outputSlotNames() const override { return {}; }
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  std::vector<Link> m_links;
};
