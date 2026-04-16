#pragma once
// CoreNode — pure base class for texture processing nodes.
// No UI dependencies (no ImGui, ImNodes, raylib).
// Used by libtexgen for headless evaluation and C export.

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "gentexture.hpp"

class CoreNode {
public:
  virtual ~CoreNode() = default;

  // Unique type identifier for serialization (e.g. "Noise", "Crystal")
  virtual std::string typeName() const = 0;

  // Display name shown in node header (defaults to typeName)
  virtual std::string displayTitle() const { return typeName(); }

  // Slot names in order (used to match connections during evaluation)
  virtual std::vector<std::string> inputSlotNames() const = 0;
  virtual std::vector<std::string> outputSlotNames() const = 0;

  // Execute the node.
  // inputs[i] corresponds to inputSlotNames()[i] (nullptr if not connected).
  // outputs must be resized and filled by this function.
  virtual void execute(const std::vector<GenTexture *> &inputs,
                       std::vector<GenTexture> &outputs) = 0;

  // JSON serialization of parameters
  virtual nlohmann::json saveParams() const {
    return nlohmann::json::object();
  }
  virtual void loadParams(const nlohmann::json &) {}

  // Node identity (set by graph)
  int id = -1;
};
