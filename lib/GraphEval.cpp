#include "GraphEval.h"

#include <queue>

#include "CoreNodeRegistry.h"

// ============================================================
// JSON transforms
// ============================================================

nlohmann::json applyRemotes(const nlohmann::json &project) {
  nlohmann::json out = project;
  if (!out.contains("nodes"))
    return out;
  // id -> index in nodes array
  std::map<int, size_t> byId;
  for (size_t i = 0; i < out["nodes"].size(); i++)
    byId[out["nodes"][i]["id"].get<int>()] = i;

  for (auto &n : out["nodes"]) {
    if (n["typeName"].get<std::string>() != "Remote")
      continue;
    auto params = n.value("params", nlohmann::json::object());
    if (!params.contains("links"))
      continue;
    for (auto &link : params["links"]) {
      int target = link.value("nodeId", -1);
      std::string key = link.value("param", std::string());
      auto it = byId.find(target);
      if (it == byId.end() || key.empty())
        continue;
      out["nodes"][it->second]["params"][key] = link.value("value", 0.0f);
    }
  }
  return out;
}

nlohmann::json flattenSubgraphs(const nlohmann::json &project) {
  nlohmann::json out = project;
  if (!out.contains("nodes"))
    return out;

  bool found = true;
  int guard = 0;
  while (found && guard++ < 32) {
    found = false;
    nlohmann::json nodes = nlohmann::json::array();
    nlohmann::json conns =
        out.value("connections", nlohmann::json::array());

    // next free id
    int nextId = 0;
    for (auto &n : out["nodes"])
      nextId = std::max(nextId, n["id"].get<int>() + 1);

    for (auto &n : out["nodes"]) {
      if (n["typeName"].get<std::string>() != "Subgraph") {
        nodes.push_back(n);
        continue;
      }
      found = true;
      int subId = n["id"].get<int>();
      auto params = n.value("params", nlohmann::json::object());
      auto inner = params.value("graph", nlohmann::json::object());
      auto inPorts = params.value("inputs", nlohmann::json::array());
      auto outPorts = params.value("outputs", nlohmann::json::array());

      // remap inner node ids (two passes: ids first, then Remote links,
      // so links only see the map of THIS subgraph's nodes)
      std::map<int, int> idMap;
      auto innerNodes = inner.value("nodes", nlohmann::json::array());
      for (auto &innerNode : innerNodes) {
        int oldId = innerNode["id"].get<int>();
        idMap[oldId] = nextId++;
      }
      for (auto innerNode : innerNodes) {
        innerNode["id"] = idMap[innerNode["id"].get<int>()];
        if (innerNode["typeName"].get<std::string>() == "Remote" &&
            innerNode.contains("params") &&
            innerNode["params"].contains("links")) {
          for (auto &link : innerNode["params"]["links"]) {
            int t = link.value("nodeId", -1);
            if (idMap.count(t))
              link["nodeId"] = idMap[t];
          }
        }
        nodes.push_back(innerNode);
      }
      // inner connections
      for (auto &c : inner.value("connections", nlohmann::json::array())) {
        nlohmann::json cj = c;
        int f = cj["fromId"].get<int>(), t = cj["toId"].get<int>();
        if (!idMap.count(f) || !idMap.count(t))
          continue;
        cj["fromId"] = idMap[f];
        cj["toId"] = idMap[t];
        conns.push_back(cj);
      }
      // rewire boundary connections
      nlohmann::json newConns = nlohmann::json::array();
      for (auto &c : conns) {
        int f = c["fromId"].get<int>(), t = c["toId"].get<int>();
        if (t == subId) {
          // outer source -> subgraph input port
          std::string port = c["toSlot"].get<std::string>();
          for (auto &ip : inPorts) {
            if (ip.value("name", std::string()) != port)
              continue;
            for (auto &tgt : ip.value("targets", nlohmann::json::array())) {
              int ti = tgt[0].get<int>();
              if (!idMap.count(ti))
                continue;
              newConns.push_back({{"fromId", f},
                                  {"fromSlot", c["fromSlot"]},
                                  {"toId", idMap[ti]},
                                  {"toSlot", tgt[1]}});
            }
          }
        } else if (f == subId) {
          // subgraph output port -> outer destination
          std::string port = c["fromSlot"].get<std::string>();
          for (auto &op : outPorts) {
            if (op.value("name", std::string()) != port)
              continue;
            int si = op.value("id", -1);
            if (!idMap.count(si))
              continue;
            newConns.push_back({{"fromId", idMap[si]},
                                {"fromSlot", op.value("slot", std::string())},
                                {"toId", t},
                                {"toSlot", c["toSlot"]}});
          }
        } else {
          newConns.push_back(c);
        }
      }
      conns = newConns;
    }
    out["nodes"] = nodes;
    out["connections"] = conns;
  }
  return out;
}

// ============================================================
// GraphEval
// ============================================================

bool GraphEval::load(const nlohmann::json &rawProject) {
  nlohmann::json project = applyRemotes(rawProject);
  if (!project.contains("nodes"))
    return false;
  auto &registry = getCoreNodeRegistry();

  for (auto &n : project["nodes"]) {
    int id = n.value("id", -1);
    std::string type = n.value("typeName", std::string());
    auto core = registry.create(type);
    if (!core || id < 0)
      continue; // unknown/legacy type — skip
    core->id = id;
    // Unexpected value types (old project formats) keep node defaults
    // instead of aborting the evaluation.
    try {
      core->loadParams(n.value("params", nlohmann::json::object()));
    } catch (const std::exception &) {
    }
    ENode en;
    en.inSlots = core->inputSlotNames();
    en.outSlots = core->outputSlotNames();
    en.core = std::move(core);
    m_nodes[id] = std::move(en);
  }
  if (project.contains("connections")) {
    for (auto &c : project["connections"]) {
      if (!c.contains("fromId") || !c.contains("toId"))
        continue;
      addConnection(c.value("fromId", -1),
                    c.value("fromSlot", std::string()), c.value("toId", -1),
                    c.value("toSlot", std::string()));
    }
  }
  return true;
}

void GraphEval::injectInput(int virtualId, GenTexture *tex) {
  m_injected[virtualId] = tex;
}

void GraphEval::addConnection(int fromId, const std::string &fromSlot,
                              int toId, const std::string &toSlot) {
  m_conns.push_back({fromId, fromSlot, toId, toSlot});
}

GenTexture *GraphEval::outputOf(int nodeId, const std::string &slot) {
  auto inj = m_injected.find(nodeId);
  if (inj != m_injected.end())
    return inj->second;
  auto it = m_nodes.find(nodeId);
  if (it == m_nodes.end())
    return nullptr;
  ENode &en = it->second;
  size_t slotIdx = 0;
  for (size_t k = 0; k < en.outSlots.size(); k++)
    if (en.outSlots[k] == slot) {
      slotIdx = k;
      break;
    }
  if (slotIdx < en.outputs.size() && en.outputs[slotIdx].Data)
    return &en.outputs[slotIdx];
  return nullptr;
}

bool GraphEval::run() {
  // Kahn topological sort over real nodes
  std::map<int, int> inDeg;
  std::map<int, std::vector<int>> adj;
  for (auto &[id, _] : m_nodes) {
    inDeg[id] = 0;
    adj[id] = {};
  }
  for (auto &c : m_conns) {
    if (!m_nodes.count(c.toId))
      continue;
    if (m_nodes.count(c.fromId)) {
      adj[c.fromId].push_back(c.toId);
      inDeg[c.toId]++;
    }
    // injected sources have no ordering constraints
  }
  std::vector<int> sorted;
  std::queue<int> q;
  for (auto &[id, d] : inDeg)
    if (d == 0)
      q.push(id);
  while (!q.empty()) {
    int cur = q.front();
    q.pop();
    sorted.push_back(cur);
    for (int next : adj[cur])
      if (--inDeg[next] == 0)
        q.push(next);
  }
  for (auto &[id, _] : m_nodes) {
    bool foundId = false;
    for (int s : sorted)
      if (s == id) {
        foundId = true;
        break;
      }
    if (!foundId)
      sorted.push_back(id);
  }

  for (int id : sorted) {
    ENode &en = m_nodes[id];
    if (en.core->typeName() == "Output") {
      for (auto &c : m_conns) {
        if (c.toId == id && c.toSlot == "In") {
          m_outputSrcId = c.fromId;
          m_outputSrcSlot = c.fromSlot;
        }
      }
      continue;
    }
    std::vector<GenTexture *> inputs(en.inSlots.size(), nullptr);
    for (size_t i = 0; i < en.inSlots.size(); i++) {
      for (auto &c : m_conns) {
        if (c.toId != id || c.toSlot != en.inSlots[i])
          continue;
        GenTexture *src = outputOf(c.fromId, c.fromSlot);
        if (src && src->Data)
          inputs[i] = src;
        break;
      }
    }
    en.core->execute(inputs, en.outputs);
  }
  return true;
}

GenTexture *GraphEval::finalOutput() {
  if (m_outputSrcId < 0)
    return nullptr;
  return outputOf(m_outputSrcId, m_outputSrcSlot);
}
