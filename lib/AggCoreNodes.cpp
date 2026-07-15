#include "AggCoreNodes.h"

#include <cmath>
#include <cstring>
#include "agg_arc.h"
#include "agg_conv_dash.h"
#include "agg_curves.h"
#include "agg_gentexture.h"
#include "agg_gradient_lut.h"
#include "agg_gsv_text.h"
#include "agg_span_allocator.h"
#include "agg_span_gradient.h"
#include "agg_span_interpolator_linear.h"

// ============================================================
// Shared helpers
// ============================================================


static int indexToSize(int idx) {
  return 32 << idx;
}

// Blend mode names for combo (0 = Over/Replace, rest are GenTexture combine
// ops)


static const GenTexture::CombineOp s_blendOps[] = {
    GenTexture::CombineOver,    GenTexture::CombineAdd,
    GenTexture::CombineSub,     GenTexture::CombineMulC,
    GenTexture::CombineMin,     GenTexture::CombineMax,
    GenTexture::CombineOver,    GenTexture::CombineMultiply,
    GenTexture::CombineScreen,  GenTexture::CombineDarken,
    GenTexture::CombineLighten,
};

// Set up output buffer for AGG rendering.
// blendMode 0 (Over): copy Bg first, AGG draws directly on top (correct alpha).
// blendMode > 0: start with transparent black, blend after AGG draws.
static void prepareOutput(std::vector<GenTexture>& outputs,
                          const std::vector<GenTexture*>& inputs,
                          int w,
                          int h,
                          int blendMode) {
  outputs.resize(1);
  outputs[0].Init(w, h);
  if (blendMode == 0 && !inputs.empty() && inputs[0] && inputs[0]->Data &&
      inputs[0]->XRes == w && inputs[0]->YRes == h) {
    // Over: copy background, AGG draws on top with its own alpha blending
    memcpy(outputs[0].Data, inputs[0]->Data, w * h * sizeof(Pixel));
  } else {
    // Other modes: transparent black, will Paste-blend after
    memset(outputs[0].Data, 0, w * h * sizeof(Pixel));
  }
}

// After AGG rendering, blend the result with the background input.
// Only needed for blendMode > 0 (Over is handled in prepareOutput).
static void applyAggBlend(std::vector<GenTexture>& outputs,
                          const std::vector<GenTexture*>& inputs,
                          int blendMode) {
  if (blendMode == 0)
    return;  // Already composited in prepareOutput
  bool hasBg = !inputs.empty() && inputs[0] && inputs[0]->Data;
  if (!hasBg)
    return;
  if (!outputs[0].SizeMatchesWith(*inputs[0]))
    return;

  {
    int idx = blendMode;
    if (idx < 0 || idx >= (int)(sizeof(s_blendOps) / sizeof(s_blendOps[0])))
      return;
    GenTexture result;
    result.Init(outputs[0].XRes, outputs[0].YRes);
    result.Paste(*inputs[0], outputs[0], 0, 0, 1, 0, 0, 1, s_blendOps[idx], 0);
    outputs[0] = result;
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

// ============================================================
// AggLineCoreNode
// ============================================================

AggLineCoreNode::AggLineCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.2f),
      m_y1(0.2f),
      m_x2(0.8f),
      m_y2(0.8f),
      m_thickness(3.0f),
      m_blendMode(0) {
  m_color[0] = 1.0f;
  m_color[1] = 1.0f;
  m_color[2] = 1.0f;
  m_color[3] = 1.0f;
  m_bgColor[0] = 0.0f;
  m_bgColor[1] = 0.0f;
  m_bgColor[2] = 0.0f;
  m_bgColor[3] = 1.0f;
}

std::vector<std::string> AggLineCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggLineCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggLineCoreNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::path_storage path;
  path.move_to(m_x1 * w, aggY(m_y1, h));
  path.line_to(m_x2 * w, aggY(m_y2, h));
  strokePath(ren, path, m_color, m_thickness);

  applyAggBlend(outputs, inputs, m_blendMode);
}

nlohmann::json AggLineCoreNode::saveParams() const {
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
          {"bga", m_bgColor[3]},
          {"blendMode", m_blendMode}};
}

void AggLineCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// AggCircleCoreNode
// ============================================================

AggCircleCoreNode::AggCircleCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_cx(0.5f),
      m_cy(0.5f),
      m_rx(0.3f),
      m_ry(0.3f),
      m_thickness(3.0f),
      m_filled(true),
      m_stroked(false),
      m_blendMode(0) {
  m_fillColor[0] = 1.0f;
  m_fillColor[1] = 1.0f;
  m_fillColor[2] = 1.0f;
  m_fillColor[3] = 1.0f;
  m_strokeColor[0] = 1.0f;
  m_strokeColor[1] = 1.0f;
  m_strokeColor[2] = 1.0f;
  m_strokeColor[3] = 1.0f;
}

std::vector<std::string> AggCircleCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggCircleCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggCircleCoreNode::execute(const std::vector<GenTexture*>& inputs,
                            std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::ellipse ell(m_cx * w, aggY(m_cy, h), m_rx * w, m_ry * h, 100);
  if (m_filled)
    fillPath(ren, ell, m_fillColor);
  if (m_stroked)
    strokePath(ren, ell, m_strokeColor, m_thickness);
}

nlohmann::json AggCircleCoreNode::saveParams() const {
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
          {"sa", m_strokeColor[3]},
          {"blendMode", m_blendMode}};
}

void AggCircleCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// AggRectCoreNode
// ============================================================

AggRectCoreNode::AggRectCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.1f),
      m_y1(0.1f),
      m_x2(0.9f),
      m_y2(0.9f),
      m_cornerRadius(0.0f),
      m_thickness(3.0f),
      m_filled(true),
      m_stroked(false),
      m_blendMode(0) {
  m_fillColor[0] = 1.0f;
  m_fillColor[1] = 1.0f;
  m_fillColor[2] = 1.0f;
  m_fillColor[3] = 1.0f;
  m_strokeColor[0] = 1.0f;
  m_strokeColor[1] = 1.0f;
  m_strokeColor[2] = 1.0f;
  m_strokeColor[3] = 1.0f;
}

std::vector<std::string> AggRectCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggRectCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggRectCoreNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

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

nlohmann::json AggRectCoreNode::saveParams() const {
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
          {"sa", m_strokeColor[3]},
          {"blendMode", m_blendMode}};
}

void AggRectCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// AggPolygonCoreNode
// ============================================================

AggPolygonCoreNode::AggPolygonCoreNode()
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
      m_stroked(false),
      m_blendMode(0) {
  m_fillColor[0] = 1.0f;
  m_fillColor[1] = 1.0f;
  m_fillColor[2] = 1.0f;
  m_fillColor[3] = 1.0f;
  m_strokeColor[0] = 1.0f;
  m_strokeColor[1] = 1.0f;
  m_strokeColor[2] = 1.0f;
  m_strokeColor[3] = 1.0f;
}

std::vector<std::string> AggPolygonCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggPolygonCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggPolygonCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

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

nlohmann::json AggPolygonCoreNode::saveParams() const {
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
          {"sa", m_strokeColor[3]},
          {"blendMode", m_blendMode}};
}

void AggPolygonCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// AggTextCoreNode
// ============================================================

AggTextCoreNode::AggTextCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x(0.1f),
      m_y(0.5f),
      m_height(30.0f),
      m_thickness(1.5f),
      m_blendMode(0) {
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

std::vector<std::string> AggTextCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggTextCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggTextCoreNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

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

  applyAggBlend(outputs, inputs, m_blendMode);
}

nlohmann::json AggTextCoreNode::saveParams() const {
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
          {"bga", m_bgColor[3]},
          {"blendMode", m_blendMode}};
}

void AggTextCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// AggArcCoreNode
// ============================================================

AggArcCoreNode::AggArcCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_cx(0.5f),
      m_cy(0.5f),
      m_rx(0.3f),
      m_ry(0.3f),
      m_angle1(0.0f),
      m_angle2(270.0f),
      m_thickness(3.0f),
      m_blendMode(0) {
  m_color[0] = 1.0f;
  m_color[1] = 1.0f;
  m_color[2] = 1.0f;
  m_color[3] = 1.0f;
}

std::vector<std::string> AggArcCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggArcCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggArcCoreNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  double a1 = m_angle1 * sPI / 180.0;
  double a2 = m_angle2 * sPI / 180.0;
  agg::arc a(m_cx * w, aggY(m_cy, h), m_rx * w, m_ry * h, a1, a2, true);
  strokePath(ren, a, m_color, m_thickness);

  applyAggBlend(outputs, inputs, m_blendMode);
}

nlohmann::json AggArcCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"cx", m_cx},
          {"cy", m_cy},
          {"rx", m_rx},
          {"ry", m_ry},
          {"angle1", m_angle1},
          {"angle2", m_angle2},
          {"thickness", m_thickness},
          {"blendMode", m_blendMode},
          {"cr", m_color[0]},
          {"cg", m_color[1]},
          {"cb", m_color[2]},
          {"ca", m_color[3]}};
}

void AggArcCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("angle1"))
    m_angle1 = j["angle1"];
  if (j.contains("angle2"))
    m_angle2 = j["angle2"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
  if (j.contains("cr"))
    m_color[0] = j["cr"];
  if (j.contains("cg"))
    m_color[1] = j["cg"];
  if (j.contains("cb"))
    m_color[2] = j["cb"];
  if (j.contains("ca"))
    m_color[3] = j["ca"];
}

// ============================================================
// AggBezierCoreNode
// ============================================================

AggBezierCoreNode::AggBezierCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.1f),
      m_y1(0.5f),
      m_cx1(0.3f),
      m_cy1(0.1f),
      m_cx2(0.7f),
      m_cy2(0.9f),
      m_x2(0.9f),
      m_y2(0.5f),
      m_thickness(3.0f),
      m_blendMode(0) {
  m_color[0] = 1.0f;
  m_color[1] = 1.0f;
  m_color[2] = 1.0f;
  m_color[3] = 1.0f;
}

std::vector<std::string> AggBezierCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggBezierCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggBezierCoreNode::execute(const std::vector<GenTexture*>& inputs,
                            std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::curve4 curve(m_x1 * w, aggY(m_y1, h), m_cx1 * w, aggY(m_cy1, h),
                    m_cx2 * w, aggY(m_cy2, h), m_x2 * w, aggY(m_y2, h));
  strokePath(ren, curve, m_color, m_thickness);

  applyAggBlend(outputs, inputs, m_blendMode);
}

nlohmann::json AggBezierCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"x1", m_x1},
          {"y1", m_y1},
          {"cx1", m_cx1},
          {"cy1", m_cy1},
          {"cx2", m_cx2},
          {"cy2", m_cy2},
          {"x2", m_x2},
          {"y2", m_y2},
          {"thickness", m_thickness},
          {"blendMode", m_blendMode},
          {"cr", m_color[0]},
          {"cg", m_color[1]},
          {"cb", m_color[2]},
          {"ca", m_color[3]}};
}

void AggBezierCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("x1"))
    m_x1 = j["x1"];
  if (j.contains("y1"))
    m_y1 = j["y1"];
  if (j.contains("cx1"))
    m_cx1 = j["cx1"];
  if (j.contains("cy1"))
    m_cy1 = j["cy1"];
  if (j.contains("cx2"))
    m_cx2 = j["cx2"];
  if (j.contains("cy2"))
    m_cy2 = j["cy2"];
  if (j.contains("x2"))
    m_x2 = j["x2"];
  if (j.contains("y2"))
    m_y2 = j["y2"];
  if (j.contains("thickness"))
    m_thickness = j["thickness"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
  if (j.contains("cr"))
    m_color[0] = j["cr"];
  if (j.contains("cg"))
    m_color[1] = j["cg"];
  if (j.contains("cb"))
    m_color[2] = j["cb"];
  if (j.contains("ca"))
    m_color[3] = j["ca"];
}

// ============================================================
// AggDashLineCoreNode
// ============================================================

AggDashLineCoreNode::AggDashLineCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.1f),
      m_y1(0.5f),
      m_x2(0.9f),
      m_y2(0.5f),
      m_thickness(3.0f),
      m_dashLen(15.0f),
      m_gapLen(10.0f),
      m_blendMode(0) {
  m_color[0] = 1.0f;
  m_color[1] = 1.0f;
  m_color[2] = 1.0f;
  m_color[3] = 1.0f;
}

std::vector<std::string> AggDashLineCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggDashLineCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggDashLineCoreNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::path_storage path;
  path.move_to(m_x1 * w, aggY(m_y1, h));
  path.line_to(m_x2 * w, aggY(m_y2, h));

  agg::conv_dash<agg::path_storage> dash(path);
  dash.add_dash(m_dashLen, m_gapLen);
  agg::conv_stroke<agg::conv_dash<agg::path_storage>> stroke(dash);
  stroke.width(m_thickness);
  stroke.line_cap(agg::round_cap);

  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;
  AggRendererSolid solid(ren);
  ras.add_path(stroke);
  solid.color(agg_color(m_color));
  agg::render_scanlines(ras, sl, solid);

  applyAggBlend(outputs, inputs, m_blendMode);
}

nlohmann::json AggDashLineCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"x1", m_x1},
          {"y1", m_y1},
          {"x2", m_x2},
          {"y2", m_y2},
          {"thickness", m_thickness},
          {"dashLen", m_dashLen},
          {"gapLen", m_gapLen},
          {"blendMode", m_blendMode},
          {"cr", m_color[0]},
          {"cg", m_color[1]},
          {"cb", m_color[2]},
          {"ca", m_color[3]}};
}

void AggDashLineCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("dashLen"))
    m_dashLen = j["dashLen"];
  if (j.contains("gapLen"))
    m_gapLen = j["gapLen"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
  if (j.contains("cr"))
    m_color[0] = j["cr"];
  if (j.contains("cg"))
    m_color[1] = j["cg"];
  if (j.contains("cb"))
    m_color[2] = j["cb"];
  if (j.contains("ca"))
    m_color[3] = j["ca"];
}

// ============================================================
// AggGradientCoreNode
// ============================================================

AggGradientCoreNode::AggGradientCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_type(0),
      m_x1(0.0f),
      m_y1(0.5f),
      m_x2(1.0f),
      m_y2(0.5f),
      m_blendMode(0) {
  m_color1[0] = 0.0f;
  m_color1[1] = 0.0f;
  m_color1[2] = 0.0f;
  m_color1[3] = 1.0f;
  m_color2[0] = 1.0f;
  m_color2[1] = 1.0f;
  m_color2[2] = 1.0f;
  m_color2[3] = 1.0f;
}

std::vector<std::string> AggGradientCoreNode::inputSlotNames() const {
  return {"Bg"};
}

std::vector<std::string> AggGradientCoreNode::outputSlotNames() const {
  return {"Out"};
}

void AggGradientCoreNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  prepareOutput(outputs, inputs, w, h, m_blendMode);

  auto rbuf = agg_rbuf_from(outputs[0]);
  AggPixfmt pixf(rbuf);
  AggRendererBase ren(pixf);

  agg::rgba16 c1 = agg_color(m_color1);
  agg::rgba16 c2 = agg_color(m_color2);

  // Build gradient color LUT
  agg::gradient_lut<agg::color_interpolator<agg::rgba16>, 256> lut;
  lut.remove_all();
  lut.add_color(0.0, c1);
  lut.add_color(1.0, c2);
  lut.build_lut();

  // Transform: map gradient space to pixel coords
  double px1 = m_x1 * w, py1 = aggY(m_y1, h);
  double px2 = m_x2 * w, py2 = aggY(m_y2, h);

  agg::trans_affine mtx;
  if (m_type == 0) {
    // Linear: gradient along the line (px1,py1)→(px2,py2)
    double dx = px2 - px1, dy = py2 - py1;
    double len = sqrt(dx * dx + dy * dy);
    if (len < 1.0)
      len = 1.0;
    double angle = atan2(dy, dx);
    mtx.reset();
    mtx *= agg::trans_affine_scaling(len / 256.0);
    mtx *= agg::trans_affine_rotation(angle);
    mtx *= agg::trans_affine_translation(px1, py1);
    mtx.invert();
  } else {
    // Radial: gradient from center (px1,py1) with radius to (px2,py2)
    double dx = px2 - px1, dy = py2 - py1;
    double radius = sqrt(dx * dx + dy * dy);
    if (radius < 1.0)
      radius = 1.0;
    mtx.reset();
    mtx *= agg::trans_affine_scaling(radius / 128.0);
    mtx *= agg::trans_affine_translation(px1, py1);
    mtx.invert();
  }

  agg::span_interpolator_linear<> inter(mtx);
  agg::span_allocator<agg::rgba16> sa;

  // Fill full image with gradient
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_u8 sl;

  // Create a full-rect path
  agg::path_storage rect;
  rect.move_to(0, 0);
  rect.line_to(w, 0);
  rect.line_to(w, h);
  rect.line_to(0, h);
  rect.close_polygon();
  ras.add_path(rect);

  if (m_type == 0) {
    agg::gradient_x gf;
    agg::span_gradient<
        agg::rgba16, agg::span_interpolator_linear<>, agg::gradient_x,
        agg::gradient_lut<agg::color_interpolator<agg::rgba16>, 256>>
        sg(inter, gf, lut, 0, 256);
    agg::render_scanlines_aa(ras, sl, ren, sa, sg);
  } else {
    agg::gradient_radial gf;
    agg::span_gradient<
        agg::rgba16, agg::span_interpolator_linear<>, agg::gradient_radial,
        agg::gradient_lut<agg::color_interpolator<agg::rgba16>, 256>>
        sg(inter, gf, lut, 0, 128);
    agg::render_scanlines_aa(ras, sl, ren, sa, sg);
  }

  applyAggBlend(outputs, inputs, m_blendMode);
}

nlohmann::json AggGradientCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"type", m_type},
          {"x1", m_x1},
          {"y1", m_y1},
          {"x2", m_x2},
          {"y2", m_y2},
          {"blendMode", m_blendMode},
          {"c1r", m_color1[0]},
          {"c1g", m_color1[1]},
          {"c1b", m_color1[2]},
          {"c1a", m_color1[3]},
          {"c2r", m_color2[0]},
          {"c2g", m_color2[1]},
          {"c2b", m_color2[2]},
          {"c2a", m_color2[3]}};
}

void AggGradientCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("type"))
    m_type = j["type"];
  if (j.contains("x1"))
    m_x1 = j["x1"];
  if (j.contains("y1"))
    m_y1 = j["y1"];
  if (j.contains("x2"))
    m_x2 = j["x2"];
  if (j.contains("y2"))
    m_y2 = j["y2"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
  if (j.contains("c1r"))
    m_color1[0] = j["c1r"];
  if (j.contains("c1g"))
    m_color1[1] = j["c1g"];
  if (j.contains("c1b"))
    m_color1[2] = j["c1b"];
  if (j.contains("c1a"))
    m_color1[3] = j["c1a"];
  if (j.contains("c2r"))
    m_color2[0] = j["c2r"];
  if (j.contains("c2g"))
    m_color2[1] = j["c2g"];
  if (j.contains("c2b"))
    m_color2[2] = j["c2b"];
  if (j.contains("c2a"))
    m_color2[3] = j["c2a"];
}
