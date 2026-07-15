#pragma once
// MMCoreNodes — node types ported from Material Maker (see mm_generators.h
// for licensing/attribution). Pure logic, no UI dependencies.
#include <string>
#include <vector>
#include "CoreNode.h"
#include "gentexture.hpp"
#include "mm_generators.h"

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
