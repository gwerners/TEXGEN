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
