#pragma once
// TextureNode — UI-aware base class for texture processing nodes.
// Inherits CoreNode (pure logic) and adds ImGui/ImNodes UI integration.

#include <string>
#include <vector>
#include "CoreNode.h"
#include "ImNodesEz.h"

class TextureNode : public CoreNode {
 public:
  // Input/Output slot definitions for ImNodes UI
  virtual std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const = 0;
  virtual std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const = 0;

  // Render ImGui widgets for this node's parameters
  virtual void renderParams() {}

  // Graph UI metadata
  ImVec2 pos = {100.0f, 100.0f};
  bool selected = false;
};

// UiNode<Core> — thin UI wrapper over a pure CoreNode implementation.
// All logic (params, execute, serialization) lives in the embedded core;
// subclasses only implement renderParams() and the ImNodes slot infos.
template <typename Core>
class UiNode : public TextureNode {
 public:
  std::string typeName() const override { return m_core.typeName(); }
  std::string displayTitle() const override { return m_core.displayTitle(); }
  std::vector<std::string> inputSlotNames() const override {
    return m_core.inputSlotNames();
  }
  std::vector<std::string> outputSlotNames() const override {
    return m_core.outputSlotNames();
  }
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override {
    m_core.execute(inputs, outputs);
  }
  nlohmann::json saveParams() const override { return m_core.saveParams(); }
  void loadParams(const nlohmann::json& j) override { m_core.loadParams(j); }

  Core& core() { return m_core; }

 protected:
  Core m_core;
};
