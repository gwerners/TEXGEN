#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "ImNodesEz.h"
#include "gentexture.hpp"

// Abstract base class for all texture processing nodes.
// Each node receives input images and produces output images.
class TextureNode {
 public:
  virtual ~TextureNode() = default;

  // Unique type identifier for serialization
  virtual std::string typeName() const = 0;
  // Display name shown in node header
  virtual std::string displayTitle() const { return typeName(); }

  // Input/Output slot definitions for ImNodes
  virtual std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const = 0;
  virtual std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const = 0;

  // Names in same order as slotInfos (used to match connections during
  // generation)
  virtual std::vector<std::string> inputSlotNames() const = 0;
  virtual std::vector<std::string> outputSlotNames() const = 0;

  // Execute the node. inputs[i] corresponds to inputSlotNames()[i] (nullptr if
  // not connected). outputs must be resized to outputSlotNames().size() and
  // filled by this function.
  virtual void execute(const std::vector<GenTexture*>& inputs,
                       std::vector<GenTexture>& outputs) = 0;

  // Render ImGui widgets for this node's parameters (called inside the node
  // box)
  virtual void renderParams() {}

  // JSON serialization of parameters
  virtual nlohmann::json saveParams() const { return nlohmann::json::object(); }
  virtual void loadParams(const nlohmann::json&) {}

  // Graph metadata
  ImVec2 pos = {100.0f, 100.0f};
  bool selected = false;
  int id = -1;
};
