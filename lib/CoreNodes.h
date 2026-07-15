#pragma once
// CoreNodes — pure logic implementations of all built-in node types.
// No UI dependencies (no ImGui, ImNodes, raylib).
#include <string>
#include <vector>
#include "CoreNode.h"
#include "extra_generators.hpp"
#include "gentexture.hpp"

// ============================================================
// ColorNode - creates a blank image filled with a solid color
// ============================================================
class ColorCoreNode : public CoreNode {
 public:
  ColorCoreNode();
  std::string typeName() const override { return "Color"; }
  std::string displayTitle() const override { return "Color"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_widthIdx;
  int m_heightIdx;
  float m_color[4];
};

// ============================================================
// OutputNode - saves the image to a file
// ============================================================
class OutputCoreNode : public CoreNode {
 public:
  OutputCoreNode();
  std::string typeName() const override { return "Output"; }
  std::string displayTitle() const override { return "Output"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  char m_filename[256];
};

// ============================================================
// NoiseNode - gentexture Noise
// ============================================================
class NoiseCoreNode : public CoreNode {
 public:
  NoiseCoreNode();
  std::string typeName() const override { return "Noise"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_freqX, m_freqY, m_oct, m_seed, m_mode;
  float m_fadeoff;
  int m_sizeIdx;
  float m_col1[4], m_col2[4];
};

// ============================================================
// CellsNode - gentexture Cells
// ============================================================
class CellsCoreNode : public CoreNode {
 public:
  CellsCoreNode();
  std::string typeName() const override { return "Cells"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_nCenters, m_seed, m_mode;
  float m_amp;
  int m_sizeIdx;
  float m_col1[4], m_col2[4];
  int m_colorMode;  // 0=Gradient, 1=Random
};

// ============================================================
// GlowRectNode - gentexture GlowRect
// ============================================================
class GlowRectCoreNode : public CoreNode {
 public:
  GlowRectCoreNode();
  std::string typeName() const override { return "GlowRect"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy, m_rectu, m_rectv;
};

// ============================================================
// ColorMatrixNode - gentexture ColorMatrixTransform
// ============================================================
class ColorMatrixCoreNode : public CoreNode {
 public:
  ColorMatrixCoreNode();
  std::string typeName() const override { return "ColorMatrix"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_matrix[16];
  bool m_clampPremult;
};

// ============================================================
// CoordMatrixNode - gentexture CoordMatrixTransform
// ============================================================
class CoordMatrixCoreNode : public CoreNode {
 public:
  CoordMatrixCoreNode();
  std::string typeName() const override { return "CoordMatrix"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_matrix[16];
  int m_filterMode;
};

// ============================================================
// ColorRemapNode - gentexture ColorRemap
// ============================================================
class ColorRemapCoreNode : public CoreNode {
 public:
  ColorRemapCoreNode();
  std::string typeName() const override { return "ColorRemap"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// CoordRemapNode - gentexture CoordRemap
// ============================================================
class CoordRemapCoreNode : public CoreNode {
 public:
  CoordRemapCoreNode();
  std::string typeName() const override { return "CoordRemap"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_strengthU, m_strengthV;
  int m_filterMode;
};

// ============================================================
// DeriveNode - gentexture Derive
// ============================================================
class DeriveCoreNode : public CoreNode {
 public:
  DeriveCoreNode();
  std::string typeName() const override { return "Derive"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_op;
  float m_strength;
};

// ============================================================
// BlurNode - gentexture Blur
// ============================================================
class BlurCoreNode : public CoreNode {
 public:
  BlurCoreNode();
  std::string typeName() const override { return "Blur"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_sizex, m_sizey;
  int m_order, m_mode;
};

// ============================================================
// TernaryNode - gentexture Ternary
// ============================================================
class TernaryCoreNode : public CoreNode {
 public:
  TernaryCoreNode();
  std::string typeName() const override { return "Ternary"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_op;
};

// ============================================================
// PasteNode - gentexture Paste
// ============================================================
class PasteCoreNode : public CoreNode {
 public:
  PasteCoreNode();
  std::string typeName() const override { return "Paste"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy;
  int m_op, m_mode;
};

// ============================================================
// BumpNode - gentexture Bump
// ============================================================
class BumpCoreNode : public CoreNode {
 public:
  BumpCoreNode();
  std::string typeName() const override { return "Bump"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_px, m_py, m_pz, m_dx, m_dy, m_dz;
  float m_ambient[4];
  float m_diffuse[4];
  bool m_directional;
};

// ============================================================
// LinearCombineNode - gentexture LinearCombine
// ============================================================
class LinearCombineCoreNode : public CoreNode {
 public:
  LinearCombineCoreNode();
  std::string typeName() const override { return "LinearCombine"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_constColor[4];
  float m_constWeight;
  float m_weights[4];
  float m_uShift[4];
  float m_vShift[4];
  int m_filterMode[4];
};

// ============================================================
// CrystalNode - extra Crystal
// ============================================================
class CrystalCoreNode : public CoreNode {
 public:
  CrystalCoreNode();
  std::string typeName() const override { return "Crystal"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_widthIdx, m_heightIdx;
  int m_seed, m_count;
  float m_colorNear[4];
  float m_colorFar[4];
};

// ============================================================
// DirectionalGradientNode - extra DirectionalGradient
// ============================================================
class DirectionalGradientCoreNode : public CoreNode {
 public:
  DirectionalGradientCoreNode();
  std::string typeName() const override { return "DirectionalGradient"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_widthIdx, m_heightIdx;
  float m_x1, m_y1, m_x2, m_y2;
  float m_col1[4];
  float m_col2[4];
};

// ============================================================
// GlowEffectNode - extra GlowEffect
// ============================================================
class GlowEffectCoreNode : public CoreNode {
 public:
  GlowEffectCoreNode();
  std::string typeName() const override { return "GlowEffect"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_widthIdx, m_heightIdx;
  float m_cx, m_cy, m_scale, m_exponent, m_intensity;
  float m_bgCol[4];
  float m_glowCol[4];
};

// ============================================================
// PerlinNoiseRG2Node - extra PerlinNoiseRG2
// ============================================================
class PerlinNoiseRG2CoreNode : public CoreNode {
 public:
  PerlinNoiseRG2CoreNode();
  std::string typeName() const override { return "PerlinNoiseRG2"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_widthIdx, m_heightIdx;
  int m_octaves, m_freqScale, m_seed, m_startOctave;
  float m_persistence, m_contrast;
  float m_col1[4];
  float m_col2[4];
};

// ============================================================
// BlurKernelNode - extra BlurKernel
// ============================================================
class BlurKernelCoreNode : public CoreNode {
 public:
  BlurKernelCoreNode();
  std::string typeName() const override { return "BlurKernel"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_radiusX, m_radiusY;
  int m_kernelType, m_wrapMode;
};

// ============================================================
// HSCBNode - extra HSCB
// ============================================================
class HSCBCoreNode : public CoreNode {
 public:
  HSCBCoreNode();
  std::string typeName() const override { return "HSCB"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_hue, m_sat, m_contrast, m_brightness;
};

// ============================================================
// WaveletNode - extra Wavelet
// ============================================================
class WaveletCoreNode : public CoreNode {
 public:
  WaveletCoreNode();
  std::string typeName() const override { return "Wavelet"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_mode, m_count;
};

// ============================================================
// ColorBalanceNode - extra ColorBalance
// ============================================================
class ColorBalanceCoreNode : public CoreNode {
 public:
  ColorBalanceCoreNode();
  std::string typeName() const override { return "ColorBalance"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  // shadow, mid, highlight for R, G, B
  float m_shadow[3];
  float m_mid[3];
  float m_highlight[3];
};

// ============================================================
// BricksNode - extra Bricks
// ============================================================
class BricksCoreNode : public CoreNode {
 public:
  BricksCoreNode();
  std::string typeName() const override { return "Bricks"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  int m_widthIdx, m_heightIdx;
  float m_col0[4];
  float m_col1[4];
  float m_colFuge[4];
  float m_fugeX, m_fugeY;
  int m_tileX, m_tileY;
  int m_seed, m_heads;
  float m_colorBalance;
};

// ============================================================
// GradientNode - outputs a 2x1 gradient texture from two colors
// ============================================================
class GradientCoreNode : public CoreNode {
 public:
  GradientCoreNode();
  std::string typeName() const override { return "Gradient"; }
  std::string displayTitle() const override { return "Gradient"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  float m_col1[4];
  float m_col2[4];
};

// ============================================================
// ImageNode - loads an image file as texture source
// ============================================================
class ImageCoreNode : public CoreNode {
 public:
  ImageCoreNode();
  std::string typeName() const override { return "Image"; }
  std::string displayTitle() const override { return "Image"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  char m_filename[256];
  bool m_loaded;
  GenTexture m_cache;
  std::string m_loadedPath;
};

// ============================================================
// CommentNode - visual note for organizing pipelines
// ============================================================
class CommentCoreNode : public CoreNode {
 public:
  CommentCoreNode();
  std::string typeName() const override { return "Comment"; }
  std::string displayTitle() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 public:
  char m_text[512];
  float m_color[3];
};
