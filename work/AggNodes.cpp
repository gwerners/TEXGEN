#include "AggNodes.h"
#include <imgui.h>
#include <cmath>
#include <cstring>
#include "agg_conv_dash.h"
#include "agg_gentexture.h"
#include "agg_gsv_text.h"

// ============================================================
// Shared helpers
// ============================================================

static const char* s_sizesStr = "32\00064\000128\000256\000512\0001024\000";
static int indexToSize(int idx) {
  return 32 << idx;
}

// Set up output buffer: copy background input or clear to color.
static void prepareOutput(std::vector<GenTexture>& outputs,
                          const std::vector<GenTexture*>& inputs,
                          int w,
                          int h,
                          const float bgColor[4]) {
  outputs.resize(1);
  outputs[0].Init(w, h);
  if (!inputs.empty() && inputs[0] && inputs[0]->Data && inputs[0]->XRes == w &&
      inputs[0]->YRes == h) {
    memcpy(outputs[0].Data, inputs[0]->Data, w * h * sizeof(Pixel));
  } else {
    auto rbuf = agg_rbuf_from(outputs[0]);
    AggPixfmt pixf(rbuf);
    AggRendererBase ren(pixf);
    ren.clear(agg_color(bgColor));
  }
}

// Rasterize a filled path.
template <class Path>
static void fillPath(AggRendererBase& ren, Path& path, const float col[4]) {
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  AggRendererSolid solid(ren);
  ras.add_path(path);
  solid.color(agg_color(col));
  agg::render_scanlines(ras, sl, solid);
}

// Rasterize a stroked path.
template <class Path>
static void strokePath(AggRendererBase& ren,
                       Path& path,
                       const float col[4],
                       double thickness) {
  agg::conv_stroke<Path> stroke(path);
  stroke.width(thickness);
  stroke.line_cap(agg::round_cap);
  stroke.line_join(agg::round_join);
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  AggRendererSolid solid(ren);
  ras.add_path(stroke);
  solid.color(agg_color(col));
  agg::render_scanlines(ras, sl, solid);
}

// ImGui slider with mouse wheel support.
static bool SliderFloatW(const char* label,
                         float* v,
                         float mn,
                         float mx,
                         float step = 0.0f) {
  bool changed = ImGui::SliderFloat(label, v, mn, mx);
  if (ImGui::IsItemHovered()) {
    if (step <= 0.0f)
      step = (mx - mn) * 0.01f;
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      *v += wheel * step;
      if (*v < mn)
        *v = mn;
      if (*v > mx)
        *v = mx;
      changed = true;
    }
  }
  return changed;
}

static bool SliderIntW(const char* label,
                       int* v,
                       int mn,
                       int mx,
                       int step = 1) {
  bool changed = ImGui::SliderInt(label, v, mn, mx);
  if (ImGui::IsItemHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      *v += (int)wheel * step;
      if (*v < mn)
        *v = mn;
      if (*v > mx)
        *v = mx;
      changed = true;
    }
  }
  return changed;
}

// ============================================================
// AggLineNode
// ============================================================

AggLineNode::AggLineNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.2f),
      m_y1(0.2f),
      m_x2(0.8f),
      m_y2(0.8f),
      m_thickness(3.0f) {
  m_color[0] = 1.0f;
  m_color[1] = 1.0f;
  m_color[2] = 1.0f;
  m_color[3] = 1.0f;
  m_bgColor[0] = 0.0f;
  m_bgColor[1] = 0.0f;
  m_bgColor[2] = 0.0f;
  m_bgColor[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> AggLineNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> AggLineNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> AggLineNode::inputSlotNames() const {
  return {"Bg"};
}
std::vector<std::string> AggLineNode::outputSlotNames() const {
  return {"Out"};
}

void AggLineNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_bgColor);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::path_storage path;
  path.move_to(m_x1 * w, aggY(m_y1, h));
  path.line_to(m_x2 * w, aggY(m_y2, h));
  strokePath(ren, path, m_color, m_thickness);
}

void AggLineNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##ln", &m_widthIdx, s_sizesStr);
  ImGui::Combo("H##ln", &m_heightIdx, s_sizesStr);
  SliderFloatW("X1##ln", &m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##ln", &m_y1, 0.0f, 1.0f);
  SliderFloatW("X2##ln", &m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##ln", &m_y2, 0.0f, 1.0f);
  SliderFloatW("Thick##ln", &m_thickness, 0.5f, 50.0f);
  ImGui::ColorEdit4("Color##ln", m_color, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("BgCol##ln", m_bgColor, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

nlohmann::json AggLineNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"x1", m_x1},
          {"y1", m_y1},
          {"x2", m_x2},
          {"y2", m_y2},
          {"thickness", m_thickness},
          {"cr", m_color[0]},
          {"cg", m_color[1]},
          {"cb", m_color[2]},
          {"ca", m_color[3]},
          {"bgr", m_bgColor[0]},
          {"bgg", m_bgColor[1]},
          {"bgb", m_bgColor[2]},
          {"bga", m_bgColor[3]}};
}

void AggLineNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("x1"))
    m_x1 = j["x1"];
  if (j.contains("y1"))
    m_y1 = j["y1"];
  if (j.contains("x2"))
    m_x2 = j["x2"];
  if (j.contains("y2"))
    m_y2 = j["y2"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("cr"))
    m_color[0] = j["cr"];
  if (j.contains("cg"))
    m_color[1] = j["cg"];
  if (j.contains("cb"))
    m_color[2] = j["cb"];
  if (j.contains("ca"))
    m_color[3] = j["ca"];
  if (j.contains("bgr"))
    m_bgColor[0] = j["bgr"];
  if (j.contains("bgg"))
    m_bgColor[1] = j["bgg"];
  if (j.contains("bgb"))
    m_bgColor[2] = j["bgb"];
  if (j.contains("bga"))
    m_bgColor[3] = j["bga"];
}

// ============================================================
// AggCircleNode
// ============================================================

AggCircleNode::AggCircleNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_cx(0.5f),
      m_cy(0.5f),
      m_rx(0.3f),
      m_ry(0.3f),
      m_thickness(3.0f),
      m_filled(true),
      m_stroked(false) {
  m_fillColor[0] = 1.0f;
  m_fillColor[1] = 1.0f;
  m_fillColor[2] = 1.0f;
  m_fillColor[3] = 1.0f;
  m_strokeColor[0] = 1.0f;
  m_strokeColor[1] = 1.0f;
  m_strokeColor[2] = 1.0f;
  m_strokeColor[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> AggCircleNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> AggCircleNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> AggCircleNode::inputSlotNames() const {
  return {"Bg"};
}
std::vector<std::string> AggCircleNode::outputSlotNames() const {
  return {"Out"};
}

void AggCircleNode::execute(const std::vector<GenTexture*>& inputs,
                            std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  float bgBlack[] = {0, 0, 0, 1};
  prepareOutput(outputs, inputs, w, h, bgBlack);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::ellipse ell(m_cx * w, aggY(m_cy, h), m_rx * w, m_ry * h, 100);
  if (m_filled)
    fillPath(ren, ell, m_fillColor);
  if (m_stroked)
    strokePath(ren, ell, m_strokeColor, m_thickness);
}

void AggCircleNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##ci", &m_widthIdx, s_sizesStr);
  ImGui::Combo("H##ci", &m_heightIdx, s_sizesStr);
  SliderFloatW("CX##ci", &m_cx, 0.0f, 1.0f);
  SliderFloatW("CY##ci", &m_cy, 0.0f, 1.0f);
  SliderFloatW("RX##ci", &m_rx, 0.0f, 1.0f);
  SliderFloatW("RY##ci", &m_ry, 0.0f, 1.0f);
  ImGui::Checkbox("Fill##ci", &m_filled);
  if (m_filled)
    ImGui::ColorEdit4("FillCol##ci", m_fillColor, ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Stroke##ci", &m_stroked);
  if (m_stroked) {
    SliderFloatW("Thick##ci", &m_thickness, 0.5f, 50.0f);
    ImGui::ColorEdit4("StrokeCol##ci", m_strokeColor,
                      ImGuiColorEditFlags_NoInputs);
  }
  ImGui::PopItemWidth();
}

nlohmann::json AggCircleNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"cx", m_cx},
          {"cy", m_cy},
          {"rx", m_rx},
          {"ry", m_ry},
          {"thickness", m_thickness},
          {"filled", m_filled},
          {"stroked", m_stroked},
          {"fr", m_fillColor[0]},
          {"fg", m_fillColor[1]},
          {"fb", m_fillColor[2]},
          {"fa", m_fillColor[3]},
          {"sr", m_strokeColor[0]},
          {"sg", m_strokeColor[1]},
          {"sb", m_strokeColor[2]},
          {"sa", m_strokeColor[3]}};
}

void AggCircleNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("cx"))
    m_cx = j["cx"];
  if (j.contains("cy"))
    m_cy = j["cy"];
  if (j.contains("rx"))
    m_rx = j["rx"];
  if (j.contains("ry"))
    m_ry = j["ry"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("filled"))
    m_filled = j["filled"];
  if (j.contains("stroked"))
    m_stroked = j["stroked"];
  if (j.contains("fr"))
    m_fillColor[0] = j["fr"];
  if (j.contains("fg"))
    m_fillColor[1] = j["fg"];
  if (j.contains("fb"))
    m_fillColor[2] = j["fb"];
  if (j.contains("fa"))
    m_fillColor[3] = j["fa"];
  if (j.contains("sr"))
    m_strokeColor[0] = j["sr"];
  if (j.contains("sg"))
    m_strokeColor[1] = j["sg"];
  if (j.contains("sb"))
    m_strokeColor[2] = j["sb"];
  if (j.contains("sa"))
    m_strokeColor[3] = j["sa"];
}

// ============================================================
// AggRectNode
// ============================================================

AggRectNode::AggRectNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.1f),
      m_y1(0.1f),
      m_x2(0.9f),
      m_y2(0.9f),
      m_cornerRadius(0.0f),
      m_thickness(3.0f),
      m_filled(true),
      m_stroked(false) {
  m_fillColor[0] = 1.0f;
  m_fillColor[1] = 1.0f;
  m_fillColor[2] = 1.0f;
  m_fillColor[3] = 1.0f;
  m_strokeColor[0] = 1.0f;
  m_strokeColor[1] = 1.0f;
  m_strokeColor[2] = 1.0f;
  m_strokeColor[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> AggRectNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> AggRectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> AggRectNode::inputSlotNames() const {
  return {"Bg"};
}
std::vector<std::string> AggRectNode::outputSlotNames() const {
  return {"Out"};
}

void AggRectNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  float bgBlack[] = {0, 0, 0, 1};
  prepareOutput(outputs, inputs, w, h, bgBlack);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  double px1 = m_x1 * w, py1 = aggY(m_y1, h);
  double px2 = m_x2 * w, py2 = aggY(m_y2, h);
  double rad = m_cornerRadius * sMin(w, h) * 0.5;

  agg::rounded_rect rect(px1, py1, px2, py2, rad);
  rect.normalize_radius();

  if (m_filled)
    fillPath(ren, rect, m_fillColor);
  if (m_stroked)
    strokePath(ren, rect, m_strokeColor, m_thickness);
}

void AggRectNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##rc", &m_widthIdx, s_sizesStr);
  ImGui::Combo("H##rc", &m_heightIdx, s_sizesStr);
  SliderFloatW("X1##rc", &m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##rc", &m_y1, 0.0f, 1.0f);
  SliderFloatW("X2##rc", &m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##rc", &m_y2, 0.0f, 1.0f);
  SliderFloatW("Corner##rc", &m_cornerRadius, 0.0f, 1.0f);
  ImGui::Checkbox("Fill##rc", &m_filled);
  if (m_filled)
    ImGui::ColorEdit4("FillCol##rc", m_fillColor, ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Stroke##rc", &m_stroked);
  if (m_stroked) {
    SliderFloatW("Thick##rc", &m_thickness, 0.5f, 50.0f);
    ImGui::ColorEdit4("StrokeCol##rc", m_strokeColor,
                      ImGuiColorEditFlags_NoInputs);
  }
  ImGui::PopItemWidth();
}

nlohmann::json AggRectNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"x1", m_x1},
          {"y1", m_y1},
          {"x2", m_x2},
          {"y2", m_y2},
          {"cornerRadius", m_cornerRadius},
          {"thickness", m_thickness},
          {"filled", m_filled},
          {"stroked", m_stroked},
          {"fr", m_fillColor[0]},
          {"fg", m_fillColor[1]},
          {"fb", m_fillColor[2]},
          {"fa", m_fillColor[3]},
          {"sr", m_strokeColor[0]},
          {"sg", m_strokeColor[1]},
          {"sb", m_strokeColor[2]},
          {"sa", m_strokeColor[3]}};
}

void AggRectNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("x1"))
    m_x1 = j["x1"];
  if (j.contains("y1"))
    m_y1 = j["y1"];
  if (j.contains("x2"))
    m_x2 = j["x2"];
  if (j.contains("y2"))
    m_y2 = j["y2"];
  if (j.contains("cornerRadius"))
    m_cornerRadius = j["cornerRadius"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("filled"))
    m_filled = j["filled"];
  if (j.contains("stroked"))
    m_stroked = j["stroked"];
  if (j.contains("fr"))
    m_fillColor[0] = j["fr"];
  if (j.contains("fg"))
    m_fillColor[1] = j["fg"];
  if (j.contains("fb"))
    m_fillColor[2] = j["fb"];
  if (j.contains("fa"))
    m_fillColor[3] = j["fa"];
  if (j.contains("sr"))
    m_strokeColor[0] = j["sr"];
  if (j.contains("sg"))
    m_strokeColor[1] = j["sg"];
  if (j.contains("sb"))
    m_strokeColor[2] = j["sb"];
  if (j.contains("sa"))
    m_strokeColor[3] = j["sa"];
}

// ============================================================
// AggPolygonNode
// ============================================================

AggPolygonNode::AggPolygonNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_cx(0.5f),
      m_cy(0.5f),
      m_radius(0.4f),
      m_innerRadius(1.0f),
      m_sides(6),
      m_rotation(0.0f),
      m_thickness(3.0f),
      m_filled(true),
      m_stroked(false) {
  m_fillColor[0] = 1.0f;
  m_fillColor[1] = 1.0f;
  m_fillColor[2] = 1.0f;
  m_fillColor[3] = 1.0f;
  m_strokeColor[0] = 1.0f;
  m_strokeColor[1] = 1.0f;
  m_strokeColor[2] = 1.0f;
  m_strokeColor[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> AggPolygonNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> AggPolygonNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> AggPolygonNode::inputSlotNames() const {
  return {"Bg"};
}
std::vector<std::string> AggPolygonNode::outputSlotNames() const {
  return {"Out"};
}

void AggPolygonNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  float bgBlack[] = {0, 0, 0, 1};
  prepareOutput(outputs, inputs, w, h, bgBlack);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  double cx = m_cx * w, cy = aggY(m_cy, h);
  double r = m_radius * sMin(w, h) * 0.5;
  double ri = r * m_innerRadius;
  double rotRad = m_rotation * sPI / 180.0;

  agg::path_storage path;
  bool isStar = m_innerRadius < 0.999f;
  int n = isStar ? m_sides * 2 : m_sides;

  for (int i = 0; i < n; i++) {
    double angle = rotRad + (2.0 * sPI * i) / n - sPI * 0.5;
    double rad = (isStar && (i & 1)) ? ri : r;
    double px = cx + cos(angle) * rad;
    double py = cy + sin(angle) * rad;
    if (i == 0)
      path.move_to(px, py);
    else
      path.line_to(px, py);
  }
  path.close_polygon();

  if (m_filled)
    fillPath(ren, path, m_fillColor);
  if (m_stroked)
    strokePath(ren, path, m_strokeColor, m_thickness);
}

void AggPolygonNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##pg", &m_widthIdx, s_sizesStr);
  ImGui::Combo("H##pg", &m_heightIdx, s_sizesStr);
  SliderFloatW("CX##pg", &m_cx, 0.0f, 1.0f);
  SliderFloatW("CY##pg", &m_cy, 0.0f, 1.0f);
  SliderFloatW("Radius##pg", &m_radius, 0.01f, 1.0f);
  SliderIntW("Sides##pg", &m_sides, 3, 32);
  SliderFloatW("Inner##pg", &m_innerRadius, 0.1f, 1.0f);
  SliderFloatW("Rot##pg", &m_rotation, 0.0f, 360.0f);
  ImGui::Checkbox("Fill##pg", &m_filled);
  if (m_filled)
    ImGui::ColorEdit4("FillCol##pg", m_fillColor, ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Stroke##pg", &m_stroked);
  if (m_stroked) {
    SliderFloatW("Thick##pg", &m_thickness, 0.5f, 50.0f);
    ImGui::ColorEdit4("StrokeCol##pg", m_strokeColor,
                      ImGuiColorEditFlags_NoInputs);
  }
  ImGui::PopItemWidth();
}

nlohmann::json AggPolygonNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"cx", m_cx},
          {"cy", m_cy},
          {"radius", m_radius},
          {"innerRadius", m_innerRadius},
          {"sides", m_sides},
          {"rotation", m_rotation},
          {"thickness", m_thickness},
          {"filled", m_filled},
          {"stroked", m_stroked},
          {"fr", m_fillColor[0]},
          {"fg", m_fillColor[1]},
          {"fb", m_fillColor[2]},
          {"fa", m_fillColor[3]},
          {"sr", m_strokeColor[0]},
          {"sg", m_strokeColor[1]},
          {"sb", m_strokeColor[2]},
          {"sa", m_strokeColor[3]}};
}

void AggPolygonNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("cx"))
    m_cx = j["cx"];
  if (j.contains("cy"))
    m_cy = j["cy"];
  if (j.contains("radius"))
    m_radius = j["radius"];
  if (j.contains("innerRadius"))
    m_innerRadius = j["innerRadius"];
  if (j.contains("sides"))
    m_sides = j["sides"];
  if (j.contains("rotation"))
    m_rotation = j["rotation"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("filled"))
    m_filled = j["filled"];
  if (j.contains("stroked"))
    m_stroked = j["stroked"];
  if (j.contains("fr"))
    m_fillColor[0] = j["fr"];
  if (j.contains("fg"))
    m_fillColor[1] = j["fg"];
  if (j.contains("fb"))
    m_fillColor[2] = j["fb"];
  if (j.contains("fa"))
    m_fillColor[3] = j["fa"];
  if (j.contains("sr"))
    m_strokeColor[0] = j["sr"];
  if (j.contains("sg"))
    m_strokeColor[1] = j["sg"];
  if (j.contains("sb"))
    m_strokeColor[2] = j["sb"];
  if (j.contains("sa"))
    m_strokeColor[3] = j["sa"];
}

// ============================================================
// AggTextNode
// ============================================================

AggTextNode::AggTextNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x(0.1f),
      m_y(0.5f),
      m_height(30.0f),
      m_thickness(1.5f) {
  strcpy(m_text, "AGG");
  m_color[0] = 1.0f;
  m_color[1] = 1.0f;
  m_color[2] = 1.0f;
  m_color[3] = 1.0f;
  m_bgColor[0] = 0.0f;
  m_bgColor[1] = 0.0f;
  m_bgColor[2] = 0.0f;
  m_bgColor[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> AggTextNode::inputSlotInfos() const {
  return {{"Bg", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> AggTextNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> AggTextNode::inputSlotNames() const {
  return {"Bg"};
}
std::vector<std::string> AggTextNode::outputSlotNames() const {
  return {"Out"};
}

void AggTextNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_bgColor);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::gsv_text text;
  text.size(m_height);
  text.start_point(m_x * w, aggY(m_y, h));
  text.text(m_text);

  agg::conv_stroke<agg::gsv_text> stroke(text);
  stroke.width(m_thickness);
  stroke.line_cap(agg::round_cap);

  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  AggRendererSolid solid(ren);
  ras.add_path(stroke);
  solid.color(agg_color(m_color));
  agg::render_scanlines(ras, sl, solid);
}

void AggTextNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##tx", &m_widthIdx, s_sizesStr);
  ImGui::Combo("H##tx", &m_heightIdx, s_sizesStr);
  ImGui::PopItemWidth();
  ImGui::PushItemWidth(200);
  ImGui::InputText("Text##tx", m_text, sizeof(m_text));
  ImGui::PopItemWidth();
  ImGui::PushItemWidth(100);
  SliderFloatW("X##tx", &m_x, 0.0f, 1.0f);
  SliderFloatW("Y##tx", &m_y, 0.0f, 1.0f);
  SliderFloatW("Size##tx", &m_height, 5.0f, 200.0f);
  SliderFloatW("Thick##tx", &m_thickness, 0.5f, 10.0f);
  ImGui::ColorEdit4("Color##tx", m_color, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("BgCol##tx", m_bgColor, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

nlohmann::json AggTextNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"text", m_text},
          {"x", m_x},
          {"y", m_y},
          {"height", m_height},
          {"thickness", m_thickness},
          {"cr", m_color[0]},
          {"cg", m_color[1]},
          {"cb", m_color[2]},
          {"ca", m_color[3]},
          {"bgr", m_bgColor[0]},
          {"bgg", m_bgColor[1]},
          {"bgb", m_bgColor[2]},
          {"bga", m_bgColor[3]}};
}

void AggTextNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("text")) {
    std::string s = j["text"];
    strncpy(m_text, s.c_str(), sizeof(m_text) - 1);
    m_text[sizeof(m_text) - 1] = '\0';
  }
  if (j.contains("x"))
    m_x = j["x"];
  if (j.contains("y"))
    m_y = j["y"];
  if (j.contains("height"))
    m_height = j["height"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("cr"))
    m_color[0] = j["cr"];
  if (j.contains("cg"))
    m_color[1] = j["cg"];
  if (j.contains("cb"))
    m_color[2] = j["cb"];
  if (j.contains("ca"))
    m_color[3] = j["ca"];
  if (j.contains("bgr"))
    m_bgColor[0] = j["bgr"];
  if (j.contains("bgg"))
    m_bgColor[1] = j["bgg"];
  if (j.contains("bgb"))
    m_bgColor[2] = j["bgb"];
  if (j.contains("bga"))
    m_bgColor[3] = j["bga"];
}
