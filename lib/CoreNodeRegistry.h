#pragma once
// CoreNodeRegistry — factory registry for CoreNode types.
// Used by the headless graph evaluator to instantiate nodes from JSON.
// No UI dependencies.

#include "CoreNode.h"
#include <functional>
#include <map>
#include <memory>
#include <string>

using CoreNodeFactory = std::function<std::unique_ptr<CoreNode>()>;

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
