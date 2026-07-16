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

class MaterialNode : public UiNode<MaterialCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class NormalMapNode : public UiNode<NormalMapCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class SdfShapeNode : public UiNode<SdfShapeCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class SdfOpNode : public UiNode<SdfOpCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class SdfTransformNode : public UiNode<SdfTransformCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class SdfShowNode : public UiNode<SdfShowCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class MakeTileableNode : public UiNode<MakeTileableCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class QuantizeNode : public UiNode<QuantizeCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class EmbossNode : public UiNode<EmbossCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class Transform2DNode : public UiNode<Transform2DCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class ShapeNode : public UiNode<ShapeCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class PatternNode : public UiNode<PatternCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class CombineNode : public UiNode<CombineCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};

class DecomposeNode : public UiNode<DecomposeCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};

class InvertNode : public UiNode<InvertCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};
