#include "Nodes.h"
#include "AllNodes.h"
#include "Utils.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <raylib.h>

#include <algorithm>
#include <cstring>
#include <queue>
#include <unordered_map>
#include <unordered_set>

// ============================================================
// Global instance
// ============================================================

NodeGraph* g_nodeGraph = nullptr;

// ============================================================
// registerAllNodes
// ============================================================

void registerAllNodes(std::map<std::string, NodeFactory>& registry) {
  registry["Input"] = []() { return std::make_unique<InputNode>(); };
  registry["Output"] = []() { return std::make_unique<OutputNode>(); };
  registry["Noise"] = []() { return std::make_unique<NoiseNode>(); };
  registry["Cells"] = []() { return std::make_unique<CellsNode>(); };
  registry["GlowRect"] = []() { return std::make_unique<GlowRectNode>(); };
  registry["ColorMatrix"] = []() {
    return std::make_unique<ColorMatrixNode>();
  };
  registry["CoordMatrix"] = []() {
    return std::make_unique<CoordMatrixNode>();
  };
  registry["ColorRemap"] = []() { return std::make_unique<ColorRemapNode>(); };
  registry["CoordRemap"] = []() { return std::make_unique<CoordRemapNode>(); };
  registry["Derive"] = []() { return std::make_unique<DeriveNode>(); };
  registry["Blur"] = []() { return std::make_unique<BlurNode>(); };
  registry["Ternary"] = []() { return std::make_unique<TernaryNode>(); };
  registry["Paste"] = []() { return std::make_unique<PasteNode>(); };
  registry["Bump"] = []() { return std::make_unique<BumpNode>(); };
  registry["LinearCombine"] = []() {
    return std::make_unique<LinearCombineNode>();
  };
  registry["Crystal"] = []() { return std::make_unique<CrystalNode>(); };
  registry["DirectionalGradient"] = []() {
    return std::make_unique<DirectionalGradientNode>();
  };
  registry["GlowEffect"] = []() { return std::make_unique<GlowEffectNode>(); };
  registry["PerlinNoiseRG2"] = []() {
    return std::make_unique<PerlinNoiseRG2Node>();
  };
  registry["BlurKernel"] = []() { return std::make_unique<BlurKernelNode>(); };
  registry["HSCB"] = []() { return std::make_unique<HSCBNode>(); };
  registry["Wavelet"] = []() { return std::make_unique<WaveletNode>(); };
  registry["ColorBalance"] = []() {
    return std::make_unique<ColorBalanceNode>();
  };
  registry["Bricks"] = []() { return std::make_unique<BricksNode>(); };
}

// ============================================================
// GraphNode
// ============================================================

GraphNode::GraphNode(std::unique_ptr<TextureNode> node, int id)
    : m_node(std::move(node)) {
  m_node->id = id;
  previewTex.id = 0;
}

GraphNode::~GraphNode() {
  if (hasPreview && previewTex.id != 0) {
    UnloadTexture(previewTex);
  }
}

void GraphNode::draw(NodeGraph* graph) {
  TextureNode* tn = m_node.get();
  std::string title = tn->displayTitle();

  if (ImNodes::Ez::BeginNode(this, title.c_str(), &tn->pos, &tn->selected)) {
    auto inSlots = tn->inputSlotInfos();
    ImNodes::Ez::InputSlots(inSlots.data(), (int)inSlots.size());

    // Render parameters (content column between input/output slots)
    tn->renderParams();

    // Reset button
    {
      std::string resetId = "Reset##" + std::to_string(tn->id);
      if (ImGui::SmallButton(resetId.c_str())) {
        graph->resetNodeParams(this);
        m_lastParamsHash = tn->saveParams().dump();
      }
    }

    // Detect parameter changes via JSON snapshot
    std::string curParams = tn->saveParams().dump();
    if (curParams != m_lastParamsHash) {
      m_lastParamsHash = curParams;
      graph->refreshNode(this);
    }

    // Show preview image
    if (hasPreview && previewTex.id != 0) {
      float maxSide = 128.0f;
      float bigger =
          (float)((previewTex.width > previewTex.height) ? previewTex.width
                                                         : previewTex.height);
      float scale = (bigger > 0) ? maxSide / bigger : 1.0f;
      if (scale > 1.0f)
        scale = 1.0f;
      ImGui::Image(ImTextureID(previewTex.id),
                   ImVec2(previewTex.width * scale, previewTex.height * scale));
    }

    auto outSlots = tn->outputSlotInfos();
    ImNodes::Ez::OutputSlots(outSlots.data(), (int)outSlots.size());

    // Handle new connections
    NodeConnection newConn;
    if (ImNodes::GetNewConnection(&newConn.inputNodePtr, &newConn.inputSlot,
                                  &newConn.outputNodePtr,
                                  &newConn.outputSlot)) {
      GraphNode* inputNode = (GraphNode*)newConn.inputNodePtr;
      GraphNode* outputNode = (GraphNode*)newConn.outputNodePtr;
      inputNode->m_connections.push_back(newConn);
      if (outputNode != inputNode)
        outputNode->m_connections.push_back(newConn);
      // Refresh the consumer node since it has a new input
      graph->refreshNode(inputNode);
    }

    // Render existing connections (only output side renders them)
    for (const NodeConnection& conn : m_connections) {
      if (conn.outputNodePtr != this)
        continue;
      if (!ImNodes::Connection(conn.inputNodePtr, conn.inputSlot,
                               conn.outputNodePtr, conn.outputSlot)) {
        GraphNode* inNode = (GraphNode*)conn.inputNodePtr;
        GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
        inNode->deleteConnection(conn);
        outNode->deleteConnection(conn);
        // Refresh since it lost a connection
        graph->refreshNode(inNode);
        break;
      }
    }
  }
}

void GraphNode::deleteAllConnections(std::vector<GraphNode*>& allNodes) {
  for (auto& conn : m_connections) {
    GraphNode* other = nullptr;
    if (conn.outputNodePtr == this) {
      other = (GraphNode*)conn.inputNodePtr;
    } else {
      other = (GraphNode*)conn.outputNodePtr;
    }
    if (other && other != this) {
      other->deleteConnection(conn);
    }
  }
  m_connections.clear();
}

void GraphNode::deleteConnection(const NodeConnection& conn) {
  for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
    if (*it == conn) {
      m_connections.erase(it);
      return;
    }
  }
}

// ============================================================
// NodeGraph
// ============================================================

NodeGraph::NodeGraph() {
  registerAllNodes(m_registry);

  // Create default Input node
  {
    auto node = std::make_unique<InputNode>();
    node->pos = {100.0f, 200.0f};
    GraphNode* gn = new GraphNode(std::move(node), m_nextId++);
    m_nodes.push_back(gn);
  }
  // Create default Output node
  {
    auto node = std::make_unique<OutputNode>();
    node->pos = {500.0f, 200.0f};
    GraphNode* gn = new GraphNode(std::move(node), m_nextId++);
    m_nodes.push_back(gn);
  }
}

NodeGraph::~NodeGraph() {
  clear();
}

void NodeGraph::clear() {
  for (auto* n : m_nodes)
    delete n;
  m_nodes.clear();
  m_nextId = 0;
  m_hasOutput = false;
}

GraphNode* NodeGraph::findNodeByPtr(void* ptr) {
  for (auto* n : m_nodes)
    if (n == (GraphNode*)ptr)
      return n;
  return nullptr;
}

GraphNode* NodeGraph::findNodeById(int id) {
  for (auto* n : m_nodes)
    if (n->texNode()->id == id)
      return n;
  return nullptr;
}

// Kahn's topological sort
std::vector<GraphNode*> NodeGraph::topologicalSort() {
  // Build adjacency: for each connection, outputNode -> inputNode (data flows
  // from output to input)
  std::unordered_map<GraphNode*, int> inDegree;
  std::unordered_map<GraphNode*, std::vector<GraphNode*>>
      adj;  // adj[out] -> [in]

  for (auto* n : m_nodes) {
    inDegree[n] = 0;
    adj[n] = {};
  }

  for (auto* n : m_nodes) {
    for (const auto& conn : n->connections()) {
      // connection goes from outputNodePtr -> inputNodePtr (data flow)
      GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
      GraphNode* inNode = (GraphNode*)conn.inputNodePtr;
      // outNode produces data consumed by inNode
      // inNode depends on outNode, so outNode must come first
      if (outNode == n) {
        // Only count each edge once (from the output side)
        adj[outNode].push_back(inNode);
        inDegree[inNode]++;
      }
    }
  }

  std::queue<GraphNode*> q;
  for (auto* n : m_nodes)
    if (inDegree[n] == 0)
      q.push(n);

  std::vector<GraphNode*> sorted;
  while (!q.empty()) {
    GraphNode* cur = q.front();
    q.pop();
    sorted.push_back(cur);
    for (auto* next : adj[cur]) {
      if (--inDegree[next] == 0)
        q.push(next);
    }
  }

  // If cycle detected, append remaining nodes
  if ((int)sorted.size() < (int)m_nodes.size()) {
    for (auto* n : m_nodes) {
      bool found = false;
      for (auto* s : sorted)
        if (s == n) {
          found = true;
          break;
        }
      if (!found)
        sorted.push_back(n);
    }
  }

  return sorted;
}

GenTexture* NodeGraph::getInputImageForSlot(GraphNode* node, int slotIdx) {
  auto slotNames = node->texNode()->inputSlotNames();
  if (slotIdx < 0 || slotIdx >= (int)slotNames.size())
    return nullptr;
  const std::string& wantedSlot = slotNames[slotIdx];

  for (const auto& conn : node->connections()) {
    if (conn.inputNodePtr != node)
      continue;
    if (conn.inputSlot == nullptr)
      continue;
    if (strcmp(conn.inputSlot, wantedSlot.c_str()) != 0)
      continue;

    // Found the connection - get output from the providing node
    GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
    if (!outNode)
      continue;

    // Find which output slot index this corresponds to
    auto outNames = outNode->texNode()->outputSlotNames();
    for (int i = 0; i < (int)outNames.size(); i++) {
      if (conn.outputSlot &&
          strcmp(conn.outputSlot, outNames[i].c_str()) == 0) {
        if (i < (int)outNode->cachedOutputs.size()) {
          return &outNode->cachedOutputs[i];
        }
      }
    }
  }
  return nullptr;
}

static void refreshSingleNode(NodeGraph* graph, GraphNode* node) {
  TextureNode* tn = node->texNode();
  auto inNames = tn->inputSlotNames();
  auto outNames = tn->outputSlotNames();

  // Gather inputs from upstream cached outputs
  std::vector<GenTexture*> inputs(inNames.size(), nullptr);
  for (int i = 0; i < (int)inNames.size(); i++)
    inputs[i] = graph->getInputImageForSlot(node, i);

  // Execute
  node->cachedOutputs.clear();
  node->cachedOutputs.resize(outNames.size());
  tn->execute(inputs, node->cachedOutputs);
  node->executed = true;

  // Find best image for preview: first output, or first input for sink nodes
  GenTexture* previewSrc = nullptr;
  if (!node->cachedOutputs.empty() && node->cachedOutputs[0].Data)
    previewSrc = &node->cachedOutputs[0];
  else if (!inputs.empty() && inputs[0] && inputs[0]->Data)
    previewSrc = inputs[0];

  // Update GPU preview texture
  if (previewSrc) {
    if (node->hasPreview && node->previewTex.id != 0)
      UnloadTexture(node->previewTex);
    node->previewTex = LoadTextureFromGenTexture(*previewSrc);
    node->hasPreview = (node->previewTex.id != 0);
  }
}

std::vector<GraphNode*> NodeGraph::getDownstreamNodes(GraphNode* start) {
  // BFS to find all nodes reachable downstream from 'start'
  std::unordered_set<GraphNode*> visited;
  std::queue<GraphNode*> bfs;

  // Seed with direct consumers of 'start'
  for (const auto& conn : start->connections()) {
    if (conn.outputNodePtr == start) {
      GraphNode* downstream = (GraphNode*)conn.inputNodePtr;
      if (downstream && downstream != start &&
          visited.find(downstream) == visited.end()) {
        visited.insert(downstream);
        bfs.push(downstream);
      }
    }
  }

  while (!bfs.empty()) {
    GraphNode* cur = bfs.front();
    bfs.pop();
    for (const auto& conn : cur->connections()) {
      if (conn.outputNodePtr == cur) {
        GraphNode* downstream = (GraphNode*)conn.inputNodePtr;
        if (downstream && downstream != cur &&
            visited.find(downstream) == visited.end()) {
          visited.insert(downstream);
          bfs.push(downstream);
        }
      }
    }
  }

  // Return them in topological order
  auto sorted = topologicalSort();
  std::vector<GraphNode*> result;
  for (auto* n : sorted) {
    if (visited.find(n) != visited.end())
      result.push_back(n);
  }
  return result;
}

void NodeGraph::refreshNode(GraphNode* node) {
  // Refresh the node itself
  refreshSingleNode(this, node);

  // Cascade: refresh all downstream nodes in topological order
  auto downstream = getDownstreamNodes(node);
  for (auto* dn : downstream) {
    refreshSingleNode(this, dn);
  }
}

void NodeGraph::resetNodeParams(GraphNode* node) {
  TextureNode* tn = node->texNode();
  std::string type = tn->typeName();
  auto it = m_registry.find(type);
  if (it == m_registry.end())
    return;

  // Create a temporary node with default params
  auto defaultNode = it->second();
  nlohmann::json defaultParams = defaultNode->saveParams();
  tn->loadParams(defaultParams);

  // Trigger a cascade refresh
  refreshNode(node);
}

void NodeGraph::generate() {
  // Reset executed flag
  for (auto* n : m_nodes) {
    n->executed = false;
    n->cachedOutputs.clear();
  }

  auto sorted = topologicalSort();

  for (auto* gn : sorted) {
    TextureNode* tn = gn->texNode();
    auto inNames = tn->inputSlotNames();
    auto outNames = tn->outputSlotNames();

    // Build inputs vector
    std::vector<GenTexture*> inputs(inNames.size(), nullptr);
    for (int i = 0; i < (int)inNames.size(); i++) {
      inputs[i] = getInputImageForSlot(gn, i);
    }

    // Execute
    gn->cachedOutputs.resize(outNames.size());
    tn->execute(inputs, gn->cachedOutputs);
    gn->executed = true;

    // If this is an Output node, store the last output for preview
    if (tn->typeName() == "Output" && !inputs.empty() && inputs[0] &&
        inputs[0]->Data) {
      m_lastOutput = *inputs[0];
      m_hasOutput = true;
    }

    // Update GPU texture for preview
    if (!gn->cachedOutputs.empty() && gn->cachedOutputs[0].Data) {
      if (gn->hasPreview && gn->previewTex.id != 0) {
        UnloadTexture(gn->previewTex);
      }
      gn->previewTex = LoadTextureFromGenTexture(gn->cachedOutputs[0]);
      gn->hasPreview = (gn->previewTex.id != 0);
    }
  }
}

GenTexture* NodeGraph::getLastOutput() {
  return m_hasOutput ? &m_lastOutput : nullptr;
}

nlohmann::json NodeGraph::save() const {
  nlohmann::json j;

  // Nodes
  nlohmann::json nodesArr = nlohmann::json::array();
  for (auto* gn : m_nodes) {
    TextureNode* tn = gn->texNode();
    nlohmann::json nj;
    nj["id"] = tn->id;
    nj["typeName"] = tn->typeName();
    nj["posX"] = tn->pos.x;
    nj["posY"] = tn->pos.y;
    nj["params"] = tn->saveParams();
    nodesArr.push_back(nj);
  }
  j["nodes"] = nodesArr;

  // Connections (collect unique ones from all nodes)
  nlohmann::json connsArr = nlohmann::json::array();
  for (auto* gn : m_nodes) {
    for (const auto& conn : gn->connections()) {
      if (conn.outputNodePtr != gn)
        continue;  // only once per connection
      GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
      GraphNode* inNode = (GraphNode*)conn.inputNodePtr;
      if (!outNode || !inNode)
        continue;
      nlohmann::json cj;
      cj["fromId"] = outNode->texNode()->id;
      cj["fromSlot"] = conn.outputSlot ? conn.outputSlot : "";
      cj["toId"] = inNode->texNode()->id;
      cj["toSlot"] = conn.inputSlot ? conn.inputSlot : "";
      connsArr.push_back(cj);
    }
  }
  j["connections"] = connsArr;

  return j;
}

void NodeGraph::load(const nlohmann::json& j) {
  clear();

  if (!j.contains("nodes"))
    return;

  for (const auto& nj : j["nodes"]) {
    std::string typeName = nj["typeName"];
    auto it = m_registry.find(typeName);
    if (it == m_registry.end())
      continue;

    auto texNode = it->second();
    texNode->pos.x = nj.value("posX", 100.0f);
    texNode->pos.y = nj.value("posY", 100.0f);
    if (nj.contains("params"))
      texNode->loadParams(nj["params"]);

    int id = nj.value("id", m_nextId);
    if (id >= m_nextId)
      m_nextId = id + 1;
    texNode->id = id;

    GraphNode* gn = new GraphNode(std::move(texNode), id);
    m_nodes.push_back(gn);
  }

  // Rebuild connections
  if (!j.contains("connections"))
    return;

  for (const auto& cj : j["connections"]) {
    int fromId = cj.value("fromId", -1);
    int toId = cj.value("toId", -1);
    std::string fromSlotStr = cj.value("fromSlot", "");
    std::string toSlotStr = cj.value("toSlot", "");

    GraphNode* fromNode = findNodeById(fromId);
    GraphNode* toNode = findNodeById(toId);
    if (!fromNode || !toNode)
      continue;

    // Find stable slot name pointers
    const char* fromSlotPtr = nullptr;
    const char* toSlotPtr = nullptr;

    auto outNames = fromNode->texNode()->outputSlotNames();
    auto outSlots = fromNode->texNode()->outputSlotInfos();
    for (int i = 0; i < (int)outSlots.size(); i++) {
      if (outSlots[i].title && fromSlotStr == outSlots[i].title) {
        fromSlotPtr = outSlots[i].title;
        break;
      }
    }

    auto inNames = toNode->texNode()->inputSlotNames();
    auto inSlots = toNode->texNode()->inputSlotInfos();
    for (int i = 0; i < (int)inSlots.size(); i++) {
      if (inSlots[i].title && toSlotStr == inSlots[i].title) {
        toSlotPtr = inSlots[i].title;
        break;
      }
    }

    if (!fromSlotPtr || !toSlotPtr)
      continue;

    NodeConnection conn;
    conn.outputNodePtr = fromNode;
    conn.outputSlot = fromSlotPtr;
    conn.inputNodePtr = toNode;
    conn.inputSlot = toSlotPtr;

    fromNode->connections().push_back(conn);
    toNode->connections().push_back(conn);
  }
}

void NodeGraph::draw() {
  static ImNodes::Ez::Context* context = nullptr;
  if (!context)
    context = ImNodes::Ez::CreateContext();

  ImGui::BeginGroup();
  ImNodes::Ez::BeginCanvas();

  for (auto it = m_nodes.begin(); it != m_nodes.end();) {
    GraphNode* gn = *it;
    gn->draw(this);
    ImNodes::Ez::EndNode();

    if (gn->isSelected() && ImGui::IsKeyPressed(ImGuiKey_Delete) &&
        ImGui::IsWindowFocused()) {
      gn->deleteAllConnections(m_nodes);
      delete gn;
      it = m_nodes.erase(it);
    } else {
      ++it;
    }
  }

  // Right-click context menu
  if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered() &&
      !ImGui::IsMouseDragging(1)) {
    ImGui::FocusWindow(ImGui::GetCurrentWindow());
    ImGui::OpenPopup("NodeGraphContextMenu");
  }

  if (ImGui::BeginPopup("NodeGraphContextMenu")) {
    for (auto& kv : m_registry) {
      if (ImGui::MenuItem(kv.first.c_str())) {
        auto texNode = kv.second();
        int newId = m_nextId++;
        GraphNode* gn = new GraphNode(std::move(texNode), newId);
        m_nodes.push_back(gn);
        ImNodes::AutoPositionNode(m_nodes.back());
      }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Reset Zoom"))
      ImNodes::GetCurrentCanvas()->Zoom = 1;

    if (ImGui::IsAnyMouseDown() && !ImGui::IsWindowHovered())
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }

  ImNodes::Ez::EndCanvas();
  ImGui::EndGroup();
}

// ============================================================
// createNodeCanvas
// ============================================================

void createNodeCanvas() {
  if (!g_nodeGraph) {
    g_nodeGraph = new NodeGraph();
  }
  g_nodeGraph->draw();
}
