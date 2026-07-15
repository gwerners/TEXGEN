#pragma once
#include "CoreNodes.h"
#include "TextureNode.h"

// ============================================================
// ColorNode — UI wrapper for ColorCoreNode
// ============================================================
class ColorNode : public UiNode<ColorCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// OutputNode — UI wrapper for OutputCoreNode
// ============================================================
class OutputNode : public UiNode<OutputCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// NoiseNode — UI wrapper for NoiseCoreNode
// ============================================================
class NoiseNode : public UiNode<NoiseCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// CellsNode — UI wrapper for CellsCoreNode
// ============================================================
class CellsNode : public UiNode<CellsCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// GlowRectNode — UI wrapper for GlowRectCoreNode
// ============================================================
class GlowRectNode : public UiNode<GlowRectCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// ColorMatrixNode — UI wrapper for ColorMatrixCoreNode
// ============================================================
class ColorMatrixNode : public UiNode<ColorMatrixCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// CoordMatrixNode — UI wrapper for CoordMatrixCoreNode
// ============================================================
class CoordMatrixNode : public UiNode<CoordMatrixCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// ColorRemapNode — UI wrapper for ColorRemapCoreNode
// ============================================================
class ColorRemapNode : public UiNode<ColorRemapCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
};

// ============================================================
// CoordRemapNode — UI wrapper for CoordRemapCoreNode
// ============================================================
class CoordRemapNode : public UiNode<CoordRemapCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// DeriveNode — UI wrapper for DeriveCoreNode
// ============================================================
class DeriveNode : public UiNode<DeriveCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// BlurNode — UI wrapper for BlurCoreNode
// ============================================================
class BlurNode : public UiNode<BlurCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// TernaryNode — UI wrapper for TernaryCoreNode
// ============================================================
class TernaryNode : public UiNode<TernaryCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// PasteNode — UI wrapper for PasteCoreNode
// ============================================================
class PasteNode : public UiNode<PasteCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// BumpNode — UI wrapper for BumpCoreNode
// ============================================================
class BumpNode : public UiNode<BumpCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// LinearCombineNode — UI wrapper for LinearCombineCoreNode
// ============================================================
class LinearCombineNode : public UiNode<LinearCombineCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// CrystalNode — UI wrapper for CrystalCoreNode
// ============================================================
class CrystalNode : public UiNode<CrystalCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// DirectionalGradientNode — UI wrapper for DirectionalGradientCoreNode
// ============================================================
class DirectionalGradientNode : public UiNode<DirectionalGradientCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// GlowEffectNode — UI wrapper for GlowEffectCoreNode
// ============================================================
class GlowEffectNode : public UiNode<GlowEffectCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// PerlinNoiseRG2Node — UI wrapper for PerlinNoiseRG2CoreNode
// ============================================================
class PerlinNoiseRG2Node : public UiNode<PerlinNoiseRG2CoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// BlurKernelNode — UI wrapper for BlurKernelCoreNode
// ============================================================
class BlurKernelNode : public UiNode<BlurKernelCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// HSCBNode — UI wrapper for HSCBCoreNode
// ============================================================
class HSCBNode : public UiNode<HSCBCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// WaveletNode — UI wrapper for WaveletCoreNode
// ============================================================
class WaveletNode : public UiNode<WaveletCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// ColorBalanceNode — UI wrapper for ColorBalanceCoreNode
// ============================================================
class ColorBalanceNode : public UiNode<ColorBalanceCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// BricksNode — UI wrapper for BricksCoreNode
// ============================================================
class BricksNode : public UiNode<BricksCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// GradientNode — UI wrapper for GradientCoreNode
// ============================================================
class GradientNode : public UiNode<GradientCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// ImageNode — UI wrapper for ImageCoreNode
// ============================================================
class ImageNode : public UiNode<ImageCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};

// ============================================================
// CommentNode — UI wrapper for CommentCoreNode
// ============================================================
class CommentNode : public UiNode<CommentCoreNode> {
 public:
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  void renderParams() override;
};
