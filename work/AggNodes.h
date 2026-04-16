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
  int m_blendMode;
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
  int m_blendMode;
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
  int m_blendMode;
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
  float m_innerRadius;
  int m_sides;
  float m_rotation;
  float m_thickness;
  float m_fillColor[4];
  float m_strokeColor[4];
  bool m_filled, m_stroked;
  int m_blendMode;
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
  int m_blendMode;
};

// ============================================================
// AggArcNode - draws an arc (partial ellipse) using AGG
// ============================================================
class AggArcNode : public TextureNode {
public:
  AggArcNode();
  std::string typeName() const override { return "AggArc"; }
  std::string displayTitle() const override { return "Arc"; }
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
  float m_angle1, m_angle2;
  float m_thickness;
  float m_color[4];
  int m_blendMode;
};

// ============================================================
// AggBezierNode - draws a cubic bezier curve using AGG
// ============================================================
class AggBezierNode : public TextureNode {
public:
  AggBezierNode();
  std::string typeName() const override { return "AggBezier"; }
  std::string displayTitle() const override { return "Bezier"; }
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
  float m_x1, m_y1;
  float m_cx1, m_cy1;
  float m_cx2, m_cy2;
  float m_x2, m_y2;
  float m_thickness;
  float m_color[4];
  int m_blendMode;
};

// ============================================================
// AggDashLineNode - draws a dashed line using AGG
// ============================================================
class AggDashLineNode : public TextureNode {
public:
  AggDashLineNode();
  std::string typeName() const override { return "AggDashLine"; }
  std::string displayTitle() const override { return "DashLine"; }
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
  float m_dashLen, m_gapLen;
  float m_color[4];
  int m_blendMode;
};

// ============================================================
// AggGradientNode - fills with linear or radial gradient using AGG
// ============================================================
class AggGradientNode : public TextureNode {
public:
  AggGradientNode();
  std::string typeName() const override { return "AggGradient"; }
  std::string displayTitle() const override { return "GradientFill"; }
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
  int m_type; // 0=linear, 1=radial
  float m_x1, m_y1, m_x2, m_y2;
  float m_color1[4];
  float m_color2[4];
  int m_blendMode;
};
