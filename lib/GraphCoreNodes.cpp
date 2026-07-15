#include "GraphCoreNodes.h"

#include "GraphEval.h"

// ============================================================
// SubgraphCoreNode
// ============================================================

SubgraphCoreNode::SubgraphCoreNode() : m_title("Subgraph") {
  m_graph = {{"nodes", nlohmann::json::array()},
             {"connections", nlohmann::json::array()}};
}

std::vector<std::string> SubgraphCoreNode::inputSlotNames() const {
  std::vector<std::string> names;
  for (auto& p : m_inputs)
    names.push_back(p.name);
  return names;
}

std::vector<std::string> SubgraphCoreNode::outputSlotNames() const {
  std::vector<std::string> names;
  for (auto& p : m_outputs)
    names.push_back(p.name);
  return names;
}

void SubgraphCoreNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  GraphEval ge;
  if (!ge.load(m_graph)) {
    outputs.resize(m_outputs.size());
    return;
  }
  // feed outer inputs into the declared inner destinations
  for (size_t i = 0; i < m_inputs.size(); i++) {
    if (i >= inputs.size() || !inputs[i] || !inputs[i]->Data)
      continue;
    int virtualId = -1000 - (int)i;
    ge.injectInput(virtualId, inputs[i]);
    for (auto& tgt : m_inputs[i].targets)
      ge.addConnection(virtualId, "Out", tgt.first, tgt.second);
  }
  ge.run();
  outputs.clear();
  outputs.resize(m_outputs.size());
  for (size_t k = 0; k < m_outputs.size(); k++) {
    GenTexture* t = ge.outputOf(m_outputs[k].id, m_outputs[k].slot);
    if (t && t->Data)
      outputs[k] = *t;
  }
}

nlohmann::json SubgraphCoreNode::saveParams() const {
  nlohmann::json ins = nlohmann::json::array();
  for (auto& p : m_inputs) {
    nlohmann::json targets = nlohmann::json::array();
    for (auto& t : p.targets)
      targets.push_back({t.first, t.second});
    ins.push_back({{"name", p.name}, {"targets", targets}});
  }
  nlohmann::json outs = nlohmann::json::array();
  for (auto& p : m_outputs)
    outs.push_back({{"name", p.name}, {"id", p.id}, {"slot", p.slot}});
  return {{"title", m_title},
          {"graph", m_graph},
          {"inputs", ins},
          {"outputs", outs}};
}

void SubgraphCoreNode::loadParams(const nlohmann::json& j) {
  m_title = j.value("title", std::string("Subgraph"));
  if (j.contains("graph"))
    m_graph = j["graph"];
  m_inputs.clear();
  for (auto& p : j.value("inputs", nlohmann::json::array())) {
    InPort ip;
    ip.name = p.value("name", std::string("In"));
    for (auto& t : p.value("targets", nlohmann::json::array()))
      if (t.is_array() && t.size() >= 2)
        ip.targets.push_back({t[0].get<int>(), t[1].get<std::string>()});
    m_inputs.push_back(std::move(ip));
  }
  m_outputs.clear();
  for (auto& p : j.value("outputs", nlohmann::json::array())) {
    OutPort op;
    op.name = p.value("name", std::string("Out"));
    op.id = p.value("id", -1);
    op.slot = p.value("slot", std::string("Out"));
    m_outputs.push_back(std::move(op));
  }
}

// ============================================================
// RemoteCoreNode
// ============================================================

RemoteCoreNode::RemoteCoreNode() {}

void RemoteCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                             std::vector<GenTexture>& outputs) {
  outputs.resize(0); // values are applied by the graph evaluator
}

nlohmann::json RemoteCoreNode::saveParams() const {
  nlohmann::json links = nlohmann::json::array();
  for (auto& l : m_links)
    links.push_back({{"label", l.label},
                     {"nodeId", l.nodeId},
                     {"param", l.param},
                     {"min", l.minV},
                     {"max", l.maxV},
                     {"value", l.value}});
  return {{"links", links}};
}

void RemoteCoreNode::loadParams(const nlohmann::json& j) {
  m_links.clear();
  for (auto& l : j.value("links", nlohmann::json::array())) {
    Link link;
    link.label = l.value("label", std::string("Value"));
    link.nodeId = l.value("nodeId", -1);
    link.param = l.value("param", std::string());
    link.minV = l.value("min", 0.0f);
    link.maxV = l.value("max", 1.0f);
    link.value = l.value("value", 0.5f);
    m_links.push_back(std::move(link));
  }
}
