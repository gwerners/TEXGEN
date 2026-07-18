#include "AggNodes.h"
#include <imgui.h>
#include "UiWidgets.h"
#include <cstring>

static const char* s_blendModes =
    "Over\0Add\0Sub\0Multiply\0Min\0Max\0Composite\0MulColor\0Screen\0"
    "Darken\0Lighten\0";

static const char* s_sizesStr = "32\00064\000128\000256\000512\0001024\000";

static void renderBlendCombo(const char* id, int* blendMode) {
  ImGui::Combo(id, blendMode, s_blendModes);
}



// ============================================================
// AggLineNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggLineNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggLineNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggLineNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##ln", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##ln", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("X1##ln", &m_core.m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##ln", &m_core.m_y1, 0.0f, 1.0f);
  SliderFloatW("X2##ln", &m_core.m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##ln", &m_core.m_y2, 0.0f, 1.0f);
  SliderFloatW("Thick##ln", &m_core.m_thickness, 0.5f, 50.0f);
  ImGui::ColorEdit4("Color##ln", m_core.m_color, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("BgCol##ln", m_core.m_bgColor,
                    ImGuiColorEditFlags_NoInputs);
  renderBlendCombo("Blend##ln", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggCircleNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggCircleNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggCircleNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggCircleNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##ci", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##ci", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("CX##ci", &m_core.m_cx, 0.0f, 1.0f);
  SliderFloatW("CY##ci", &m_core.m_cy, 0.0f, 1.0f);
  SliderFloatW("RX##ci", &m_core.m_rx, 0.0f, 1.0f);
  SliderFloatW("RY##ci", &m_core.m_ry, 0.0f, 1.0f);
  ImGui::Checkbox("Fill##ci", &m_core.m_filled);
  if (m_core.m_filled)
    ImGui::ColorEdit4("FillCol##ci", m_core.m_fillColor,
                      ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Stroke##ci", &m_core.m_stroked);
  if (m_core.m_stroked) {
    SliderFloatW("Thick##ci", &m_core.m_thickness, 0.5f, 50.0f);
    ImGui::ColorEdit4("StrokeCol##ci", m_core.m_strokeColor,
                      ImGuiColorEditFlags_NoInputs);
  }
  renderBlendCombo("Blend##ci", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggRectNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggRectNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggRectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggRectNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##rc", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##rc", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("X1##rc", &m_core.m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##rc", &m_core.m_y1, 0.0f, 1.0f);
  SliderFloatW("X2##rc", &m_core.m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##rc", &m_core.m_y2, 0.0f, 1.0f);
  SliderFloatW("Corner##rc", &m_core.m_cornerRadius, 0.0f, 1.0f);
  ImGui::Checkbox("Fill##rc", &m_core.m_filled);
  if (m_core.m_filled)
    ImGui::ColorEdit4("FillCol##rc", m_core.m_fillColor,
                      ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Stroke##rc", &m_core.m_stroked);
  if (m_core.m_stroked) {
    SliderFloatW("Thick##rc", &m_core.m_thickness, 0.5f, 50.0f);
    ImGui::ColorEdit4("StrokeCol##rc", m_core.m_strokeColor,
                      ImGuiColorEditFlags_NoInputs);
  }
  renderBlendCombo("Blend##rc", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggPolygonNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggPolygonNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggPolygonNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggPolygonNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##pg", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##pg", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("CX##pg", &m_core.m_cx, 0.0f, 1.0f);
  SliderFloatW("CY##pg", &m_core.m_cy, 0.0f, 1.0f);
  SliderFloatW("Radius##pg", &m_core.m_radius, 0.01f, 1.0f);
  SliderIntW("Sides##pg", &m_core.m_sides, 3, 32);
  SliderFloatW("Inner##pg", &m_core.m_innerRadius, 0.1f, 1.0f);
  SliderFloatW("Rot##pg", &m_core.m_rotation, 0.0f, 360.0f);
  ImGui::Checkbox("Fill##pg", &m_core.m_filled);
  if (m_core.m_filled)
    ImGui::ColorEdit4("FillCol##pg", m_core.m_fillColor,
                      ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Stroke##pg", &m_core.m_stroked);
  if (m_core.m_stroked) {
    SliderFloatW("Thick##pg", &m_core.m_thickness, 0.5f, 50.0f);
    ImGui::ColorEdit4("StrokeCol##pg", m_core.m_strokeColor,
                      ImGuiColorEditFlags_NoInputs);
  }
  renderBlendCombo("Blend##pg", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggTextNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggTextNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggTextNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggTextNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##tx", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##tx", &m_core.m_heightIdx, s_sizesStr);
  ImGui::PopItemWidth();
  ImGui::PushItemWidth(200);
  ImGui::InputText("Text##tx", m_core.m_text, sizeof(m_core.m_text));
  ImGui::PopItemWidth();
  ImGui::PushItemWidth(100);
  SliderFloatW("X##tx", &m_core.m_x, 0.0f, 1.0f);
  SliderFloatW("Y##tx", &m_core.m_y, 0.0f, 1.0f);
  SliderFloatW("Size##tx", &m_core.m_height, 5.0f, 200.0f);
  SliderFloatW("Thick##tx", &m_core.m_thickness, 0.5f, 10.0f);
  ImGui::ColorEdit4("Color##tx", m_core.m_color, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("BgCol##tx", m_core.m_bgColor,
                    ImGuiColorEditFlags_NoInputs);
  renderBlendCombo("Blend##tx", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggArcNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggArcNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggArcNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggArcNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##ar", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##ar", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("CX##ar", &m_core.m_cx, 0.0f, 1.0f);
  SliderFloatW("CY##ar", &m_core.m_cy, 0.0f, 1.0f);
  SliderFloatW("RX##ar", &m_core.m_rx, 0.0f, 1.0f);
  SliderFloatW("RY##ar", &m_core.m_ry, 0.0f, 1.0f);
  SliderFloatW("Start##ar", &m_core.m_angle1, 0.0f, 360.0f);
  SliderFloatW("End##ar", &m_core.m_angle2, 0.0f, 360.0f);
  SliderFloatW("Thick##ar", &m_core.m_thickness, 0.5f, 50.0f);
  ImGui::ColorEdit4("Color##ar", m_core.m_color, ImGuiColorEditFlags_NoInputs);
  renderBlendCombo("Blend##ar", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggBezierNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggBezierNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggBezierNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggBezierNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##bz", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##bz", &m_core.m_heightIdx, s_sizesStr);
  ImGui::Text("Start");
  SliderFloatW("X1##bz", &m_core.m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##bz", &m_core.m_y1, 0.0f, 1.0f);
  ImGui::Text("Ctrl 1");
  SliderFloatW("CX1##bz", &m_core.m_cx1, 0.0f, 1.0f);
  SliderFloatW("CY1##bz", &m_core.m_cy1, 0.0f, 1.0f);
  ImGui::Text("Ctrl 2");
  SliderFloatW("CX2##bz", &m_core.m_cx2, 0.0f, 1.0f);
  SliderFloatW("CY2##bz", &m_core.m_cy2, 0.0f, 1.0f);
  ImGui::Text("End");
  SliderFloatW("X2##bz", &m_core.m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##bz", &m_core.m_y2, 0.0f, 1.0f);
  SliderFloatW("Thick##bz", &m_core.m_thickness, 0.5f, 50.0f);
  ImGui::ColorEdit4("Color##bz", m_core.m_color, ImGuiColorEditFlags_NoInputs);
  renderBlendCombo("Blend##bz", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggDashLineNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggDashLineNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggDashLineNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggDashLineNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##dl", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##dl", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("X1##dl", &m_core.m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##dl", &m_core.m_y1, 0.0f, 1.0f);
  SliderFloatW("X2##dl", &m_core.m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##dl", &m_core.m_y2, 0.0f, 1.0f);
  SliderFloatW("Thick##dl", &m_core.m_thickness, 0.5f, 50.0f);
  SliderFloatW("Dash##dl", &m_core.m_dashLen, 1.0f, 100.0f);
  SliderFloatW("Gap##dl", &m_core.m_gapLen, 1.0f, 100.0f);
  ImGui::ColorEdit4("Color##dl", m_core.m_color, ImGuiColorEditFlags_NoInputs);
  renderBlendCombo("Blend##dl", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}

// ============================================================
// AggGradientNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AggGradientNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> AggGradientNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void AggGradientNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##gf", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##gf", &m_core.m_heightIdx, s_sizesStr);
  static const char* types = "Linear\0Radial\0";
  ImGui::Combo("Type##gf", &m_core.m_type, types);
  if (m_core.m_type == 0) {
    ImGui::Text("Start");
    SliderFloatW("X1##gf", &m_core.m_x1, 0.0f, 1.0f);
    SliderFloatW("Y1##gf", &m_core.m_y1, 0.0f, 1.0f);
    ImGui::Text("End");
    SliderFloatW("X2##gf", &m_core.m_x2, 0.0f, 1.0f);
    SliderFloatW("Y2##gf", &m_core.m_y2, 0.0f, 1.0f);
  } else {
    ImGui::Text("Center");
    SliderFloatW("CX##gf", &m_core.m_x1, 0.0f, 1.0f);
    SliderFloatW("CY##gf", &m_core.m_y1, 0.0f, 1.0f);
    ImGui::Text("Edge");
    SliderFloatW("EX##gf", &m_core.m_x2, 0.0f, 1.0f);
    SliderFloatW("EY##gf", &m_core.m_y2, 0.0f, 1.0f);
  }
  ImGui::ColorEdit4("Color1##gf", m_core.m_color1,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##gf", m_core.m_color2,
                    ImGuiColorEditFlags_NoInputs);
  renderBlendCombo("Blend##gf", &m_core.m_blendMode);
  ImGui::PopItemWidth();
}
