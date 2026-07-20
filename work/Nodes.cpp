#include "Nodes.h"
#include "AggNodes.h"
#include "AllNodes.h"
#include "CoreNodeRegistry.h"
#include "GraphRunner.h"
#include "Icons.h"
#include "MMNodes.h"
#include "StructNodes.h"
#include "Utils.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <raylib.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

// ============================================================
// Global instance
// ============================================================

NodeGraph* g_nodeGraph = nullptr;

// ============================================================
// registerAllNodes
// ============================================================

void registerAllNodes(std::map<std::string, NodeFactory>& registry) {
  // Material Maker ports
  registry["Voronoi"] = []() { return std::make_unique<VoronoiNode>(); };
  registry["FBM"] = []() { return std::make_unique<FBMNode>(); };
  registry["Blend"] = []() { return std::make_unique<BlendNode>(); };
  registry["Warp"] = []() { return std::make_unique<WarpNode>(); };
  registry["Colorize"] = []() { return std::make_unique<ColorizeNode>(); };
  registry["BricksMM"] = []() { return std::make_unique<MMBricksNode>(); };
  registry["Material"] = []() { return std::make_unique<MaterialNode>(); };
  registry["NormalMap"] = []() { return std::make_unique<NormalMapNode>(); };
  registry["SdfShape"] = []() { return std::make_unique<SdfShapeNode>(); };
  registry["SdfOp"] = []() { return std::make_unique<SdfOpNode>(); };
  registry["SdfTransform"] = []() {
    return std::make_unique<SdfTransformNode>();
  };
  registry["SdfShow"] = []() { return std::make_unique<SdfShowNode>(); };
  registry["MakeTileable"] = []() {
    return std::make_unique<MakeTileableNode>();
  };
  registry["Quantize"] = []() { return std::make_unique<QuantizeNode>(); };
  registry["Emboss"] = []() { return std::make_unique<EmbossNode>(); };
  registry["Transform2D"] = []() {
    return std::make_unique<Transform2DNode>();
  };
  registry["Shape"] = []() { return std::make_unique<ShapeNode>(); };
  registry["Pattern"] = []() { return std::make_unique<PatternNode>(); };
  registry["Combine"] = []() { return std::make_unique<CombineNode>(); };
  registry["Decompose"] = []() { return std::make_unique<DecomposeNode>(); };
  registry["Invert"] = []() { return std::make_unique<InvertNode>(); };
  registry["LayerMix"] = []() { return std::make_unique<LayerMixNode>(); };
  registry["WorkflowOutput"] = []() {
    return std::make_unique<WorkflowOutputNode>();
  };
  registry["MathOp"] = []() { return std::make_unique<MathOpNode>(); };
  registry["GradientMM"] = []() { return std::make_unique<GradientMMNode>(); };
  registry["Tiler"] = []() { return std::make_unique<TilerNode>(); };
  registry["MultiWarp"] = []() { return std::make_unique<MultiWarpNode>(); };
  registry["SlopeBlur"] = []() { return std::make_unique<SlopeBlurNode>(); };
  registry["Sphere"] = []() { return std::make_unique<SphereNode>(); };
  registry["AnisotropicNoise"] = []() {
    return std::make_unique<AnisotropicNoiseNode>();
  };
  registry["HeightToOffset"] = []() {
    return std::make_unique<HeightToOffsetNode>();
  };
  registry["Bevel"] = []() { return std::make_unique<BevelNode>(); };
  registry["Dilate"] = []() { return std::make_unique<DilateNode>(); };
  registry["NormalBlend"] = []() {
    return std::make_unique<NormalBlendNode>();
  };
  registry["ColorNoise"] = []() { return std::make_unique<ColorNoiseNode>(); };
  registry["AddTiler"] = []() { return std::make_unique<AddTilerNode>(); };
  registry["AutoTones"] = []() { return std::make_unique<AutoTonesNode>(); };
  registry["Mingle"] = []() { return std::make_unique<MingleNode>(); };
  registry["FillFromColors"] = []() {
    return std::make_unique<FillFromColorsNode>();
  };
  registry["DirectionalWarp"] = []() {
    return std::make_unique<DirectionalWarpNode>();
  };
  registry["WarpDilate"] = []() { return std::make_unique<WarpDilateNode>(); };
  registry["AnisotropicKuwahara"] = []() {
    return std::make_unique<AnisotropicKuwaharaNode>();
  };
  registry["Box"] = []() { return std::make_unique<BoxNode>(); };
  registry["Cairo"] = []() { return std::make_unique<CairoNode>(); };
  registry["ShardFBM"] = []() { return std::make_unique<ShardFBMNode>(); };
  registry["BricksUneven"] = []() {
    return std::make_unique<BricksUnevenNode>();
  };
  registry["CircleSplatter"] = []() {
    return std::make_unique<CircleSplatterNode>();
  };
  registry["WaveletNoise"] = []() {
    return std::make_unique<WaveletNoiseNode>();
  };
  registry["BinarySmooth"] = []() {
    return std::make_unique<BinarySmoothNode>();
  };
  registry["Weave"] = []() { return std::make_unique<WeaveNode>(); };
  registry["Weave2"] = []() { return std::make_unique<Weave2Node>(); };
  registry["EdgeDetect2"] = []() {
    return std::make_unique<EdgeDetect2Node>();
  };
  registry["SmoothMinMax"] = []() {
    return std::make_unique<SmoothMinMaxNode>();
  };
  registry["FillToGradient"] = []() {
    return std::make_unique<FillToGradientNode>();
  };
  registry["FillToSize"] = []() { return std::make_unique<FillToSizeNode>(); };
  registry["DirectionalBlur"] = []() {
    return std::make_unique<DirectionalBlurNode>();
  };
  registry["TilerAdvanced"] = []() {
    return std::make_unique<TilerAdvancedNode>();
  };
  registry["DotNoise"] = []() { return std::make_unique<DotNoiseNode>(); };
  registry["Scratches"] = []() { return std::make_unique<ScratchesNode>(); };
  registry["Mirror"] = []() { return std::make_unique<MirrorNode>(); };
  registry["EdgeDetect"] = []() { return std::make_unique<EdgeDetectNode>(); };
  registry["CreateMap"] = []() { return std::make_unique<CreateMapNode>(); };
  registry["MatMap"] = []() { return std::make_unique<MatMapNode>(); };
  registry["Fill"] = []() { return std::make_unique<FillNode>(); };
  registry["FillToUV"] = []() { return std::make_unique<FillToUVNode>(); };
  registry["FillToRandomGray"] = []() {
    return std::make_unique<FillToRandomGrayNode>();
  };
  registry["FillToRandomColor"] = []() {
    return std::make_unique<FillToRandomColorNode>();
  };
  registry["FillToColor"] = []() {
    return std::make_unique<FillToColorNode>();
  };
  registry["Remap"] = []() { return std::make_unique<RemapNode>(); };
  registry["Levels"] = []() { return std::make_unique<LevelsNode>(); };
  registry["Tile2x2"] = []() { return std::make_unique<Tile2x2Node>(); };
  registry["NormalConvert"] = []() {
    return std::make_unique<NormalConvertNode>();
  };
  registry["CustomUV"] = []() { return std::make_unique<CustomUVNode>(); };
  registry["SmoothCurvature"] = []() {
    return std::make_unique<SmoothCurvatureNode>();
  };
  registry["AmbientOcclusion"] = []() {
    return std::make_unique<AmbientOcclusionNode>();
  };
  // Structural
  registry["Subgraph"] = []() { return std::make_unique<SubgraphNode>(); };
  registry["Remote"] = []() { return std::make_unique<RemoteNode>(); };

  registry["Color"] = []() { return std::make_unique<ColorNode>(); };
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
  registry["Gradient"] = []() { return std::make_unique<GradientNode>(); };
  registry["Image"] = []() { return std::make_unique<ImageNode>(); };
  // AGG vector drawing nodes
  registry["AggLine"] = []() { return std::make_unique<AggLineNode>(); };
  registry["AggCircle"] = []() { return std::make_unique<AggCircleNode>(); };
  registry["AggRect"] = []() { return std::make_unique<AggRectNode>(); };
  registry["AggPolygon"] = []() { return std::make_unique<AggPolygonNode>(); };
  registry["AggText"] = []() { return std::make_unique<AggTextNode>(); };
  registry["AggArc"] = []() { return std::make_unique<AggArcNode>(); };
  registry["AggBezier"] = []() { return std::make_unique<AggBezierNode>(); };
  registry["AggDashLine"] = []() {
    return std::make_unique<AggDashLineNode>();
  };
  registry["AggGradient"] = []() {
    return std::make_unique<AggGradientNode>();
  };
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

    // Collapsible parameters
    {
      ImGui::SetNextItemOpen(paramsOpen);
      std::string paramHeader = "Params##" + std::to_string(tn->id);
      paramsOpen = ImGui::TreeNode(paramHeader.c_str());
      if (paramsOpen) {
        tn->renderParams();

        // Reset button
        std::string resetId = "Reset##" + std::to_string(tn->id);
        if (ImGui::SmallButton(resetId.c_str())) {
          graph->resetNodeParams(this);
          m_lastParamsHash = tn->saveParams().dump();
        }

        // Presets
        ImGui::SameLine();
        std::string presetPopup = "Presets##pop_" + std::to_string(tn->id);
        if (ImGui::SmallButton(
                ("Presets##" + std::to_string(tn->id)).c_str())) {
          ImGui::OpenPopup(presetPopup.c_str());
        }
        if (ImGui::BeginPopup(presetPopup.c_str())) {
          std::string presetDir = "presets/" + tn->typeName();
          // List existing presets
          try {
            if (fs::is_directory(presetDir)) {
              for (auto& entry : fs::directory_iterator(presetDir)) {
                if (entry.path().extension() == ".json") {
                  std::string name = entry.path().stem().string();
                  if (ImGui::MenuItem(name.c_str())) {
                    std::ifstream pf(entry.path());
                    if (pf.is_open()) {
                      nlohmann::json pj;
                      pf >> pj;
                      graph->pushUndo();
                      tn->loadParams(pj);
                      m_lastParamsHash = tn->saveParams().dump();
                      graph->refreshNode(this);
                    }
                  }
                }
              }
            }
          } catch (...) {
          }
          ImGui::Separator();
          // Save current as preset
          static char presetName[128] = "";
          ImGui::SetNextItemWidth(120);
          ImGui::InputText("##pname", presetName, sizeof(presetName));
          ImGui::SameLine();
          if (ImGui::SmallButton("Save##preset")) {
            if (strlen(presetName) > 0) {
              fs::create_directories(presetDir);
              std::string path = presetDir + "/" + presetName + ".json";
              std::ofstream of(path);
              if (of.is_open()) {
                of << tn->saveParams().dump(2);
              }
            }
          }
          ImGui::EndPopup();
        }

        ImGui::TreePop();
      }
    }

    // Detect parameter changes via JSON snapshot
    std::string curParams = tn->saveParams().dump();
    if (curParams != m_lastParamsHash) {
      if (!m_paramsDirty) {
        // First change — save undo before modifying
        graph->pushUndo();
        m_paramsDirty = true;
      }
      m_lastParamsHash = curParams;
      graph->refreshNode(this);
    }
    // Reset dirty flag when mouse is released (end of drag)
    if (m_paramsDirty && !ImGui::IsMouseDown(0)) {
      m_paramsDirty = false;
    }

    // Collapsible preview
    if (hasPreview && previewTex.id != 0) {
      ImGui::SetNextItemOpen(previewOpen);
      std::string prevHeader = "Preview##" + std::to_string(tn->id);
      previewOpen = ImGui::TreeNode(prevHeader.c_str());
      if (previewOpen) {
        float maxSide = 128.0f;
        float bigger =
            (float)((previewTex.width > previewTex.height) ? previewTex.width
                                                           : previewTex.height);
        float scale = (bigger > 0) ? maxSide / bigger : 1.0f;
        if (scale > 1.0f)
          scale = 1.0f;
        ImGui::Image(
            ImTextureID(previewTex.id),
            ImVec2(previewTex.width * scale, previewTex.height * scale));
        ImGui::TreePop();
      }
    }

    auto outSlots = tn->outputSlotInfos();
    ImNodes::Ez::OutputSlots(outSlots.data(), (int)outSlots.size());

    // Handle new connections
    NodeConnection newConn;
    if (ImNodes::GetNewConnection(&newConn.inputNodePtr, &newConn.inputSlot,
                                  &newConn.outputNodePtr,
                                  &newConn.outputSlot)) {
      graph->pushUndo();
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
        graph->pushUndo();
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

  // Create default Color node
  {
    auto node = std::make_unique<ColorNode>();
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

void NodeGraph::pushUndo() {
  m_undoStack.push_back(save());
  if ((int)m_undoStack.size() > MAX_UNDO)
    m_undoStack.erase(m_undoStack.begin());
  m_redoStack.clear();
}

// After load, sync all nodes' param hashes so draw() doesn't see false changes
void NodeGraph::syncParamHashes() {
  for (auto* gn : m_nodes)
    gn->syncParamsHash();
}

void NodeGraph::undo() {
  if (m_undoStack.empty())
    return;
  m_redoStack.push_back(save());
  nlohmann::json state = m_undoStack.back();
  m_undoStack.pop_back();
  load(state);
  generate();
  syncParamHashes();
}

void NodeGraph::redo() {
  if (m_redoStack.empty())
    return;
  m_undoStack.push_back(save());
  nlohmann::json state = m_redoStack.back();
  m_redoStack.pop_back();
  load(state);
  generate();
  syncParamHashes();
}

bool NodeGraph::canUndo() const {
  return !m_undoStack.empty();
}

bool NodeGraph::canRedo() const {
  return !m_redoStack.empty();
}

void NodeGraph::copySelected() {
  m_clipboard = nlohmann::json();

  // Collect selected nodes
  std::set<int> selectedIds;
  nlohmann::json nodesArr = nlohmann::json::array();
  for (auto* gn : m_nodes) {
    if (!gn->isSelected())
      continue;
    TextureNode* tn = gn->texNode();
    selectedIds.insert(tn->id);
    nlohmann::json nj;
    nj["id"] = tn->id;
    nj["typeName"] = tn->typeName();
    nj["posX"] = tn->pos.x;
    nj["posY"] = tn->pos.y;
    nj["params"] = tn->saveParams();
    nj["paramsOpen"] = gn->paramsOpen;
    nj["previewOpen"] = gn->previewOpen;
    nodesArr.push_back(nj);
  }

  // Collect connections between selected nodes only
  nlohmann::json connsArr = nlohmann::json::array();
  for (auto* gn : m_nodes) {
    for (const auto& conn : gn->connections()) {
      if (conn.outputNodePtr != gn)
        continue;
      GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
      GraphNode* inNode = (GraphNode*)conn.inputNodePtr;
      if (!outNode || !inNode)
        continue;
      int fromId = outNode->texNode()->id;
      int toId = inNode->texNode()->id;
      if (selectedIds.count(fromId) && selectedIds.count(toId)) {
        nlohmann::json cj;
        cj["fromId"] = fromId;
        cj["fromSlot"] = conn.outputSlot ? conn.outputSlot : "";
        cj["toId"] = toId;
        cj["toSlot"] = conn.inputSlot ? conn.inputSlot : "";
        connsArr.push_back(cj);
      }
    }
  }

  m_clipboard["nodes"] = nodesArr;
  m_clipboard["connections"] = connsArr;
}

void NodeGraph::pasteClipboard() {
  if (!m_clipboard.contains("nodes") || m_clipboard["nodes"].empty())
    return;

  pushUndo();

  // Deselect all current nodes
  for (auto* gn : m_nodes)
    gn->texNode()->selected = false;

  // Map old IDs to new IDs
  std::map<int, int> idMap;
  float offsetX = 50.0f, offsetY = 50.0f;

  for (const auto& nj : m_clipboard["nodes"]) {
    std::string typeName = nj["typeName"];
    auto it = m_registry.find(typeName);
    if (it == m_registry.end())
      continue;

    int oldId = nj["id"];
    int newId = m_nextId++;
    idMap[oldId] = newId;

    auto texNode = it->second();
    texNode->id = newId;
    texNode->pos.x = nj.value("posX", 100.0f) + offsetX;
    texNode->pos.y = nj.value("posY", 100.0f) + offsetY;
    texNode->selected = true;
    if (nj.contains("params"))
      texNode->loadParams(nj["params"]);

    GraphNode* gn = new GraphNode(std::move(texNode), newId);
    gn->paramsOpen = nj.value("paramsOpen", false);
    gn->previewOpen = nj.value("previewOpen", false);
    m_nodes.push_back(gn);
  }

  // Recreate connections with new IDs
  if (m_clipboard.contains("connections")) {
    for (const auto& cj : m_clipboard["connections"]) {
      int oldFrom = cj["fromId"];
      int oldTo = cj["toId"];
      if (!idMap.count(oldFrom) || !idMap.count(oldTo))
        continue;

      GraphNode* fromNode = findNodeById(idMap[oldFrom]);
      GraphNode* toNode = findNodeById(idMap[oldTo]);
      if (!fromNode || !toNode)
        continue;

      std::string fromSlot = cj.value("fromSlot", "");
      std::string toSlot = cj.value("toSlot", "");

      NodeConnection conn;
      conn.outputNodePtr = fromNode;
      conn.outputSlot = fromSlot.empty()
                            ? nullptr
                            : fromNode->texNode()->outputSlotNames()[0].c_str();
      conn.inputNodePtr = toNode;
      conn.inputSlot = toSlot.empty()
                           ? nullptr
                           : toNode->texNode()->inputSlotNames()[0].c_str();

      // Find correct slot pointers by matching names
      auto outNames = fromNode->texNode()->outputSlotNames();
      for (const auto& name : outNames) {
        if (name == fromSlot) {
          conn.outputSlot = name.c_str();
          break;
        }
      }
      auto inNames = toNode->texNode()->inputSlotNames();
      for (const auto& name : inNames) {
        if (name == toSlot) {
          conn.inputSlot = name.c_str();
          break;
        }
      }

      fromNode->connections().push_back(conn);
      if (toNode != fromNode)
        toNode->connections().push_back(conn);
    }
  }

  // Generate the pasted nodes
  generate();
  syncParamHashes();
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
  std::unordered_map<GraphNode*, int> inDegree;
  std::unordered_map<GraphNode*, std::vector<GraphNode*>> adj;

  for (auto* n : m_nodes) {
    inDegree[n] = 0;
    adj[n] = {};
  }

  for (auto* n : m_nodes) {
    for (const auto& conn : n->connections()) {
      GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
      GraphNode* inNode = (GraphNode*)conn.inputNodePtr;
      if (outNode == n) {
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

    GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
    if (!outNode)
      continue;

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

  std::vector<GenTexture*> inputs(inNames.size(), nullptr);
  for (int i = 0; i < (int)inNames.size(); i++)
    inputs[i] = graph->getInputImageForSlot(node, i);

  node->cachedOutputs.clear();
  node->cachedOutputs.resize(outNames.size());
  tn->execute(inputs, node->cachedOutputs);
  node->executed = true;

  GenTexture* previewSrc = nullptr;
  if (!node->cachedOutputs.empty() && node->cachedOutputs[0].Data)
    previewSrc = &node->cachedOutputs[0];
  else if (!inputs.empty() && inputs[0] && inputs[0]->Data)
    previewSrc = inputs[0];

  if (previewSrc) {
    if (node->hasPreview && node->previewTex.id != 0)
      UnloadTexture(node->previewTex);
    node->previewTex = LoadTextureFromGenTexture(*previewSrc);
    node->hasPreview = (node->previewTex.id != 0);
  }
}

std::vector<GraphNode*> NodeGraph::getDownstreamNodes(GraphNode* start) {
  std::unordered_set<GraphNode*> visited;
  std::queue<GraphNode*> bfs;

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

  auto sorted = topologicalSort();
  std::vector<GraphNode*> result;
  for (auto* n : sorted) {
    if (visited.find(n) != visited.end())
      result.push_back(n);
  }
  return result;
}

void NodeGraph::refreshNode(GraphNode* node) {
  if (m_runner) {
    if (node->texNode()->typeName() == "Remote") {
      // Remote edits patch OTHER nodes' params, so the dirty set is
      // unknown here — let the worker do a full run.
      applyRemoteNode(node, false);
      m_runner->request(save());
    } else {
      // Param-level change: the worker re-runs just this node first,
      // then streams its downstream cone. Structural changes (this is
      // also called on connect/disconnect) fall back to a full run
      // via the worker's topology check.
      m_runner->requestNode(save(), node->texNode()->id, m_autoPropagate);
    }
    m_evaluating = true;
    return;
  }
  // Remote nodes don't process textures: pushing their sliders means
  // patching the target nodes' params and refreshing those instead.
  if (node->texNode()->typeName() == "Remote") {
    applyRemoteNode(node, true);
    return;
  }

  refreshSingleNode(this, node);
  auto downstream = getDownstreamNodes(node);
  for (auto* dn : downstream) {
    refreshSingleNode(this, dn);
  }

  // Update last output if any refreshed node is an OutputNode
  auto updateOutput = [&](GraphNode* gn) {
    if (gn->texNode()->typeName() == "Output") {
      auto inNames = gn->texNode()->inputSlotNames();
      if (!inNames.empty()) {
        GenTexture* in0 = getInputImageForSlot(gn, 0);
        if (in0 && in0->Data) {
          m_lastOutput = *in0;
          m_hasOutput = true;
          m_changeCount++;
        }
      }
    }
  };
  updateOutput(node);
  for (auto* dn : downstream)
    updateOutput(dn);
}

void NodeGraph::resetNodeParams(GraphNode* node) {
  TextureNode* tn = node->texNode();
  std::string type = tn->typeName();
  auto it = m_registry.find(type);
  if (it == m_registry.end())
    return;

  auto defaultNode = it->second();
  nlohmann::json defaultParams = defaultNode->saveParams();
  tn->loadParams(defaultParams);
  refreshNode(node);
}

void NodeGraph::generate() {
  if (m_runner) {
    // evaluate a snapshot on the worker thread; results land in
    // pollRunner() and the previous outputs stay visible meanwhile
    applyRemoteValues();
    m_runner->request(save());
    m_evaluating = true;
    return;
  }
  applyRemoteValues();

  for (auto* n : m_nodes) {
    n->executed = false;
    n->cachedOutputs.clear();
  }

  auto sorted = topologicalSort();

  for (auto* gn : sorted) {
    TextureNode* tn = gn->texNode();
    auto inNames = tn->inputSlotNames();
    auto outNames = tn->outputSlotNames();

    std::vector<GenTexture*> inputs(inNames.size(), nullptr);
    for (int i = 0; i < (int)inNames.size(); i++) {
      inputs[i] = getInputImageForSlot(gn, i);
    }

    gn->cachedOutputs.resize(outNames.size());
    tn->execute(inputs, gn->cachedOutputs);
    gn->executed = true;

    if (tn->typeName() == "Output" && !inputs.empty() && inputs[0] &&
        inputs[0]->Data) {
      m_lastOutput = *inputs[0];
      m_hasOutput = true;
    }

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

  nlohmann::json nodesArr = nlohmann::json::array();
  for (auto* gn : m_nodes) {
    TextureNode* tn = gn->texNode();
    nlohmann::json nj;
    nj["id"] = tn->id;
    nj["typeName"] = tn->typeName();
    nj["posX"] = tn->pos.x;
    nj["posY"] = tn->pos.y;
    nj["params"] = tn->saveParams();
    nj["paramsOpen"] = gn->paramsOpen;
    nj["previewOpen"] = gn->previewOpen;
    nodesArr.push_back(nj);
  }
  j["nodes"] = nodesArr;

  nlohmann::json connsArr = nlohmann::json::array();
  for (auto* gn : m_nodes) {
    for (const auto& conn : gn->connections()) {
      if (conn.outputNodePtr != gn)
        continue;
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
    std::string typeName = nj.value("typeName", std::string());
    auto it = m_registry.find(typeName);
    if (it == m_registry.end())
      continue;

    auto texNode = it->second();
    texNode->pos.x = nj.value("posX", 100.0f);
    texNode->pos.y = nj.value("posY", 100.0f);
    if (nj.contains("params")) {
      // Unexpected value types (old project formats) must not take the
      // editor down — keep that node's defaults instead.
      try {
        texNode->loadParams(nj["params"]);
      } catch (const std::exception& e) {
        fprintf(stderr, "load: bad params for node type '%s' (%s)\n",
                typeName.c_str(), e.what());
      }
    }

    int id = nj.value("id", m_nextId);
    if (id >= m_nextId)
      m_nextId = id + 1;
    texNode->id = id;

    GraphNode* gn = new GraphNode(std::move(texNode), id);
    gn->paramsOpen = nj.value("paramsOpen", false);
    gn->previewOpen = nj.value("previewOpen", false);
    m_nodes.push_back(gn);
  }

  // without this, the first draw() after a plain file load sees an empty
  // m_lastParamsHash on every node and mistakes it for a param edit —
  // pushing a spurious undo entry and rendering the whole graph (undo()/
  // redo()/pasteClipboard() already do this after their own load() call)
  syncParamHashes();

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

    const char* fromSlotPtr = nullptr;
    const char* toSlotPtr = nullptr;

    auto outSlots = fromNode->texNode()->outputSlotInfos();
    for (int i = 0; i < (int)outSlots.size(); i++) {
      if (outSlots[i].title && fromSlotStr == outSlots[i].title) {
        fromSlotPtr = outSlots[i].title;
        break;
      }
    }

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

void NodeGraph::loadRenderCache(const std::string& dir) {
  if (!fs::exists(dir))
    return;

  std::string outPath = dir + "/output.png";
  if (fs::exists(outPath) && LoadGenTextureFromFile(outPath, m_lastOutput)) {
    m_hasOutput = true;
    m_changeCount++;
  }

  for (auto* gn : m_nodes) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/%03d_%s.png", gn->texNode()->id,
             gn->texNode()->typeName().c_str());
    std::string nodePath = dir + fname;
    if (!fs::exists(nodePath))
      continue;
    GenTexture tex;
    if (!LoadGenTextureFromFile(nodePath, tex))
      continue;
    auto outNames = gn->texNode()->outputSlotNames();
    gn->cachedOutputs.assign(outNames.empty() ? 1 : outNames.size(),
                             GenTexture{});
    gn->cachedOutputs[0] = std::move(tex);
    // multi-output nodes (Subgraph, Voronoi, Decompose, ...) cache every
    // slot beyond the first as its own "..._sN.png" — restore those too,
    // else e.g. a Subgraph's Normal/AO/Roughness outputs stay empty until
    // the user forces a full re-generate
    for (size_t slot = 1; slot < gn->cachedOutputs.size(); slot++) {
      char slotFname[80];
      snprintf(slotFname, sizeof(slotFname), "/%03d_%s_s%zu.png",
               gn->texNode()->id, gn->texNode()->typeName().c_str(), slot);
      std::string slotPath = dir + slotFname;
      if (!fs::exists(slotPath))
        continue;
      GenTexture slotTex;
      if (LoadGenTextureFromFile(slotPath, slotTex))
        gn->cachedOutputs[slot] = std::move(slotTex);
    }
    gn->executed = true;
    if (gn->hasPreview && gn->previewTex.id != 0)
      UnloadTexture(gn->previewTex);
    gn->previewTex = LoadTextureFromGenTexture(gn->cachedOutputs[0]);
    gn->hasPreview = (gn->previewTex.id != 0);
  }
}

// ============================================================
// draw - ImNodes canvas with z-order overlap fix
// ============================================================

// Check if a point is inside a rect
static bool rectContains(ImVec2 rmin, ImVec2 rmax, ImVec2 p) {
  return p.x >= rmin.x && p.x <= rmax.x && p.y >= rmin.y && p.y <= rmax.y;
}

void NodeGraph::pollRunner() {
  if (!m_runner)
    return;

  // Per-node results streamed by incremental (cascade) runs — applied
  // as they land so the user watches the change ripple through.
  std::vector<GraphRunner::NodeResult> stream;
  m_runner->pollStream(stream);
  for (auto& nr : stream) {
    for (auto* gn : m_nodes) {
      if (gn->texNode()->id != nr.nodeId)
        continue;
      gn->cachedOutputs = std::move(nr.outputs);
      gn->executed = true;
      if (!gn->cachedOutputs.empty() && gn->cachedOutputs[0].Data) {
        if (gn->hasPreview && gn->previewTex.id != 0)
          UnloadTexture(gn->previewTex);
        gn->previewTex = LoadTextureFromGenTexture(gn->cachedOutputs[0]);
        gn->hasPreview = (gn->previewTex.id != 0);
      }
      break;
    }
  }

  GraphRunner::Result res;
  if (!m_runner->poll(res)) {
    m_evaluating = m_runner->busy();
    return;
  }
  m_evaluating = m_runner->busy();
  if (!res.ok)
    return;

  // res.outputs is only populated by a full rebuild (the incremental
  // cascade path streams per-node results via pollStream() instead) —
  // a non-empty map here means every node's output is fresh.
  if (!res.outputs.empty())
    m_fullRunCount++;

  for (auto* gn : m_nodes) {
    auto it = res.outputs.find(gn->texNode()->id);
    if (it == res.outputs.end())
      continue;
    gn->cachedOutputs = std::move(it->second);
    gn->executed = true;
    if (!gn->cachedOutputs.empty() && gn->cachedOutputs[0].Data) {
      if (gn->hasPreview && gn->previewTex.id != 0)
        UnloadTexture(gn->previewTex);
      gn->previewTex = LoadTextureFromGenTexture(gn->cachedOutputs[0]);
      gn->hasPreview = (gn->previewTex.id != 0);
    }
  }
  if (res.hasFinal) {
    m_lastOutput = std::move(res.finalOutput);
    m_hasOutput = true;
    m_changeCount++;
    // keep the Output node behavior of writing its file
    for (auto* gn : m_nodes) {
      if (gn->texNode()->typeName() == "Output") {
        std::string fn =
            gn->texNode()->saveParams().value("filename", std::string());
        if (!fn.empty())
          SaveImage(m_lastOutput, fn.c_str());
        break;
      }
    }
  }
}

void NodeGraph::draw() {
  static ImNodes::Ez::Context* context = nullptr;
  if (!context)
    context = ImNodes::Ez::CreateContext();

  pollRunner();

  ImGui::BeginGroup();
  ImNodes::Ez::BeginCanvas();

  ImVec2 mousePos = ImGui::GetMousePos();
  GraphNode* bringToFront = nullptr;
  GraphNode* hoveredNode = nullptr;

  for (int idx = 0; idx < (int)m_nodes.size(); idx++) {
    GraphNode* gn = m_nodes[idx];

    // Check if any node drawn AFTER this one (higher z-order) covers the
    // mouse position AND this node also covers it. If so, this node's
    // widgets should be disabled since a higher-z node is on top.
    bool blocked = false;
    bool thisCovers = rectContains(gn->lastRectMin, gn->lastRectMax, mousePos);
    if (thisCovers) {
      for (int j = idx + 1; j < (int)m_nodes.size(); j++) {
        if (rectContains(m_nodes[j]->lastRectMin, m_nodes[j]->lastRectMax,
                         mousePos)) {
          blocked = true;
          break;
        }
      }
    }

    if (thisCovers && !blocked)
      hoveredNode = gn;

    if (blocked)
      ImGui::BeginDisabled();

    gn->draw(this);
    ImNodes::Ez::EndNode();

    // Save node rect for next frame overlap detection
    gn->lastRectMin = ImGui::GetItemRectMin();
    gn->lastRectMax = ImGui::GetItemRectMax();

    // Bring clicked node to front
    if (!blocked && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
      bringToFront = gn;

    if (blocked)
      ImGui::EndDisabled();
  }

  // Contextual hint for the status bar
  {
    int selCount = 0;
    for (auto* gn : m_nodes)
      if (gn->isSelected())
        selCount++;

    if (hoveredNode) {
      TextureNode* tn = hoveredNode->texNode();
      const NodeMeta* meta = getNodeMeta(tn->typeName());
      m_hintText = tn->displayTitle();
      if (meta) {
        m_hintText += "  [";
        m_hintText += meta->category;
        m_hintText += "]  ";
        m_hintText += meta->description;
      }
    } else if (selCount > 0) {
      m_hintText = std::to_string(selCount) +
                   " selected  |  Ctrl+C/V copy/paste  |  Del delete  |  "
                   "RMB group/ungroup";
    } else {
      m_hintText =
          "RMB add node  |  P previews  |  Ctrl+Z/Y undo/redo  |  "
          "sliders: click+arrows step (Shift fine), Ctrl+click types  |  " +
          std::to_string(m_nodes.size()) + " nodes";
    }
    if (m_evaluating)
      m_hintText = "[evaluating...]  " + m_hintText;
  }

  // Undo/Redo shortcuts
  if (ImGui::IsWindowFocused()) {
    bool ctrl = ImGui::GetIO().KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z))
      undo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y))
      redo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C))
      copySelected();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V))
      pasteClipboard();
    // P: toggle every node preview (chain tracing)
    if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_P) &&
        !ImGui::GetIO().WantTextInput) {
      bool anyClosed = false;
      for (auto* gn : m_nodes)
        if (!gn->previewOpen)
          anyClosed = true;
      for (auto* gn : m_nodes)
        gn->previewOpen = anyClosed;
    }
  }

  // Delete selected nodes
  for (auto it = m_nodes.begin(); it != m_nodes.end();) {
    GraphNode* gn = *it;
    if (gn->isSelected() && ImGui::IsKeyPressed(ImGuiKey_Delete) &&
        ImGui::IsWindowFocused()) {
      pushUndo();
      gn->deleteAllConnections(m_nodes);
      delete gn;
      it = m_nodes.erase(it);
    } else {
      ++it;
    }
  }

  // Move clicked node to end of draw order (top of z-order)
  if (bringToFront) {
    auto it = std::find(m_nodes.begin(), m_nodes.end(), bringToFront);
    if (it != m_nodes.end()) {
      m_nodes.erase(it);
      m_nodes.push_back(bringToFront);
    }
  }

  // Right-click context menu
  if (ImGui::IsMouseReleased(1) && ImGui::IsWindowHovered() &&
      !ImGui::IsMouseDragging(1)) {
    ImGui::FocusWindow(ImGui::GetCurrentWindow());
    ImGui::OpenPopup("NodeGraphContextMenu");
  }

  if (ImGui::BeginPopup("NodeGraphContextMenu")) {
    static char nodeSearch[64] = "";
    if (ImGui::IsWindowAppearing()) {
      nodeSearch[0] = '\0';
      ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(200.0f);
    bool searchAccepted = ImGui::InputTextWithHint(
        "##node_search", "Search nodes...", nodeSearch, sizeof(nodeSearch),
        ImGuiInputTextFlags_EnterReturnsTrue);

    auto containsCI = [](const char* hay, const char* needle) {
      if (!hay)
        return false;
      size_t nlen = strlen(needle);
      for (const char* p = hay; *p; p++) {
        size_t i = 0;
        while (i < nlen && p[i] && tolower(p[i]) == tolower(needle[i]))
          i++;
        if (i == nlen)
          return true;
      }
      return false;
    };

    auto addNode = [&](const std::string& type) {
      pushUndo();
      auto texNode = m_registry[type]();
      int newId = m_nextId++;
      GraphNode* gn = new GraphNode(std::move(texNode), newId);
      m_nodes.push_back(gn);
      ImNodes::AutoPositionNode(m_nodes.back());
    };

    auto drawIcon = [](const char* category) {
      float side = ImGui::GetFontSize();
      if (Texture2D* tex = categoryIcon(category))
        ImGui::Image(ImTextureID(tex->id), ImVec2(side, side));
      else
        ImGui::Dummy(ImVec2(side, side));
      ImGui::SameLine();
    };

    auto menuEntry = [&](const std::string& type, const NodeMeta* meta,
                         bool isFirstMatch) {
      drawIcon(meta ? meta->category : "");
      if (ImGui::MenuItem(type.c_str(), isFirstMatch ? "Enter" : nullptr))
        addNode(type);
      if (meta && ImGui::IsItemHovered())
        ImGui::SetTooltip("[%s] %s", meta->category, meta->description);
    };

    if (nodeSearch[0] == '\0') {
      // Category submenus
      for (const char* cat : nodeCategoryOrder()) {
        std::vector<std::pair<std::string, const NodeMeta*>> entries;
        for (auto& kv : m_registry) {
          const NodeMeta* meta = getNodeMeta(kv.first);
          if (meta && strcmp(meta->category, cat) == 0)
            entries.push_back({kv.first, meta});
        }
        if (entries.empty())
          continue;
        drawIcon(cat);
        if (ImGui::BeginMenu(cat)) {
          for (auto& e : entries)
            menuEntry(e.first, e.second, false);
          ImGui::EndMenu();
        }
      }
      // Types missing from the catalog still need to be reachable
      {
        std::vector<std::string> other;
        for (auto& kv : m_registry)
          if (!getNodeMeta(kv.first))
            other.push_back(kv.first);
        if (!other.empty() && ImGui::BeginMenu("Other")) {
          for (auto& t : other)
            menuEntry(t, nullptr, false);
          ImGui::EndMenu();
        }
      }
    } else {
      // Flat list filtered by name, category or description; name matches
      // rank above the rest so Enter picks the intuitive node
      std::vector<std::pair<std::string, const NodeMeta*>> matches;
      size_t nameMatches = 0;
      for (auto& kv : m_registry) {
        const NodeMeta* meta = getNodeMeta(kv.first);
        if (containsCI(kv.first.c_str(), nodeSearch)) {
          matches.insert(matches.begin() + nameMatches, {kv.first, meta});
          nameMatches++;
        } else if (meta && (containsCI(meta->category, nodeSearch) ||
                            containsCI(meta->description, nodeSearch))) {
          matches.push_back({kv.first, meta});
        }
      }
      for (size_t i = 0; i < matches.size(); i++)
        menuEntry(matches[i].first, matches[i].second, i == 0);
      if (matches.empty())
        ImGui::TextDisabled("No match");
      else if (searchAccepted) {
        addNode(matches[0].first);
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Group Selected"))
      groupSelected();
    if (ImGui::MenuItem("Ungroup Selected"))
      ungroupSelected();

    ImGui::Separator();

    if (ImGui::MenuItem("Show All Previews", "P")) {
      for (auto* gn : m_nodes)
        gn->previewOpen = true;
    }
    if (ImGui::MenuItem("Hide All Previews", "P")) {
      for (auto* gn : m_nodes)
        gn->previewOpen = false;
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Auto Propagate Changes", nullptr, m_autoPropagate))
      m_autoPropagate = !m_autoPropagate;
    if (ImGui::MenuItem("Propagate Now", nullptr, false, m_runner != nullptr))
      generate();

    ImGui::Separator();

    if (ImGui::MenuItem("Reset Zoom"))
      ImNodes::GetCurrentCanvas()->Zoom = 1;

    ImGui::EndPopup();
  }

  // Evaluation overlay: animated spinner + node progress (top-right)
  if (m_evaluating && m_runner) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    const float pillW = 190.0f, pillH = 34.0f, margin = 8.0f;
    ImVec2 p0(winPos.x + winSize.x - pillW - margin, winPos.y + 34.0f);
    ImVec2 p1(p0.x + pillW, p0.y + pillH);

    dl->AddRectFilled(p0, p1, IM_COL32(22, 24, 30, 235), pillH * 0.5f);
    dl->AddRect(p0, p1, IM_COL32(90, 140, 220, 180), pillH * 0.5f, 0, 1.5f);

    // spinner: 3/4 arc rotating with time
    const float t = (float)ImGui::GetTime();
    ImVec2 c(p0.x + 19.0f, p0.y + pillH * 0.5f);
    const float start = t * 5.0f;
    dl->PathClear();
    dl->PathArcTo(c, 8.0f, start, start + 4.7f, 20);
    dl->PathStroke(IM_COL32(120, 180, 255, 255), 0, 3.0f);

    // label with real progress when available
    int done = m_runner->progressDone();
    int total = m_runner->progressTotal();
    char label[64];
    if (total > 0)
      snprintf(label, sizeof(label), "Evaluating %d/%d", done, total);
    else
      snprintf(label, sizeof(label), "Evaluating...");
    dl->AddText(ImVec2(c.x + 15.0f, p0.y + 5.0f), IM_COL32(225, 228, 235, 255),
                label);

    // thin progress bar along the pill bottom
    if (total > 0) {
      const float barX0 = c.x + 15.0f;
      const float barX1 = p1.x - 12.0f;
      const float barY = p1.y - 9.0f;
      dl->AddRectFilled(ImVec2(barX0, barY), ImVec2(barX1, barY + 3.0f),
                        IM_COL32(60, 65, 80, 255), 1.5f);
      const float f = (float)done / (float)total;
      dl->AddRectFilled(ImVec2(barX0, barY),
                        ImVec2(barX0 + (barX1 - barX0) * f, barY + 3.0f),
                        IM_COL32(120, 180, 255, 255), 1.5f);
    }
  }

  // Reminder that param edits are NOT cascading right now
  if (!m_autoPropagate) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    const char* txt = "auto propagate off";
    ImVec2 ts = ImGui::CalcTextSize(txt);
    const float pad = 7.0f, margin = 8.0f;
    ImVec2 p0(winPos.x + winSize.x - ts.x - pad * 2.0f - margin,
              winPos.y + 34.0f + (m_evaluating && m_runner ? 40.0f : 0.0f));
    ImVec2 p1(p0.x + ts.x + pad * 2.0f, p0.y + ts.y + 8.0f);
    dl->AddRectFilled(p0, p1, IM_COL32(45, 36, 16, 225), (p1.y - p0.y) * 0.5f);
    dl->AddRect(p0, p1, IM_COL32(220, 170, 60, 160), (p1.y - p0.y) * 0.5f, 0,
                1.2f);
    dl->AddText(ImVec2(p0.x + pad, p0.y + 4.0f), IM_COL32(235, 200, 120, 255),
                txt);
  }

  // Minimap overlay
  {
    auto* canvas = ImNodes::GetCurrentCanvas();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    float mapW = 180.0f, mapH = 130.0f;
    float margin = 8.0f;
    ImVec2 mapPos(winPos.x + winSize.x - mapW - margin,
                  winPos.y + winSize.y - mapH - margin);

    // Compute bounding box of all nodes in canvas space
    if (!m_nodes.empty()) {
      float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
      for (auto* gn : m_nodes) {
        ImVec2 p = gn->texNode()->pos;
        if (p.x < minX)
          minX = p.x;
        if (p.y < minY)
          minY = p.y;
        if (p.x > maxX)
          maxX = p.x;
        if (p.y > maxY)
          maxY = p.y;
      }
      // Add padding
      float padX = (maxX - minX) * 0.1f + 100.0f;
      float padY = (maxY - minY) * 0.1f + 100.0f;
      minX -= padX;
      minY -= padY;
      maxX += padX;
      maxY += padY;
      float rangeX = maxX - minX;
      float rangeY = maxY - minY;
      if (rangeX < 1.0f)
        rangeX = 1.0f;
      if (rangeY < 1.0f)
        rangeY = 1.0f;

      // Fit aspect ratio
      float scaleX = mapW / rangeX;
      float scaleY = mapH / rangeY;
      float scale = (scaleX < scaleY) ? scaleX : scaleY;

      float cx = mapPos.x + mapW * 0.5f;
      float cy = mapPos.y + mapH * 0.5f;
      float midX = (minX + maxX) * 0.5f;
      float midY = (minY + maxY) * 0.5f;

      ImDrawList* dl = ImGui::GetWindowDrawList();

      // Background
      dl->AddRectFilled(mapPos, ImVec2(mapPos.x + mapW, mapPos.y + mapH),
                        IM_COL32(20, 20, 20, 180), 4.0f);
      dl->AddRect(mapPos, ImVec2(mapPos.x + mapW, mapPos.y + mapH),
                  IM_COL32(80, 80, 80, 200), 4.0f);

      // Draw nodes as small rectangles
      for (auto* gn : m_nodes) {
        ImVec2 p = gn->texNode()->pos;
        float nx = cx + (p.x - midX) * scale;
        float ny = cy + (p.y - midY) * scale;
        float nw = 8.0f, nh = 6.0f;
        ImU32 col = gn->isSelected() ? IM_COL32(255, 200, 50, 220)
                                     : IM_COL32(100, 150, 200, 180);
        dl->AddRectFilled(ImVec2(nx - nw * 0.5f, ny - nh * 0.5f),
                          ImVec2(nx + nw * 0.5f, ny + nh * 0.5f), col, 2.0f);
      }

      // Draw viewport rectangle and handle click-drag navigation
      if (canvas) {
        float zoom = canvas->Zoom;
        ImVec2 off = canvas->Offset;
        float vpX = -off.x / zoom;
        float vpY = -off.y / zoom;
        float vpW = winSize.x / zoom;
        float vpH = winSize.y / zoom;

        float vx1 = cx + (vpX - midX) * scale;
        float vy1 = cy + (vpY - midY) * scale;
        float vx2 = cx + (vpX + vpW - midX) * scale;
        float vy2 = cy + (vpY + vpH - midY) * scale;

        dl->AddRect(ImVec2(vx1, vy1), ImVec2(vx2, vy2),
                    IM_COL32(255, 255, 255, 120), 2.0f);

        // Click or drag on minimap to pan the canvas
        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool inMap = mouse.x >= mapPos.x && mouse.x <= mapPos.x + mapW &&
                     mouse.y >= mapPos.y && mouse.y <= mapPos.y + mapH;
        if (inMap && (ImGui::IsMouseClicked(0) || ImGui::IsMouseDragging(0))) {
          // Convert minimap screen position to canvas coords
          float canvasX = midX + (mouse.x - cx) / scale;
          float canvasY = midY + (mouse.y - cy) / scale;
          // Center the viewport on that canvas position
          canvas->Offset.x = -(canvasX - vpW * 0.5f) * zoom;
          canvas->Offset.y = -(canvasY - vpH * 0.5f) * zoom;
        }
      }
    }
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

// ============================================================
// Subgraphs and Remote parameters
// ============================================================

bool NodeGraph::makeConnection(GraphNode* from,
                               const std::string& fromSlot,
                               GraphNode* to,
                               const std::string& toSlot) {
  const char* fromPtr = nullptr;
  const char* toPtr = nullptr;
  auto outSlots = from->texNode()->outputSlotInfos();
  for (auto& s : outSlots)
    if (s.title && fromSlot == s.title) {
      fromPtr = s.title;
      break;
    }
  auto inSlots = to->texNode()->inputSlotInfos();
  for (auto& s : inSlots)
    if (s.title && toSlot == s.title) {
      toPtr = s.title;
      break;
    }
  if (!fromPtr || !toPtr)
    return false;
  NodeConnection conn;
  conn.outputNodePtr = from;
  conn.outputSlot = fromPtr;
  conn.inputNodePtr = to;
  conn.inputSlot = toPtr;
  from->connections().push_back(conn);
  to->connections().push_back(conn);
  return true;
}

void NodeGraph::applyRemoteNode(GraphNode* remote, bool refreshTargets) {
  nlohmann::json p = remote->texNode()->saveParams();
  for (auto& l : p.value("links", nlohmann::json::array())) {
    GraphNode* target = findNodeById(l.value("nodeId", -1));
    std::string key = l.value("param", std::string());
    if (!target || key.empty() || target == remote)
      continue;
    nlohmann::json patch;
    patch[key] = l.value("value", 0.0f);
    target->texNode()->loadParams(patch);
    if (refreshTargets)
      refreshNode(target);
    else
      target->syncParamsHash();
  }
}

void NodeGraph::applyRemoteValues() {
  for (auto* gn : m_nodes)
    if (gn->texNode()->typeName() == "Remote")
      applyRemoteNode(gn, false);
}

void NodeGraph::groupSelected() {
  std::vector<GraphNode*> sel;
  for (auto* gn : m_nodes)
    if (gn->isSelected())
      sel.push_back(gn);
  if (sel.size() < 2)
    return;

  pushUndo();

  std::set<int> selIds;
  float cx = 0, cy = 0;
  for (auto* gn : sel) {
    selIds.insert(gn->texNode()->id);
    cx += gn->texNode()->pos.x;
    cy += gn->texNode()->pos.y;
  }
  cx /= sel.size();
  cy /= sel.size();

  // Inner graph JSON (ids preserved — they are internal to the subgraph)
  nlohmann::json nodesArr = nlohmann::json::array();
  for (auto* gn : sel) {
    TextureNode* tn = gn->texNode();
    nodesArr.push_back({{"id", tn->id},
                        {"typeName", tn->typeName()},
                        {"posX", tn->pos.x},
                        {"posY", tn->pos.y},
                        {"params", tn->saveParams()}});
  }
  nlohmann::json connsArr = nlohmann::json::array();

  // Boundary analysis. Each connection appears in both endpoints' lists,
  // so only process entries where gn is the OUTPUT side, plus external
  // sources feeding selected nodes (processed from the input side).
  struct ExtRef {
    GraphNode* node;
    std::string slot;
  };
  struct InPortDef {
    std::string name;
    ExtRef source;
    std::vector<std::pair<int, std::string>> targets;
  };
  struct OutPortDef {
    std::string name;
    int id;
    std::string slot;
    std::vector<ExtRef> dests;
  };
  std::vector<InPortDef> inPorts;
  std::vector<OutPortDef> outPorts;

  auto uniqueName = [](std::vector<std::string>& used, std::string base) {
    std::string name = base;
    int n = 2;
    while (std::find(used.begin(), used.end(), name) != used.end())
      name = base + "_" + std::to_string(n++);
    used.push_back(name);
    return name;
  };
  std::vector<std::string> usedIn, usedOut;

  for (auto* gn : sel) {
    for (auto& conn : gn->connections()) {
      GraphNode* outNode = (GraphNode*)conn.outputNodePtr;
      GraphNode* inNode = (GraphNode*)conn.inputNodePtr;
      if (!outNode || !inNode || !conn.outputSlot || !conn.inputSlot)
        continue;
      bool fromSel = selIds.count(outNode->texNode()->id) > 0;
      bool toSel = selIds.count(inNode->texNode()->id) > 0;

      if (fromSel && toSel) {
        // internal connection — record once (from the output side)
        if (outNode == gn)
          connsArr.push_back({{"fromId", outNode->texNode()->id},
                              {"fromSlot", conn.outputSlot},
                              {"toId", inNode->texNode()->id},
                              {"toSlot", conn.inputSlot}});
      } else if (!fromSel && toSel && inNode == gn) {
        // external source -> selected: input port (one per unique source)
        InPortDef* port = nullptr;
        for (auto& p : inPorts)
          if (p.source.node == outNode && p.source.slot == conn.outputSlot) {
            port = &p;
            break;
          }
        if (!port) {
          inPorts.push_back({uniqueName(usedIn, conn.inputSlot),
                             {outNode, conn.outputSlot},
                             {}});
          port = &inPorts.back();
        }
        port->targets.push_back({inNode->texNode()->id, conn.inputSlot});
      } else if (fromSel && !toSel && outNode == gn) {
        // selected -> external: output port (one per unique inner source)
        OutPortDef* port = nullptr;
        for (auto& p : outPorts)
          if (p.id == outNode->texNode()->id && p.slot == conn.outputSlot) {
            port = &p;
            break;
          }
        if (!port) {
          outPorts.push_back({uniqueName(usedOut, conn.outputSlot),
                              outNode->texNode()->id,
                              conn.outputSlot,
                              {}});
          port = &outPorts.back();
        }
        port->dests.push_back({inNode, conn.inputSlot});
      }
    }
  }

  // Build subgraph params
  nlohmann::json ins = nlohmann::json::array();
  for (auto& p : inPorts) {
    nlohmann::json targets = nlohmann::json::array();
    for (auto& t : p.targets)
      targets.push_back({t.first, t.second});
    ins.push_back({{"name", p.name}, {"targets", targets}});
  }
  nlohmann::json outs = nlohmann::json::array();
  for (auto& p : outPorts)
    outs.push_back({{"name", p.name}, {"id", p.id}, {"slot", p.slot}});

  nlohmann::json subParams = {
      {"title", "Subgraph"},
      {"graph", {{"nodes", nodesArr}, {"connections", connsArr}}},
      {"inputs", ins},
      {"outputs", outs}};

  // Create the subgraph node
  auto it = m_registry.find("Subgraph");
  if (it == m_registry.end())
    return;
  auto texNode = it->second();
  int subId = m_nextId++;
  texNode->id = subId;
  texNode->pos = {cx, cy};
  texNode->loadParams(subParams);
  GraphNode* sub = new GraphNode(std::move(texNode), subId);
  sub->paramsOpen = true;
  m_nodes.push_back(sub);

  // Remove selected nodes (this also drops boundary connections from the
  // external nodes' lists)
  for (auto* gn : sel) {
    gn->deleteAllConnections(m_nodes);
    m_nodes.erase(std::find(m_nodes.begin(), m_nodes.end(), gn));
    delete gn;
  }

  // Rewire boundary connections to the subgraph's ports
  for (auto& p : inPorts)
    makeConnection(p.source.node, p.source.slot, sub, p.name);
  for (auto& p : outPorts)
    for (auto& d : p.dests)
      makeConnection(sub, p.name, d.node, d.slot);

  generate();
}

void NodeGraph::ungroupSelected() {
  std::vector<GraphNode*> subs;
  for (auto* gn : m_nodes)
    if (gn->isSelected() && gn->texNode()->typeName() == "Subgraph")
      subs.push_back(gn);
  if (subs.empty())
    return;

  pushUndo();

  for (auto* sub : subs) {
    nlohmann::json p = sub->texNode()->saveParams();
    auto inner = p.value("graph", nlohmann::json::object());
    auto innerNodes = inner.value("nodes", nlohmann::json::array());
    auto innerConns = inner.value("connections", nlohmann::json::array());

    // Position offset: inner centroid -> subgraph position
    float icx = 0, icy = 0;
    for (auto& nj : innerNodes) {
      icx += nj.value("posX", 0.0f);
      icy += nj.value("posY", 0.0f);
    }
    if (!innerNodes.empty()) {
      icx /= innerNodes.size();
      icy /= innerNodes.size();
    }
    float dx = sub->texNode()->pos.x - icx;
    float dy = sub->texNode()->pos.y - icy;

    // Recreate inner nodes with fresh ids
    std::map<int, int> idMap;
    for (auto& nj : innerNodes) {
      std::string typeName = nj["typeName"];
      auto it = m_registry.find(typeName);
      if (it == m_registry.end())
        continue;
      int oldId = nj["id"];
      int newId = m_nextId++;
      idMap[oldId] = newId;
      auto texNode = it->second();
      texNode->id = newId;
      texNode->pos.x = nj.value("posX", 100.0f) + dx;
      texNode->pos.y = nj.value("posY", 100.0f) + dy;
      texNode->selected = true;
      if (nj.contains("params"))
        texNode->loadParams(nj["params"]);
      m_nodes.push_back(new GraphNode(std::move(texNode), newId));
    }

    // Remap Remote link targets inside the expanded nodes
    for (auto& [oldId, newId] : idMap) {
      GraphNode* gn = findNodeById(newId);
      if (!gn || gn->texNode()->typeName() != "Remote")
        continue;
      nlohmann::json rp = gn->texNode()->saveParams();
      for (auto& l : rp["links"]) {
        int t = l.value("nodeId", -1);
        if (idMap.count(t))
          l["nodeId"] = idMap[t];
      }
      gn->texNode()->loadParams(rp);
    }

    // Inner connections
    for (auto& cj : innerConns) {
      int f = cj.value("fromId", -1), t = cj.value("toId", -1);
      if (!idMap.count(f) || !idMap.count(t))
        continue;
      makeConnection(findNodeById(idMap[f]), cj.value("fromSlot", ""),
                     findNodeById(idMap[t]), cj.value("toSlot", ""));
    }

    // Boundary connections through the port maps
    auto inPorts = p.value("inputs", nlohmann::json::array());
    auto outPorts = p.value("outputs", nlohmann::json::array());
    std::vector<NodeConnection> boundary = sub->connections();
    for (auto& conn : boundary) {
      if (!conn.outputSlot || !conn.inputSlot)
        continue;
      if (conn.inputNodePtr == sub) {
        // external source -> port
        GraphNode* ext = (GraphNode*)conn.outputNodePtr;
        for (auto& ip : inPorts) {
          if (ip.value("name", std::string()) != conn.inputSlot)
            continue;
          for (auto& tgt : ip.value("targets", nlohmann::json::array())) {
            int ti = tgt[0].get<int>();
            if (idMap.count(ti))
              makeConnection(ext, conn.outputSlot, findNodeById(idMap[ti]),
                             tgt[1].get<std::string>());
          }
        }
      } else if (conn.outputNodePtr == sub) {
        // port -> external destination
        GraphNode* ext = (GraphNode*)conn.inputNodePtr;
        for (auto& op : outPorts) {
          if (op.value("name", std::string()) != conn.outputSlot)
            continue;
          int si = op.value("id", -1);
          if (idMap.count(si))
            makeConnection(findNodeById(idMap[si]),
                           op.value("slot", std::string()), ext,
                           conn.inputSlot);
        }
      }
    }

    // Remove the subgraph node
    sub->deleteAllConnections(m_nodes);
    m_nodes.erase(std::find(m_nodes.begin(), m_nodes.end(), sub));
    delete sub;
  }

  generate();
}
