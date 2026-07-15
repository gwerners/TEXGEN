#pragma once
#include "AggCoreNodes.h"
#include "TextureNode.h"

// ============================================================
// AggLineNode — UI wrapper for AggLineCoreNode
// ============================================================
class AggLineNode : public UiNode<AggLineCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggCircleNode — UI wrapper for AggCircleCoreNode
// ============================================================
class AggCircleNode : public UiNode<AggCircleCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggRectNode — UI wrapper for AggRectCoreNode
// ============================================================
class AggRectNode : public UiNode<AggRectCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggPolygonNode — UI wrapper for AggPolygonCoreNode
// ============================================================
class AggPolygonNode : public UiNode<AggPolygonCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggTextNode — UI wrapper for AggTextCoreNode
// ============================================================
class AggTextNode : public UiNode<AggTextCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggArcNode — UI wrapper for AggArcCoreNode
// ============================================================
class AggArcNode : public UiNode<AggArcCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggBezierNode — UI wrapper for AggBezierCoreNode
// ============================================================
class AggBezierNode : public UiNode<AggBezierCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggDashLineNode — UI wrapper for AggDashLineCoreNode
// ============================================================
class AggDashLineNode : public UiNode<AggDashLineCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// AggGradientNode — UI wrapper for AggGradientCoreNode
// ============================================================
class AggGradientNode : public UiNode<AggGradientCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};
