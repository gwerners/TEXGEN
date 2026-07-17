#pragma once
// MMCoreNodes — node types ported from Material Maker (see mm_generators.h
// for licensing/attribution). Pure logic, no UI dependencies.
#include <string>
#include <vector>
#include "CoreNode.h"
#include "gentexture.hpp"
#include "mm_filters.h"
#include "mm_generators.h"
#include "mm_sdf.h"

// ============================================================
// VoronoiCoreNode — tileable Voronoi with 3 outputs (Color/F1/Edge)
// ============================================================
class VoronoiCoreNode : public CoreNode {
 public:
  VoronoiCoreNode();
  std::string typeName() const override { return "Voronoi"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx, m_heightIdx;
  int m_scaleX, m_scaleY;
  float m_stretchX, m_stretchY;
  float m_intensity, m_randomness, m_seed;
};

// ============================================================
// FBMCoreNode — fractal brownian motion noise (9 variants)
// ============================================================
class FBMCoreNode : public CoreNode {
 public:
  FBMCoreNode();
  std::string typeName() const override { return "FBM"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx, m_heightIdx;
  int m_mode, m_scaleX, m_scaleY, m_folds, m_octaves;
  float m_persistence, m_seed;
};

// ============================================================
// BlendCoreNode — 15 Photoshop-style blend modes + mask + opacity
// ============================================================
class BlendCoreNode : public CoreNode {
 public:
  BlendCoreNode();
  std::string typeName() const override { return "Blend"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_mode;
  float m_opacity;
};

// ============================================================
// WarpCoreNode — displace UVs along a height map gradient
// ============================================================
class WarpCoreNode : public CoreNode {
 public:
  WarpCoreNode();
  std::string typeName() const override { return "Warp"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_amount, m_epsilon;
  int m_mode = 0;  // 0 slope, 1 distance-to-top
};

// ============================================================
// ColorizeCoreNode — multi-stop gradient map
// ============================================================
class ColorizeCoreNode : public CoreNode {
 public:
  ColorizeCoreNode();
  std::string typeName() const override { return "Colorize"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  std::vector<MMGradientStop> m_stops;
};

// ============================================================
// MMBricksCoreNode — brick tessellations (5 layouts)
// ============================================================
class MMBricksCoreNode : public CoreNode {
 public:
  MMBricksCoreNode();
  std::string typeName() const override { return "BricksMM"; }
  std::string displayTitle() const override { return "Bricks v2"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx, m_heightIdx;
  int m_pattern, m_countX, m_countY, m_repeat;
  float m_offset, m_mortar, m_round, m_bevel;
  float m_col0[4], m_col1[4], m_colMortar[4];
  float m_colorBalance, m_seed;
};

// ============================================================
// MaterialCoreNode — PBR multi-map export sink
// ============================================================
class MaterialCoreNode : public CoreNode {
 public:
  MaterialCoreNode();
  std::string typeName() const override { return "Material"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  char m_baseName[256];
  // lit-preview light (see MMShadePreview)
  float m_lightAzimuth = 135.0f;
  float m_lightElevation = 45.0f;
  float m_lightIntensity = 1.0f;
  float m_ambient = 0.25f;
};

// ============================================================
// NormalMapCoreNode — height to tangent-space normal map
// ============================================================
class NormalMapCoreNode : public CoreNode {
 public:
  NormalMapCoreNode();
  std::string typeName() const override { return "NormalMap"; }
  std::string displayTitle() const override { return "Normal Map"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_amount;
  int m_format;
};

// ============================================================
// SdfShapeCoreNode — 2D SDF shape generator
// ============================================================
class SdfShapeCoreNode : public CoreNode {
 public:
  SdfShapeCoreNode();
  std::string typeName() const override { return "SdfShape"; }
  std::string displayTitle() const override { return "SDF Shape"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx, m_heightIdx;
  MMSdfShapeParams m_p;
};

// ============================================================
// SdfOpCoreNode — boolean/smooth/morph combination of two SDFs
// ============================================================
class SdfOpCoreNode : public CoreNode {
 public:
  SdfOpCoreNode();
  std::string typeName() const override { return "SdfOp"; }
  std::string displayTitle() const override { return "SDF Op"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_op;
  float m_k;
};

// ============================================================
// SdfTransformCoreNode — translate/rotate/scale/round/annular
// ============================================================
class SdfTransformCoreNode : public CoreNode {
 public:
  SdfTransformCoreNode();
  std::string typeName() const override { return "SdfTransform"; }
  std::string displayTitle() const override { return "SDF Transform"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_tx, m_ty, m_rot, m_scale, m_round, m_annularW;
  int m_annularCount;
};

// ============================================================
// SdfShowCoreNode — SDF to grayscale image
// ============================================================
class SdfShowCoreNode : public CoreNode {
 public:
  SdfShowCoreNode();
  std::string typeName() const override { return "SdfShow"; }
  std::string displayTitle() const override { return "SDF Show"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_base, m_bevel;
};

// ============================================================
// MakeTileableCoreNode
// ============================================================
class MakeTileableCoreNode : public CoreNode {
 public:
  MakeTileableCoreNode();
  std::string typeName() const override { return "MakeTileable"; }
  std::string displayTitle() const override { return "Make Tileable"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_width;
};

// ============================================================
// QuantizeCoreNode
// ============================================================
class QuantizeCoreNode : public CoreNode {
 public:
  QuantizeCoreNode();
  std::string typeName() const override { return "Quantize"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_steps;
};

// ============================================================
// EmbossCoreNode
// ============================================================
class EmbossCoreNode : public CoreNode {
 public:
  EmbossCoreNode();
  std::string typeName() const override { return "Emboss"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_angle, m_amount;
  int m_width;
};

// ============================================================
// Transform2DCoreNode — affine transform with tiling
// ============================================================
class Transform2DCoreNode : public CoreNode {
 public:
  Transform2DCoreNode();
  std::string typeName() const override { return "Transform2D"; }
  std::string displayTitle() const override { return "Transform"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_tx, m_ty, m_rot, m_scaleX, m_scaleY;
  bool m_repeat;
};

// ============================================================
// ShapeCoreNode — simple soft-edge shapes
// ============================================================
class ShapeCoreNode : public CoreNode {
 public:
  ShapeCoreNode();
  std::string typeName() const override { return "Shape"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx, m_heightIdx;
  int m_shape;
  float m_sides, m_radius, m_edge;
};

// ============================================================
// PatternCoreNode — X/Y wave combinations
// ============================================================
class PatternCoreNode : public CoreNode {
 public:
  PatternCoreNode();
  std::string typeName() const override { return "Pattern"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx, m_heightIdx;
  int m_mix, m_xWave, m_yWave;
  float m_xScale, m_yScale;
};

// ============================================================
// CombineCoreNode — four grayscale channels into RGBA
// ============================================================
class CombineCoreNode : public CoreNode {
 public:
  CombineCoreNode() {}
  std::string typeName() const override { return "Combine"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// DecomposeCoreNode — RGBA into four grayscale outputs
// ============================================================
class DecomposeCoreNode : public CoreNode {
 public:
  DecomposeCoreNode() {}
  std::string typeName() const override { return "Decompose"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// InvertCoreNode — 1 - rgb, alpha preserved
// ============================================================
class InvertCoreNode : public CoreNode {
 public:
  InvertCoreNode() {}
  std::string typeName() const override { return "Invert"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// MathOpCoreNode — per-pixel scalar math (math.mmg)
// ============================================================
class MathOpCoreNode : public CoreNode {
 public:
  MathOpCoreNode() {}
  std::string typeName() const override { return "MathOp"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_op = 0;
  float m_def1 = 0.0f;  // used when A is unconnected
  float m_def2 = 0.0f;  // used when B is unconnected
  bool m_clamp = false;
};

// ============================================================
// GradientMMCoreNode — rotated repeating gradient (gradient.mmg)
// ============================================================
class GradientMMCoreNode : public CoreNode {
 public:
  GradientMMCoreNode();
  std::string typeName() const override { return "GradientMM"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  std::vector<MMGradientStop> m_stops;
  float m_repeat = 1.0f;
  float m_rotate = 0.0f;
  bool m_mirror = false;
  int m_shape = 0;  // 0 linear, 1 radial, 2 circular
  int m_widthIdx = 3, m_heightIdx = 3;
};

// ============================================================
// TilerCoreNode — instance scatter/tiler (tiler.mmg)
// ============================================================
class TilerCoreNode : public CoreNode {
 public:
  TilerCoreNode() {}
  std::string typeName() const override { return "Tiler"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_tx = 4.0f, m_ty = 4.0f;
  int m_overlap = 1;
  int m_inputs = 1;  // tileset subdivision of the input (1, 2 or 4)
  float m_scaleX = 1.0f, m_scaleY = 1.0f;
  float m_fixedOffset = 0.0f, m_offset = 0.5f;
  float m_rotate = 0.0f, m_scale = 0.0f;
  float m_value = 0.0f;
  float m_seed = 0.0f;
};

// ============================================================
// MultiWarpCoreNode — iterative slope warp (multi_warp.mmg)
// ============================================================
class MultiWarpCoreNode : public CoreNode {
 public:
  MultiWarpCoreNode() {}
  std::string typeName() const override { return "MultiWarp"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_size = 9.0f;
  float m_intensity = 0.5f;
  float m_quality = 50.0f;
  int m_mode = 2;  // 0 min, 1 blur, 2 max
};

// ============================================================
// LevelsCoreNode — per-channel levels (tones.mmg)
// ============================================================
class LevelsCoreNode : public CoreNode {
 public:
  LevelsCoreNode() {}
  std::string typeName() const override { return "Levels"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_inMin[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float m_inMid[4] = {0.5f, 0.5f, 0.5f, 0.5f};
  float m_inMax[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float m_outMin[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float m_outMax[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

// ============================================================
// RemapCoreNode — linear remap + quantization (remap.mmg)
// ============================================================
class RemapCoreNode : public CoreNode {
 public:
  RemapCoreNode() {}
  std::string typeName() const override { return "Remap"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_min = 0.0f, m_max = 1.0f, m_step = 0.0f;
};

// ============================================================
// Tile2x2CoreNode — quadrant packing (tile2x2.mmg)
// ============================================================
class Tile2x2CoreNode : public CoreNode {
 public:
  Tile2x2CoreNode() {}
  std::string typeName() const override { return "Tile2x2"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// NormalConvertCoreNode — normal convention flip
// ============================================================
class NormalConvertCoreNode : public CoreNode {
 public:
  NormalConvertCoreNode() {}
  std::string typeName() const override { return "NormalConvert"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_op = 1;  // 0 OpenGL flip, 1 DirectX flip
};

// ============================================================
// CustomUVCoreNode — per-region UV scatter (custom_uv.mmg)
// ============================================================
class CustomUVCoreNode : public CoreNode {
 public:
  CustomUVCoreNode() {}
  std::string typeName() const override { return "CustomUV"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_inputs = 1;
  float m_sx = 1.0f, m_sy = 1.0f;
  float m_rotate = 0.0f, m_scale = 0.5f;
  float m_seed = 0.0f;
};

// ============================================================
// SmoothCurvatureCoreNode — heightmap curvature
// ============================================================
class SmoothCurvatureCoreNode : public CoreNode {
 public:
  SmoothCurvatureCoreNode() {}
  std::string typeName() const override { return "SmoothCurvature"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_quality = 4.0f;
  float m_strength = 1.0f;
  float m_radius = 1.0f;
};

// ============================================================
// AmbientOcclusionCoreNode — blur-based AO from height
// ============================================================
class AmbientOcclusionCoreNode : public CoreNode {
 public:
  AmbientOcclusionCoreNode() {}
  std::string typeName() const override { return "AmbientOcclusion"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_radius = 0.05f;  // blur radius as a fraction of the width
  float m_strength = 1.0f;
};

// ============================================================
// FillCoreNode — region detection into a fill map (fill.mmg)
// ============================================================
class FillCoreNode : public CoreNode {
 public:
  FillCoreNode() {}
  std::string typeName() const override { return "Fill"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// FillToUVCoreNode — per-region local UVs (fill_to_uv.mmg)
// ============================================================
class FillToUVCoreNode : public CoreNode {
 public:
  FillToUVCoreNode() {}
  std::string typeName() const override { return "FillToUV"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_mode = 0;  // 0 stretch, 1 square
  float m_seed = 0.0f;
};

// ============================================================
// FillToRandomGrayCoreNode / FillToRandomColorCoreNode
// ============================================================
class FillToRandomGrayCoreNode : public CoreNode {
 public:
  FillToRandomGrayCoreNode() {}
  std::string typeName() const override { return "FillToRandomGray"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_edgecolor = 1.0f;
  float m_seed = 0.0f;
};

class FillToRandomColorCoreNode : public CoreNode {
 public:
  FillToRandomColorCoreNode() {}
  std::string typeName() const override { return "FillToRandomColor"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_edge[3] = {1.0f, 1.0f, 1.0f};
  float m_seed = 0.0f;
};

// ============================================================
// FillToColorCoreNode — sample a map at region centers
// ============================================================
class FillToColorCoreNode : public CoreNode {
 public:
  FillToColorCoreNode() {}
  std::string typeName() const override { return "FillToColor"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_edge[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

// ============================================================
// HeightToOffsetCoreNode — contour push (height_to_offset.mmg)
// ============================================================
class HeightToOffsetCoreNode : public CoreNode {
 public:
  HeightToOffsetCoreNode() {}
  std::string typeName() const override { return "HeightToOffset"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_target = 0.5f;
};

// ============================================================
// BevelCoreNode — distance-ramp bevel + profile curve (bevel.mmg)
// ============================================================
class BevelCoreNode : public CoreNode {
 public:
  BevelCoreNode() {}
  std::string typeName() const override { return "Bevel"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_distance = 0.1f;
  // [[x, y, ls, rs], ...] — identity ramp when empty
  std::vector<MMCurvePoint> m_curve;
};

// ============================================================
// DilateCoreNode — spread source colors from a mask (dilate.mmg)
// ============================================================
class DilateCoreNode : public CoreNode {
 public:
  DilateCoreNode() {}
  std::string typeName() const override { return "Dilate"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_length = 0.27f;
  float m_fill = 0.0f;
  int m_metric = 0;  // 0 euclidean, 1 manhattan, 2 chebyshev
};

// ============================================================
// WeaveCoreNode / Weave2CoreNode — woven patterns (weave/weave2.mmg)
// ============================================================
class WeaveCoreNode : public CoreNode {
 public:
  WeaveCoreNode() {}
  std::string typeName() const override { return "Weave"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  int m_columns = 4, m_rows = 4;
  float m_width = 0.8f;
};

class Weave2CoreNode : public CoreNode {
 public:
  Weave2CoreNode() {}
  std::string typeName() const override { return "Weave2"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  int m_columns = 4, m_rows = 4;
  float m_widthX = 0.8f, m_widthY = 0.8f;
  float m_stitch = 1.0f;
};

// ============================================================
// EdgeDetect2CoreNode — laplacian edges (edge_detect_2.mmg)
// ============================================================
class EdgeDetect2CoreNode : public CoreNode {
 public:
  EdgeDetect2CoreNode() {}
  std::string typeName() const override { return "EdgeDetect2"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_size = 512.0f;
};

// ============================================================
// SmoothMinMaxCoreNode — polynomial smin/smax (smooth_minmax.mmg)
// ============================================================
class SmoothMinMaxCoreNode : public CoreNode {
 public:
  SmoothMinMaxCoreNode() {}
  std::string typeName() const override { return "SmoothMinMax"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_op = 0;  // 0 smin, 1 smax
  float m_k = 0.1f;
  float m_def1 = 0.0f, m_def2 = 0.0f;
};

// ============================================================
// FillToGradientCoreNode / FillToSizeCoreNode (fill family)
// ============================================================
class FillToGradientCoreNode : public CoreNode {
 public:
  FillToGradientCoreNode() {}
  std::string typeName() const override { return "FillToGradient"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  std::vector<MMGradientStop> m_stops;
  int m_mode = 0;  // 0 stretch, 1 square
  int m_layers = 1;
  float m_rotate = 0.0f, m_rndRotate = 0.0f, m_rndOffset = 0.0f;
  float m_seed = 0.0f;
};

class FillToSizeCoreNode : public CoreNode {
 public:
  FillToSizeCoreNode() {}
  std::string typeName() const override { return "FillToSize"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_formula = 0;  // 0 area, 1 width, 2 height, 3 max
};

// ============================================================
// ColorNoiseCoreNode — random RGB per grid cell (color_noise.mmg)
// ============================================================
class ColorNoiseCoreNode : public CoreNode {
 public:
  ColorNoiseCoreNode() {}
  std::string typeName() const override { return "ColorNoise"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  int m_grid = 256;  // cells per axis
  float m_seed = 0.0f;
};

// ============================================================
// DirectionalBlurCoreNode — one-sided gaussian smear along an
// angle (directional_blur.mmg)
// ============================================================
class DirectionalBlurCoreNode : public CoreNode {
 public:
  DirectionalBlurCoreNode() {}
  std::string typeName() const override { return "DirectionalBlur"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_size = 512.0f;  // texel step = 1/size UV
  float m_sigma = 0.5f;
  float m_angle = 0.0f;
  int m_mode = 0;  // first tap index (1 skips the center texel)
};

// ============================================================
// NormalBlendCoreNode — reoriented normal mapping (normal_blend.mmg)
// ============================================================
class NormalBlendCoreNode : public CoreNode {
 public:
  NormalBlendCoreNode() {}
  std::string typeName() const override { return "NormalBlend"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_amount = 0.5f;
};

// ============================================================
// AnisotropicNoiseCoreNode — stripe noise (noise_anisotropic.mmg)
// ============================================================
class AnisotropicNoiseCoreNode : public CoreNode {
 public:
  AnisotropicNoiseCoreNode() {}
  std::string typeName() const override { return "AnisotropicNoise"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  float m_scaleX = 4.0f, m_scaleY = 256.0f;
  float m_smoothness = 1.0f, m_interpolation = 1.0f;
  float m_seed = 0.0f;
};

// ============================================================
// TilerAdvancedCoreNode — per-instance modulated scatter
// (tiler_advanced.mmg)
// ============================================================
class TilerAdvancedCoreNode : public CoreNode {
 public:
  TilerAdvancedCoreNode() {}
  std::string typeName() const override { return "TilerAdvanced"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_tx = 4.0f, m_ty = 4.0f;
  int m_overlap = 1;
  int m_inputs = 1;
  float m_translateX = 0.0f, m_translateY = 0.0f;
  float m_rotate = 0.0f;
  float m_scaleX = 1.0f, m_scaleY = 1.0f;
  float m_seed = 0.0f;
};

// ============================================================
// SphereCoreNode — hemisphere heightmap (sphere.mmg)
// ============================================================
class SphereCoreNode : public CoreNode {
 public:
  SphereCoreNode() {}
  std::string typeName() const override { return "Sphere"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  float m_cx = 0.5f, m_cy = 0.5f, m_r = 0.5f;
  bool m_normalized = false;
};

// ============================================================
// DotNoiseCoreNode — per-cell random dots (noise.mmg)
// ============================================================
class DotNoiseCoreNode : public CoreNode {
 public:
  DotNoiseCoreNode() {}
  std::string typeName() const override { return "DotNoise"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  int m_grid = 256;  // cells per axis
  float m_density = 0.5f;
  float m_seed = 0.0f;
  int m_mode = 0;  // 0 = threshold dots, 1 = raw random value
};

// ============================================================
// ScratchesCoreNode — layered line scratches (scratches.mmg)
// ============================================================
class ScratchesCoreNode : public CoreNode {
 public:
  ScratchesCoreNode() {}
  std::string typeName() const override { return "Scratches"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_widthIdx = 3, m_heightIdx = 3;
  int m_layers = 4;
  float m_length = 0.25f;
  float m_width = 0.5f;
  float m_waviness = 0.5f;
  float m_angle = 0.0f;
  float m_randomness = 0.5f;
  float m_seed = 0.0f;
};

// ============================================================
// MirrorCoreNode — UV mirror (mirror.mmg)
// ============================================================
class MirrorCoreNode : public CoreNode {
 public:
  MirrorCoreNode() {}
  std::string typeName() const override { return "Mirror"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_direction = 0;  // 0 horizontal, 1 vertical
  float m_offset = 0.0f;
  bool m_flipSides = false;
};

// ============================================================
// EdgeDetectCoreNode — color-distance edges (edge_detect.mmg)
// ============================================================
class EdgeDetectCoreNode : public CoreNode {
 public:
  EdgeDetectCoreNode() {}
  std::string typeName() const override { return "EdgeDetect"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_size = 512.0f;  // sampling distance = 1/size
  int m_width = 1;
  float m_threshold = 0.5f;
};

// ============================================================
// CreateMapCoreNode — workflow placement map (mwf_create_map.mmg)
// ============================================================
class CreateMapCoreNode : public CoreNode {
 public:
  CreateMapCoreNode() {}
  std::string typeName() const override { return "CreateMap"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_height = 1.0f;
  float m_angle = 0.0f;
  float m_seed = 0.0f;
};

// ============================================================
// MatMapCoreNode — apply placement map to a bundle (mwf_map.mmg)
// ============================================================
class MatMapCoreNode : public CoreNode {
 public:
  MatMapCoreNode() {}
  std::string typeName() const override { return "MatMap"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
};

// ============================================================
// SlopeBlurCoreNode — directional gaussian along the heightmap
// slope (slope_blur.mmg)
// ============================================================
class SlopeBlurCoreNode : public CoreNode {
 public:
  SlopeBlurCoreNode() {}
  std::string typeName() const override { return "SlopeBlur"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_size = 9.0f;
  float m_sigma = 0.5f;
};

// ============================================================
// LayerMixCoreNode — height-based mix of two material layers
// (Material Maker mwf_mix / mwf_mix_smooth)
// ============================================================
class LayerMixCoreNode : public CoreNode {
 public:
  LayerMixCoreNode() {}
  std::string typeName() const override { return "LayerMix"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  int m_mode = 0; // 0 = hard max-height select, 1 = smooth blend
  float m_width = 0.05f;
};

// ============================================================
// WorkflowOutputCoreNode — material bundle to PBR maps
// (Material Maker mwf_output)
// ============================================================
class WorkflowOutputCoreNode : public CoreNode {
 public:
  WorkflowOutputCoreNode() {}
  std::string typeName() const override { return "WorkflowOutput"; }
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture*>& inputs,
               std::vector<GenTexture>& outputs) override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json& j) override;

  float m_matNormal = 1.0f;  // weight of the bundle normal
  float m_occlusion = 1.0f;  // AO strength
};
