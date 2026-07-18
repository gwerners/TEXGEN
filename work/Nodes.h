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
class GraphRunner;

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
  void generate();  // evaluate all nodes (async when a runner is set)

  // Attach the worker-thread evaluator; when set, generate() and
  // refreshNode() enqueue snapshots instead of blocking the UI.
  void setRunner(GraphRunner* runner) { m_runner = runner; }
  bool evaluating() const { return m_evaluating; }

  // Refresh a single node (gather inputs, execute, update preview)
  // then cascade-refresh all downstream nodes
  void refreshNode(GraphNode* node);

  // Reset a node's parameters to factory defaults
  void resetNodeParams(GraphNode* node);

  // Returns output image from last OutputNode executed, or nullptr
  GenTexture* getLastOutput();

  // Change counter — increments whenever the graph output changes
  int changeCount() const { return m_changeCount; }

  // Increments only when a FULL graph run lands (every node's output is
  // fresh) — as opposed to an incremental param-edit cascade, which only
  // touches part of the graph. Used to know when the on-disk render
  // cache should be refreshed.
  int fullRunCount() const { return m_fullRunCount; }

  // Contextual hint for the status bar (hovered node, shortcuts...)
  const std::string& hintText() const { return m_hintText; }

  // All graph nodes (for panels that inspect the graph, e.g. 3D preview)
  const std::vector<GraphNode*>& nodes() const { return m_nodes; }

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

  // Subgraphs
  void groupSelected();    // collapse selected nodes into a Subgraph node
  void ungroupSelected();  // expand selected Subgraph nodes in place

  // Push Remote node values into their target nodes' params
  void applyRemoteValues();

 private:
  std::vector<GraphNode*> m_nodes;
  std::map<std::string, NodeFactory> m_registry;
  GraphRunner* m_runner = nullptr;
  bool m_evaluating = false;
  // When true, a param edit re-runs the node's downstream cone (one
  // node at a time, streamed); when false only the edited node
  // refreshes and "Propagate Now" runs the rest on demand.
  bool m_autoPropagate = true;
  void pollRunner();  // apply finished worker results (main thread)
  int m_nextId = 0;
  GenTexture m_lastOutput;
  bool m_hasOutput = false;
  int m_changeCount = 0;
  int m_fullRunCount = 0;
  std::string m_hintText;

  void syncParamHashes();
  GraphNode* findNodeByPtr(void* ptr);
  GraphNode* findNodeById(int id);
  bool makeConnection(GraphNode* from,
                      const std::string& fromSlot,
                      GraphNode* to,
                      const std::string& toSlot);
  void applyRemoteNode(GraphNode* remote, bool refreshTargets);
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
