#include "CoreNodeRegistry.h"

void CoreNodeRegistry::add(const std::string &typeName,
                           CoreNodeFactory factory) {
  m_factories[typeName] = std::move(factory);
}

std::unique_ptr<CoreNode>
CoreNodeRegistry::create(const std::string &typeName) const {
  auto it = m_factories.find(typeName);
  if (it != m_factories.end())
    return it->second();
  return nullptr;
}

bool CoreNodeRegistry::has(const std::string &typeName) const {
  return m_factories.count(typeName) > 0;
}

// The global registry is currently empty — nodes are evaluated via
// HeadlessEval's built-in logic. In the future, as nodes are migrated
// to pure CoreNode implementations, they will be registered here.
CoreNodeRegistry &getCoreNodeRegistry() {
  static CoreNodeRegistry reg;
  return reg;
}
