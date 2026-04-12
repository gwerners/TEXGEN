#ifndef NODES_H
#define NODES_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include "ImNodes.h"
#include "ImNodesEz.h"
#include "TextureNode.h"
#include "gentexture.hpp"
#include "rlImGui.h"

enum NodeSlotTypes {
  NodeSlotImageInput = 1,
  NodeSlotImageOutput,
};

struct NodeConnection {
  void* inputNodePtr = nullptr;
  const char* inputSlot = nullptr;
  void* outputNodePtr = nullptr;
  const char* outputSlot = nullptr;

  bool operator==(const NodeConnection& o) const {
    return inputNodePtr == o.inputNodePtr && inputSlot == o.inputSlot &&
           outputNodePtr == o.outputNodePtr && outputSlot == o.outputSlot;
  }
  bool operator!=(const NodeConnection& o) const { return !operator==(o); }
};

class NodeGraph;

// Wraps a TextureNode with ImNodes graph state
class GraphNode {
 public:
  explicit GraphNode(std::unique_ptr<TextureNode> node, int id);
  ~GraphNode();

  TextureNode* texNode() { return m_node.get(); }
  const TextureNode* texNode() const { return m_node.get(); }

  void draw(NodeGraph* graph);
  bool isSelected() const { return m_node->selected; }

  void deleteAllConnections(std::vector<GraphNode*>& allNodes);
  void deleteConnection(const NodeConnection& conn);
  void syncParamsHash() { m_lastParamsHash = m_node->saveParams().dump(); }

  std::vector<NodeConnection>& connections() { return m_connections; }
  const std::vector<NodeConnection>& connections() const {
    return m_connections;
  }

  // Cached outputs from last execute
  std::vector<GenTexture> cachedOutputs;
  bool executed = false;

  // GPU preview texture
  Texture2D previewTex;
  bool hasPreview = false;

  // Node rect from previous frame (for z-order overlap detection)
  ImVec2 lastRectMin = {0, 0};
  ImVec2 lastRectMax = {0, 0};

  // Collapsible section state
  bool paramsOpen = false;
  bool previewOpen = false;

 private:
  std::unique_ptr<TextureNode> m_node;
  std::vector<NodeConnection> m_connections;
  std::string m_lastParamsHash;
  bool m_paramsDirty = false;
};

// Factory registry for creating nodes by type name
using NodeFactory = std::function<std::unique_ptr<TextureNode>()>;
void registerAllNodes(std::map<std::string, NodeFactory>& registry);

class NodeGraph {
 public:
  NodeGraph();
  ~NodeGraph();

  void draw();      // renders ImNodes canvas with context menu
  void generate();  // topological sort + execute all nodes

  // Refresh a single node (gather inputs, execute, update preview)
  // then cascade-refresh all downstream nodes
  void refreshNode(GraphNode* node);

  // Reset a node's parameters to factory defaults
  void resetNodeParams(GraphNode* node);

  // Returns output image from last OutputNode executed, or nullptr
  GenTexture* getLastOutput();

  // Change counter — increments whenever the graph output changes
  int changeCount() const { return m_changeCount; }

  nlohmann::json save() const;
  void load(const nlohmann::json& j);
  void clear();

  GenTexture* getInputImageForSlot(GraphNode* node, int slotIdx);

  // Undo/Redo
  void pushUndo();  // save current state to undo stack
  void undo();
  void redo();
  bool canUndo() const;
  bool canRedo() const;

  // Copy/Paste
  void copySelected();
  void pasteClipboard();

 private:
  std::vector<GraphNode*> m_nodes;
  std::map<std::string, NodeFactory> m_registry;
  int m_nextId = 0;
  GenTexture m_lastOutput;
  bool m_hasOutput = false;
  int m_changeCount = 0;

  void syncParamHashes();
  GraphNode* findNodeByPtr(void* ptr);
  GraphNode* findNodeById(int id);
  std::vector<GraphNode*> topologicalSort();
  std::vector<GraphNode*> getDownstreamNodes(GraphNode* start);

  // Undo/Redo state
  std::vector<nlohmann::json> m_undoStack;
  std::vector<nlohmann::json> m_redoStack;
  static const int MAX_UNDO = 50;

  // Copy/Paste clipboard
  nlohmann::json m_clipboard;
};

extern NodeGraph* g_nodeGraph;
void createNodeCanvas();

#endif  // NODES_H
