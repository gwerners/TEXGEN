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
