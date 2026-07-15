#pragma once
// MMNodes — UI wrappers for the Material Maker ported node types.
#include "MMCoreNodes.h"
#include "TextureNode.h"

class VoronoiNode : public UiNode<VoronoiCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class FBMNode : public UiNode<FBMCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class BlendNode : public UiNode<BlendCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class WarpNode : public UiNode<WarpCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class ColorizeNode : public UiNode<ColorizeCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class MMBricksNode : public UiNode<MMBricksCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};
