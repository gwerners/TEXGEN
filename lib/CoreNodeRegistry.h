#pragma once
// CoreNodeRegistry — factory registry for CoreNode types.
// Used by the headless graph evaluator to instantiate nodes from JSON.
// No UI dependencies.

#include "CoreNode.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

using CoreNodeFactory = std::function<std::unique_ptr<CoreNode>()>;

// Static per-type metadata, the single source for creation menus,
// tooltips and hint bars (GUI and any future tooling).
struct NodeMeta {
  const char *category;    // e.g. "Generator", "Filter"
  const char *description; // one-line summary
};

// Metadata for a node type, or nullptr if the type is unknown.
const NodeMeta *getNodeMeta(const std::string &typeName);

// Categories in menu display order.
const std::vector<const char *> &nodeCategoryOrder();

class CoreNodeRegistry {
public:
  void add(const std::string &typeName, CoreNodeFactory factory);
  std::unique_ptr<CoreNode> create(const std::string &typeName) const;
  bool has(const std::string &typeName) const;

private:
  std::map<std::string, CoreNodeFactory> m_factories;
};

// Get the global registry (lazy-initialized with all built-in node types).
CoreNodeRegistry &getCoreNodeRegistry();
