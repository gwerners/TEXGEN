#include "StructNodes.h"
#include <imgui.h>
#include "UiWidgets.h"
#include <cstring>


// ============================================================
// SubgraphNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SubgraphNode::inputSlotInfos() const {
  std::vector<ImNodes::Ez::SlotInfo> infos;
  for (auto& p : m_core.m_inputs)
    infos.push_back({p.name.c_str(), 1});
  return infos;
}
std::vector<ImNodes::Ez::SlotInfo> SubgraphNode::outputSlotInfos() const {
  std::vector<ImNodes::Ez::SlotInfo> infos;
  for (auto& p : m_core.m_outputs)
    infos.push_back({p.name.c_str(), 1});
  return infos;
}

void SubgraphNode::renderParams() {
  ImGui::PushItemWidth(140);
  char buf[128];
  strncpy(buf, m_core.m_title.c_str(), sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  if (ImGui::InputText("Title##sub", buf, sizeof(buf)))
    m_core.m_title = buf;
  Hint("Display name of this subgraph");
  int nNodes = m_core.m_graph.contains("nodes")
                   ? (int)m_core.m_graph["nodes"].size()
                   : 0;
  ImGui::Text("%d inner nodes, %d in / %d out", nNodes,
              (int)m_core.m_inputs.size(), (int)m_core.m_outputs.size());
  ImGui::TextDisabled("Right-click canvas: Ungroup Selected");
  ImGui::PopItemWidth();
}

// ============================================================
// RemoteNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> RemoteNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> RemoteNode::outputSlotInfos() const {
  return {};
}

void RemoteNode::renderParams() {
  ImGui::PushItemWidth(140);
  ImGui::TextDisabled("Exposed parameters");
  Hint(
      "Each slider drives a parameter of another node.\n"
      "Open 'edit' to choose the target node id and parameter key.");
  int removeIdx = -1;
  for (int i = 0; i < (int)m_core.m_links.size(); i++) {
    auto& l = m_core.m_links[i];
    ImGui::PushID(i);
    ImGui::SliderFloat(l.label.empty() ? "##val" : l.label.c_str(), &l.value,
                       l.minV, l.maxV);
    if (ImGui::TreeNode("edit##link")) {
      char buf[64];
      strncpy(buf, l.label.c_str(), sizeof(buf) - 1);
      buf[sizeof(buf) - 1] = '\0';
      if (ImGui::InputText("Label##rl", buf, sizeof(buf)))
        l.label = buf;
      ImGui::InputInt("Node id##rl", &l.nodeId);
      Hint("The id of the node to control (shown in its header)");
      char pbuf[64];
      strncpy(pbuf, l.param.c_str(), sizeof(pbuf) - 1);
      pbuf[sizeof(pbuf) - 1] = '\0';
      if (ImGui::InputText("Param##rl", pbuf, sizeof(pbuf)))
        l.param = pbuf;
      Hint("The JSON parameter key (as in the project file)");
      ImGui::DragFloat("Min##rl", &l.minV, 0.01f);
      ImGui::DragFloat("Max##rl", &l.maxV, 0.01f);
      if (ImGui::SmallButton("remove##rl"))
        removeIdx = i;
      ImGui::TreePop();
    }
    ImGui::PopID();
  }
  if (removeIdx >= 0)
    m_core.m_links.erase(m_core.m_links.begin() + removeIdx);
  if (ImGui::SmallButton("+ link"))
    m_core.m_links.push_back({});
  ImGui::PopItemWidth();
}
