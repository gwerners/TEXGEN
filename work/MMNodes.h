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

class MathOpNode : public UiNode<MathOpCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class GradientMMNode : public UiNode<GradientMMCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class TilerNode : public UiNode<TilerCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class MultiWarpNode : public UiNode<MultiWarpCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class SlopeBlurNode : public UiNode<SlopeBlurCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class DotNoiseNode : public UiNode<DotNoiseCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class ScratchesNode : public UiNode<ScratchesCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class MirrorNode : public UiNode<MirrorCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class EdgeDetectNode : public UiNode<EdgeDetectCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class CreateMapNode : public UiNode<CreateMapCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class MatMapNode : public UiNode<MatMapCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};

class FillNode : public UiNode<FillCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};

class FillToUVNode : public UiNode<FillToUVCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class FillToRandomGrayNode : public UiNode<FillToRandomGrayCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class FillToRandomColorNode : public UiNode<FillToRandomColorCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class FillToColorNode : public UiNode<FillToColorCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class LevelsNode : public UiNode<LevelsCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class RemapNode : public UiNode<RemapCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class Tile2x2Node : public UiNode<Tile2x2CoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};

class NormalConvertNode : public UiNode<NormalConvertCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class CustomUVNode : public UiNode<CustomUVCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class SmoothCurvatureNode : public UiNode<SmoothCurvatureCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class AmbientOcclusionNode : public UiNode<AmbientOcclusionCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class LayerMixNode : public UiNode<LayerMixCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

class WorkflowOutputNode : public UiNode<WorkflowOutputCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};
