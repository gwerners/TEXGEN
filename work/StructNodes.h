#pragma once
// StructNodes — UI wrappers for structural node types (Subgraph, Remote).
#include "GraphCoreNodes.h"
#include "TextureNode.h"

class SubgraphNode : public UiNode<SubgraphCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class RemoteNode : public UiNode<RemoteCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};
