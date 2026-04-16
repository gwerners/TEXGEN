#pragma once
#include <string>
#include <vector>
#include "TextureNode.h"
#include "gentexture.hpp"

// ============================================================
// AggLineNode - draws a line segment using AGG
// ============================================================
class AggLineNode : public TextureNode {
public:
  AggLineNode();
  std::string typeName() const override { return "AggLine"; }
  std::string displayTitle() const override { return "Line"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture *> &inputs,
               std::vector<GenTexture> &outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json &j) override;

private:
  int m_widthIdx, m_heightIdx;
  float m_x1, m_y1, m_x2, m_y2;
  float m_thickness;
  float m_color[4];
  float m_bgColor[4];
};

// ============================================================
// AggCircleNode - draws an ellipse using AGG
// ============================================================
class AggCircleNode : public TextureNode {
public:
  AggCircleNode();
  std::string typeName() const override { return "AggCircle"; }
  std::string displayTitle() const override { return "Circle"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture *> &inputs,
               std::vector<GenTexture> &outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json &j) override;

private:
  int m_widthIdx, m_heightIdx;
  float m_cx, m_cy, m_rx, m_ry;
  float m_thickness;
  float m_fillColor[4];
  float m_strokeColor[4];
  bool m_filled, m_stroked;
};

// ============================================================
// AggRectNode - draws a (rounded) rectangle using AGG
// ============================================================
class AggRectNode : public TextureNode {
public:
  AggRectNode();
  std::string typeName() const override { return "AggRect"; }
  std::string displayTitle() const override { return "Rect"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture *> &inputs,
               std::vector<GenTexture> &outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json &j) override;

private:
  int m_widthIdx, m_heightIdx;
  float m_x1, m_y1, m_x2, m_y2;
  float m_cornerRadius;
  float m_thickness;
  float m_fillColor[4];
  float m_strokeColor[4];
  bool m_filled, m_stroked;
};

// ============================================================
// AggPolygonNode - draws a regular polygon / star using AGG
// ============================================================
class AggPolygonNode : public TextureNode {
public:
  AggPolygonNode();
  std::string typeName() const override { return "AggPolygon"; }
  std::string displayTitle() const override { return "Polygon"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture *> &inputs,
               std::vector<GenTexture> &outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json &j) override;

private:
  int m_widthIdx, m_heightIdx;
  float m_cx, m_cy, m_radius;
  float m_innerRadius; // 0..1 ratio of radius, <1 = star
  int m_sides;
  float m_rotation;
  float m_thickness;
  float m_fillColor[4];
  float m_strokeColor[4];
  bool m_filled, m_stroked;
};

// ============================================================
// AggTextNode - draws text using AGG embedded font
// ============================================================
class AggTextNode : public TextureNode {
public:
  AggTextNode();
  std::string typeName() const override { return "AggText"; }
  std::string displayTitle() const override { return "Text"; }
  std::vector<ImNodes::Ez::SlotInfo> inputSlotInfos() const override;
  std::vector<ImNodes::Ez::SlotInfo> outputSlotInfos() const override;
  std::vector<std::string> inputSlotNames() const override;
  std::vector<std::string> outputSlotNames() const override;
  void execute(const std::vector<GenTexture *> &inputs,
               std::vector<GenTexture> &outputs) override;
  void renderParams() override;
  nlohmann::json saveParams() const override;
  void loadParams(const nlohmann::json &j) override;

private:
  int m_widthIdx, m_heightIdx;
  char m_text[256];
  float m_x, m_y;
  float m_height;
  float m_thickness;
  float m_color[4];
  float m_bgColor[4];
};
