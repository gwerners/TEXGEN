#pragma once
#include <imgui.h>
#include <raylib.h>
#include <string>
#include <vector>
#include "TextureNode.h"
#include "extra_generators.hpp"
#include "gentexture.hpp"

// ============================================================
// InputNode - creates a blank image filled with a solid color
// ============================================================
class InputNode : public TextureNode {
 public:
  InputNode();
  std::string typeName() const override { return "Input"; }
  std::string displayTitle() const override { return "Input"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_widthIdx;
  int m_heightIdx;
  float m_color[4];
};

// ============================================================
// OutputNode - saves the image to a file
// ============================================================
class OutputNode : public TextureNode {
 public:
  OutputNode();
  std::string typeName() const override { return "Output"; }
  std::string displayTitle() const override { return "Output"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  char m_filename[256];
};

// ============================================================
// NoiseNode - gentexture Noise
// ============================================================
class NoiseNode : public TextureNode {
 public:
  NoiseNode();
  std::string typeName() const override { return "Noise"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_freqX, m_freqY, m_oct, m_seed, m_mode;
  float m_fadeoff;
  int m_sizeIdx;
  float m_col1[4], m_col2[4];
};

// ============================================================
// CellsNode - gentexture Cells
// ============================================================
class CellsNode : public TextureNode {
 public:
  CellsNode();
  std::string typeName() const override { return "Cells"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_nCenters, m_seed, m_mode;
  float m_amp;
  int m_sizeIdx;
  float m_col1[4], m_col2[4];
  int m_colorMode;  // 0=Gradient, 1=Random
};

// ============================================================
// GlowRectNode - gentexture GlowRect
// ============================================================
class GlowRectNode : public TextureNode {
 public:
  GlowRectNode();
  std::string typeName() const override { return "GlowRect"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy, m_rectu, m_rectv;
};

// ============================================================
// ColorMatrixNode - gentexture ColorMatrixTransform
// ============================================================
class ColorMatrixNode : public TextureNode {
 public:
  ColorMatrixNode();
  std::string typeName() const override { return "ColorMatrix"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_matrix[16];
  bool m_clampPremult;
};

// ============================================================
// CoordMatrixNode - gentexture CoordMatrixTransform
// ============================================================
class CoordMatrixNode : public TextureNode {
 public:
  CoordMatrixNode();
  std::string typeName() const override { return "CoordMatrix"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_matrix[16];
  int m_filterMode;
};

// ============================================================
// ColorRemapNode - gentexture ColorRemap
// ============================================================
class ColorRemapNode : public TextureNode {
 public:
  ColorRemapNode();
  std::string typeName() const override { return "ColorRemap"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// CoordRemapNode - gentexture CoordRemap
// ============================================================
class CoordRemapNode : public TextureNode {
 public:
  CoordRemapNode();
  std::string typeName() const override { return "CoordRemap"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_strengthU, m_strengthV;
  int m_filterMode;
};

// ============================================================
// DeriveNode - gentexture Derive
// ============================================================
class DeriveNode : public TextureNode {
 public:
  DeriveNode();
  std::string typeName() const override { return "Derive"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_op;
  float m_strength;
};

// ============================================================
// BlurNode - gentexture Blur
// ============================================================
class BlurNode : public TextureNode {
 public:
  BlurNode();
  std::string typeName() const override { return "Blur"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_sizex, m_sizey;
  int m_order, m_mode;
};

// ============================================================
// TernaryNode - gentexture Ternary
// ============================================================
class TernaryNode : public TextureNode {
 public:
  TernaryNode();
  std::string typeName() const override { return "Ternary"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_op;
};

// ============================================================
// PasteNode - gentexture Paste
// ============================================================
class PasteNode : public TextureNode {
 public:
  PasteNode();
  std::string typeName() const override { return "Paste"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy;
  int m_op, m_mode;
};

// ============================================================
// BumpNode - gentexture Bump
// ============================================================
class BumpNode : public TextureNode {
 public:
  BumpNode();
  std::string typeName() const override { return "Bump"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_px, m_py, m_pz, m_dx, m_dy, m_dz;
  float m_ambient[4];
  float m_diffuse[4];
  bool m_directional;
};

// ============================================================
// LinearCombineNode - gentexture LinearCombine
// ============================================================
class LinearCombineNode : public TextureNode {
 public:
  LinearCombineNode();
  std::string typeName() const override { return "LinearCombine"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
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
class CrystalNode : public TextureNode {
 public:
  CrystalNode();
  std::string typeName() const override { return "Crystal"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_widthIdx, m_heightIdx;
  int m_seed, m_count;
  float m_colorNear[4];
  float m_colorFar[4];
};

// ============================================================
// DirectionalGradientNode - extra DirectionalGradient
// ============================================================
class DirectionalGradientNode : public TextureNode {
 public:
  DirectionalGradientNode();
  std::string typeName() const override { return "DirectionalGradient"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_widthIdx, m_heightIdx;
  float m_x1, m_y1, m_x2, m_y2;
  float m_col1[4];
  float m_col2[4];
};

// ============================================================
// GlowEffectNode - extra GlowEffect
// ============================================================
class GlowEffectNode : public TextureNode {
 public:
  GlowEffectNode();
  std::string typeName() const override { return "GlowEffect"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_widthIdx, m_heightIdx;
  float m_cx, m_cy, m_scale, m_exponent, m_intensity;
  float m_bgCol[4];
  float m_glowCol[4];
};

// ============================================================
// PerlinNoiseRG2Node - extra PerlinNoiseRG2
// ============================================================
class PerlinNoiseRG2Node : public TextureNode {
 public:
  PerlinNoiseRG2Node();
  std::string typeName() const override { return "PerlinNoiseRG2"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_widthIdx, m_heightIdx;
  int m_octaves, m_freqScale, m_seed, m_startOctave;
  float m_persistence, m_contrast;
  float m_col1[4];
  float m_col2[4];
};

// ============================================================
// BlurKernelNode - extra BlurKernel
// ============================================================
class BlurKernelNode : public TextureNode {
 public:
  BlurKernelNode();
  std::string typeName() const override { return "BlurKernel"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_radiusX, m_radiusY;
  int m_kernelType, m_wrapMode;
};

// ============================================================
// HSCBNode - extra HSCB
// ============================================================
class HSCBNode : public TextureNode {
 public:
  HSCBNode();
  std::string typeName() const override { return "HSCB"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_hue, m_sat, m_contrast, m_brightness;
};

// ============================================================
// WaveletNode - extra Wavelet
// ============================================================
class WaveletNode : public TextureNode {
 public:
  WaveletNode();
  std::string typeName() const override { return "Wavelet"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  int m_mode, m_count;
};

// ============================================================
// ColorBalanceNode - extra ColorBalance
// ============================================================
class ColorBalanceNode : public TextureNode {
 public:
  ColorBalanceNode();
  std::string typeName() const override { return "ColorBalance"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  // shadow, mid, highlight for R, G, B
  float m_shadow[3];
  float m_mid[3];
  float m_highlight[3];
};

// ============================================================
// BricksNode - extra Bricks
// ============================================================
class BricksNode : public TextureNode {
 public:
  BricksNode();
  std::string typeName() const override { return "Bricks"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
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
class GradientNode : public TextureNode {
 public:
  GradientNode();
  std::string typeName() const override { return "Gradient"; }
  std::string displayTitle() const override { return "Gradient"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  float m_col1[4];
  float m_col2[4];
};

// ============================================================
// ImageNode - loads an image file as texture source
// ============================================================
class ImageNode : public TextureNode {
 public:
  ImageNode();
  std::string typeName() const override { return "Image"; }
  std::string displayTitle() const override { return "Image"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

 private:
  char m_filename[256];
  bool m_loaded;
  GenTexture m_cache;
  std::string m_loadedPath;
};
