#include "MMNodes.h"
#include <imgui.h>

// ============================================================
// Shared helpers (same conventions as AllNodes.cpp)
// ============================================================

static const char* s_sizesStr = "32\00064\000128\000256\000512\0001024\000";

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
      *v = sClamp(*v, mn, mx);
      changed = true;
    }
  }
  return changed;
}

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
      *v = sClamp(*v + wheel * step, mn, mx);
      changed = true;
    }
  }
  return changed;
}

static void Hint(const char* text) {
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", text);
}

// ============================================================
// VoronoiNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> VoronoiNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> VoronoiNode::outputSlotInfos() const {
  return {{"Color", 1}, {"F1", 1}, {"Edge", 1}};
}

void VoronoiNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##vor", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##vor", &m_core.m_heightIdx, s_sizesStr);
  SliderIntW("ScaleX##vor", &m_core.m_scaleX, 1, 32);
  Hint("The scale along the X axis");
  SliderIntW("ScaleY##vor", &m_core.m_scaleY, 1, 32);
  Hint("The scale along the Y axis");
  SliderFloatW("StretchX##vor", &m_core.m_stretchX, 0.01f, 1.0f);
  Hint("The stretch factor along the X axis");
  SliderFloatW("StretchY##vor", &m_core.m_stretchY, 0.01f, 1.0f);
  Hint("The stretch factor along the Y axis");
  SliderFloatW("Intensity##vor", &m_core.m_intensity, 0.0f, 1.0f);
  Hint("A value factor for the grayscale F1 output");
  SliderFloatW("Randomness##vor", &m_core.m_randomness, 0.0f, 1.0f);
  Hint("The randomness of cell centers positions");
  SliderFloatW("Seed##vor", &m_core.m_seed, 0.0f, 1.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// FBMNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> FBMNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> FBMNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void FBMNode::renderParams() {
  static const char* modes =
      "Value\0Perlin\0Perlin (abs)\0Cellular F1\0Cellular F2-F1\0"
      "Cellular Manhattan F1\0Cellular Manhattan F2-F1\0"
      "Cellular Chebyshev F1\0Cellular Chebyshev F2-F1\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##fbm", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##fbm", &m_core.m_heightIdx, s_sizesStr);
  ImGui::Combo("Noise##fbm", &m_core.m_mode, modes);
  Hint("The noise variant accumulated per octave");
  SliderIntW("ScaleX##fbm", &m_core.m_scaleX, 1, 32);
  Hint("The scale along the X axis");
  SliderIntW("ScaleY##fbm", &m_core.m_scaleY, 1, 32);
  Hint("The scale along the Y axis");
  SliderIntW("Octaves##fbm", &m_core.m_octaves, 1, 10);
  Hint("Number of noise layers (detail levels)");
  SliderIntW("Folds##fbm", &m_core.m_folds, 0, 5);
  Hint("Folds the noise (abs(2x-1)) for ridged looks");
  SliderFloatW("Persistence##fbm", &m_core.m_persistence, 0.0f, 1.0f);
  Hint("Amplitude decay per octave");
  SliderFloatW("Seed##fbm", &m_core.m_seed, 0.0f, 1.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// BlendNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> BlendNode::inputSlotInfos() const {
  return {{"A", 1}, {"B", 1}, {"Mask", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> BlendNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void BlendNode::renderParams() {
  static const char* modes =
      "Normal\0Dissolve\0Multiply\0Screen\0Overlay\0Hard Light\0Soft Light\0"
      "Burn\0Dodge\0Lighten\0Darken\0Difference\0Additive\0AddSub\0"
      "Linear Light\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("Mode##blend", &m_core.m_mode, modes);
  Hint("The blend operation applied to A (top) over B (bottom)");
  SliderFloatW("Opacity##blend", &m_core.m_opacity, 0.0f, 1.0f);
  Hint("The opacity of the blend operation (modulated by Mask)");
  ImGui::PopItemWidth();
}

// ============================================================
// WarpNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> WarpNode::inputSlotInfos() const {
  return {{"In", 1}, {"Height", 1}, {"Strength", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> WarpNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void WarpNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Amount##warp", &m_core.m_amount, -1.0f, 1.0f);
  Hint("The strength of the warp effect");
  SliderFloatW("Epsilon##warp", &m_core.m_epsilon, 0.001f, 0.1f);
  Hint("Sampling distance for the height map slope");
  ImGui::PopItemWidth();
}

// ============================================================
// ColorizeNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ColorizeNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> ColorizeNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ColorizeNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Text("Gradient stops:");
  Hint("Maps input luminance through this color ramp");
  int removeIdx = -1;
  for (int i = 0; i < (int)m_core.m_stops.size(); i++) {
    auto& s = m_core.m_stops[i];
    ImGui::PushID(i);
    float col[4] = {s.r, s.g, s.b, s.a};
    if (ImGui::ColorEdit4("##stopcol", col, ImGuiColorEditFlags_NoInputs)) {
      s.r = col[0];
      s.g = col[1];
      s.b = col[2];
      s.a = col[3];
    }
    ImGui::SameLine();
    SliderFloatW("##stoppos", &s.pos, 0.0f, 1.0f);
    Hint("Position of this stop on the ramp");
    if (m_core.m_stops.size() > 1) {
      ImGui::SameLine();
      if (ImGui::SmallButton("x"))
        removeIdx = i;
    }
    ImGui::PopID();
  }
  if (removeIdx >= 0)
    m_core.m_stops.erase(m_core.m_stops.begin() + removeIdx);
  if (m_core.m_stops.size() < 8 && ImGui::SmallButton("+ stop")) {
    MMGradientStop last = m_core.m_stops.back();
    m_core.m_stops.push_back(last);
  }
  ImGui::PopItemWidth();
}

// ============================================================
// MMBricksNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MMBricksNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> MMBricksNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void MMBricksNode::renderParams() {
  static const char* patterns =
      "Running Bond\0Running Bond (2)\0Herringbone\0Basket Weave\0"
      "Spanish Bond\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##mmb", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##mmb", &m_core.m_heightIdx, s_sizesStr);
  ImGui::Combo("Pattern##mmb", &m_core.m_pattern, patterns);
  Hint("The brick layout pattern");
  SliderIntW("CountX##mmb", &m_core.m_countX, 1, 16);
  Hint("Bricks per row (or pattern arm length)");
  SliderIntW("CountY##mmb", &m_core.m_countY, 1, 16);
  Hint("Brick rows (or pattern arm width)");
  SliderIntW("Repeat##mmb", &m_core.m_repeat, 1, 8);
  Hint("Pattern repetitions");
  SliderFloatW("Offset##mmb", &m_core.m_offset, 0.0f, 1.0f);
  Hint("Row offset for running bond layouts");
  SliderFloatW("Mortar##mmb", &m_core.m_mortar, 0.0f, 0.5f);
  Hint("Mortar (gap) width relative to brick size");
  SliderFloatW("Round##mmb", &m_core.m_round, 0.0f, 0.5f);
  Hint("Brick corner rounding");
  SliderFloatW("Bevel##mmb", &m_core.m_bevel, 0.01f, 0.5f);
  Hint("Brick edge bevel softness");
  SliderFloatW("Balance##mmb", &m_core.m_colorBalance, 0.0f, 1.0f);
  Hint("Random color mix between Color1 and Color2 per brick");
  SliderFloatW("Seed##mmb", &m_core.m_seed, 0.0f, 1.0f);
  ImGui::ColorEdit4("Color1##mmb", m_core.m_col0, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##mmb", m_core.m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Mortar##mmbc", m_core.m_colMortar,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// MaterialNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MaterialNode::inputSlotInfos() const {
  return {{"Albedo", 1}, {"Normal", 1}, {"Roughness", 1}, {"Metallic", 1},
          {"Height", 1}, {"AO", 1},     {"Emission", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MaterialNode::outputSlotInfos() const {
  return {};
}

void MaterialNode::renderParams() {
  ImGui::PushItemWidth(160);
  ImGui::InputText("Base##mat", m_core.m_baseName, sizeof(m_core.m_baseName));
  Hint(
      "Base filename: each connected channel is saved as\n"
      "<base>_<channel>.png (e.g. material_albedo.png)");
  ImGui::PopItemWidth();
}

// ============================================================
// NormalMapNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> NormalMapNode::inputSlotInfos() const {
  return {{"Height", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> NormalMapNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void NormalMapNode::renderParams() {
  static const char* formats = "Default (-Z)\0OpenGL\0DirectX\0";
  ImGui::PushItemWidth(120);
  SliderFloatW("Amount##nm", &m_core.m_amount, 0.0f, 4.0f);
  Hint("The strength of the generated normals");
  ImGui::Combo("Format##nm", &m_core.m_format, formats);
  Hint("Normal map convention (DirectX flips the Y axis)");
  ImGui::PopItemWidth();
}

// ============================================================
// SdfShapeNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SdfShapeNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> SdfShapeNode::outputSlotInfos() const {
  return {{"Sdf", 1}};
}

void SdfShapeNode::renderParams() {
  static const char* s_sizes = "32\00064\000128\000256\000512\0001024\000";
  static const char* shapes = "Circle\0Box\0Line\0Star\0N-gon\0Rhombus\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##sdfs", &m_core.m_widthIdx, s_sizes);
  ImGui::Combo("H##sdfs", &m_core.m_heightIdx, s_sizes);
  ImGui::Combo("Shape##sdfs", &m_core.m_p.shape, shapes);
  Hint("The distance-field shape to generate");
  int sh = m_core.m_p.shape;
  if (sh == MMSdfLine) {
    SliderFloatW("Ax##sdfs", &m_core.m_p.ax, 0.0f, 1.0f);
    SliderFloatW("Ay##sdfs", &m_core.m_p.ay, 0.0f, 1.0f);
    SliderFloatW("Bx##sdfs", &m_core.m_p.bx, 0.0f, 1.0f);
    SliderFloatW("By##sdfs", &m_core.m_p.by, 0.0f, 1.0f);
    SliderFloatW("Width##sdfs", &m_core.m_p.w, 0.0f, 0.5f);
  } else {
    SliderFloatW("CX##sdfs", &m_core.m_p.cx, 0.0f, 1.0f);
    SliderFloatW("CY##sdfs", &m_core.m_p.cy, 0.0f, 1.0f);
    SliderFloatW("Size##sdfs", &m_core.m_p.w, 0.0f, 1.0f);
    Hint("Radius (circle/star/ngon) or half-width (box/rhombus)");
    if (sh == MMSdfBox || sh == MMSdfRhombus)
      SliderFloatW("SizeY##sdfs", &m_core.m_p.h, 0.0f, 1.0f);
    if (sh == MMSdfStar || sh == MMSdfNgon) {
      SliderIntW("Sides##sdfs", &m_core.m_p.n, 2, 16);
      SliderFloatW("Rot##sdfs", &m_core.m_p.rot, -180.0f, 180.0f);
    }
    if (sh == MMSdfStar) {
      SliderFloatW("Inner##sdfs", &m_core.m_p.ir, 0.0f, 1.0f);
      Hint("Inner radius ratio (spikiness of the star)");
    }
  }
  ImGui::PopItemWidth();
}

// ============================================================
// SdfOpNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SdfOpNode::inputSlotInfos() const {
  return {{"A", 1}, {"B", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> SdfOpNode::outputSlotInfos() const {
  return {{"Sdf", 1}};
}

void SdfOpNode::renderParams() {
  static const char* ops =
      "Union\0Subtraction (B-A)\0Intersection\0Smooth Union\0"
      "Smooth Subtraction\0Smooth Intersection\0Morph\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("Op##sdfo", &m_core.m_op, ops);
  Hint("Boolean operation between the two distance fields");
  SliderFloatW("K##sdfo", &m_core.m_k, 0.0f, 1.0f);
  Hint("Smoothness for smooth ops / mix amount for morph");
  ImGui::PopItemWidth();
}

// ============================================================
// SdfTransformNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SdfTransformNode::inputSlotInfos() const {
  return {{"Sdf", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> SdfTransformNode::outputSlotInfos() const {
  return {{"Sdf", 1}};
}

void SdfTransformNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("TX##sdft", &m_core.m_tx, -1.0f, 1.0f);
  SliderFloatW("TY##sdft", &m_core.m_ty, -1.0f, 1.0f);
  SliderFloatW("Rot##sdft", &m_core.m_rot, -180.0f, 180.0f);
  SliderFloatW("Scale##sdft", &m_core.m_scale, 0.1f, 4.0f);
  SliderFloatW("Round##sdft", &m_core.m_round, 0.0f, 0.5f);
  Hint("Grows the shape by rounding its distance field");
  SliderIntW("Rings##sdft", &m_core.m_annularCount, 0, 8);
  Hint("Annular ripples: repeats d = abs(d) - width");
  SliderFloatW("RingW##sdft", &m_core.m_annularW, 0.0f, 0.25f);
  ImGui::PopItemWidth();
}

// ============================================================
// SdfShowNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SdfShowNode::inputSlotInfos() const {
  return {{"Sdf", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> SdfShowNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void SdfShowNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Base##sdfw", &m_core.m_base, 0.0f, 1.0f);
  Hint("Base value added before the distance falloff");
  SliderFloatW("Bevel##sdfw", &m_core.m_bevel, 0.001f, 0.5f);
  Hint("Softness of the shape edge");
  ImGui::PopItemWidth();
}

// ============================================================
// MakeTileableNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MakeTileableNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MakeTileableNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void MakeTileableNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Width##mt", &m_core.m_width, 0.0f, 0.25f);
  Hint("Width of the transition areas between parts of the image");
  ImGui::PopItemWidth();
}

// ============================================================
// QuantizeNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> QuantizeNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> QuantizeNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void QuantizeNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderIntW("Steps##qt", &m_core.m_steps, 2, 32);
  Hint("The number of quantization steps");
  ImGui::PopItemWidth();
}

// ============================================================
// EmbossNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> EmbossNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> EmbossNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void EmbossNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Angle##em", &m_core.m_angle, -180.0f, 180.0f);
  Hint("Light direction in degrees");
  SliderFloatW("Amount##em", &m_core.m_amount, 0.0f, 4.0f);
  SliderIntW("Width##em", &m_core.m_width, 1, 8);
  Hint("Kernel radius in pixels");
  ImGui::PopItemWidth();
}

// ============================================================
// Transform2DNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> Transform2DNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> Transform2DNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void Transform2DNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("TX##t2d", &m_core.m_tx, -1.0f, 1.0f);
  Hint("Translation along the X axis");
  SliderFloatW("TY##t2d", &m_core.m_ty, -1.0f, 1.0f);
  Hint("Translation along the Y axis");
  SliderFloatW("Rot##t2d", &m_core.m_rot, -180.0f, 180.0f);
  Hint("Rotation around the center, in degrees");
  SliderFloatW("ScaleX##t2d", &m_core.m_scaleX, 0.05f, 4.0f);
  SliderFloatW("ScaleY##t2d", &m_core.m_scaleY, 0.05f, 4.0f);
  ImGui::Checkbox("Repeat##t2d", &m_core.m_repeat);
  Hint("Wrap the image (tile) instead of clamping at the edges");
  ImGui::PopItemWidth();
}

// ============================================================
// ShapeNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ShapeNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> ShapeNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ShapeNode::renderParams() {
  static const char* s_sizes = "32\00064\000128\000256\000512\0001024\000";
  static const char* shapes = "Circle\0Polygon\0Star\0Curved Star\0Rays\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##shp", &m_core.m_widthIdx, s_sizes);
  ImGui::Combo("H##shp", &m_core.m_heightIdx, s_sizes);
  ImGui::Combo("Shape##shp", &m_core.m_shape, shapes);
  Hint("Draws a white shape on a black background");
  SliderFloatW("Sides##shp", &m_core.m_sides, 2.0f, 16.0f);
  Hint("Number of sides (polygon/star/rays)");
  SliderFloatW("Radius##shp", &m_core.m_radius, 0.0f, 1.0f);
  SliderFloatW("Edge##shp", &m_core.m_edge, 0.0f, 1.0f);
  Hint("Edge softness");
  ImGui::PopItemWidth();
}

// ============================================================
// PatternNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> PatternNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> PatternNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void PatternNode::renderParams() {
  static const char* s_sizes = "32\00064\000128\000256\000512\0001024\000";
  static const char* waves =
      "Sine\0Triangle\0Square\0Sawtooth\0Constant\0Bounce\0";
  static const char* mixes = "Multiply\0Add\0Max\0Min\0Xor\0Pow\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##pat", &m_core.m_widthIdx, s_sizes);
  ImGui::Combo("H##pat", &m_core.m_heightIdx, s_sizes);
  ImGui::Combo("Mix##pat", &m_core.m_mix, mixes);
  Hint("How the X and Y waves are combined");
  ImGui::Combo("X Wave##pat", &m_core.m_xWave, waves);
  SliderFloatW("X Scale##pat", &m_core.m_xScale, 0.0f, 32.0f);
  ImGui::Combo("Y Wave##pat", &m_core.m_yWave, waves);
  SliderFloatW("Y Scale##pat", &m_core.m_yScale, 0.0f, 32.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// Combine / Decompose / Invert
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CombineNode::inputSlotInfos() const {
  return {{"R", 1}, {"G", 1}, {"B", 1}, {"A", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CombineNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> DecomposeNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> DecomposeNode::outputSlotInfos() const {
  return {{"R", 1}, {"G", 1}, {"B", 1}, {"A", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> InvertNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> InvertNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

// ============================================================
// LayerMixNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> LayerMixNode::inputSlotInfos() const {
  return {{"H1", 1}, {"C1", 1}, {"ORM1", 1}, {"EM1", 1}, {"NM1", 1},
          {"H2", 1}, {"C2", 1}, {"ORM2", 1}, {"EM2", 1}, {"NM2", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> LayerMixNode::outputSlotInfos() const {
  return {{"H", 1}, {"C", 1}, {"ORM", 1}, {"EM", 1}, {"NM", 1}};
}

void LayerMixNode::renderParams() {
  static const char* modes = "Hard (max height)\0Smooth\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("Mode##lm", &m_core.m_mode, modes);
  Hint(
      "Hard picks the layer with the highest height per pixel;\n"
      "Smooth blends across the height difference");
  if (m_core.m_mode == 1) {
    SliderFloatW("Width##lm", &m_core.m_width, 0.001f, 0.5f);
    Hint("Blend width (height difference range)");
  }
  ImGui::PopItemWidth();
}

// ============================================================
// WorkflowOutputNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> WorkflowOutputNode::inputSlotInfos() const {
  return {
      {"Height", 1}, {"Albedo", 1}, {"ORM", 1}, {"Emission", 1}, {"Normal", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> WorkflowOutputNode::outputSlotInfos() const {
  return {{"Albedo", 1}, {"Metallic", 1},  {"Roughness", 1}, {"Emission", 1},
          {"Normal", 1}, {"Occlusion", 1}, {"Depth", 1}};
}

void WorkflowOutputNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Mat Normal##wo", &m_core.m_matNormal, 0.0f, 2.0f);
  Hint(
      "Weight of the material normals relative to the normal\n"
      "generated from the height input");
  SliderFloatW("Occlusion##wo", &m_core.m_occlusion, 0.0f, 4.0f);
  Hint("Strength of the ambient occlusion approximated from height");
  ImGui::PopItemWidth();
}

// ============================================================
// MathOpNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MathOpNode::inputSlotInfos() const {
  return {{"A", 1}, {"B", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MathOpNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void MathOpNode::renderParams() {
  static const char* ops =
      "A + B\0A - B\0A * B\0A / B\0log(A)\0log2(A)\0pow(A, B)\0abs(A)\0"
      "round(A)\0floor(A)\0ceil(A)\0trunc(A)\0fract(A)\0min(A, B)\0"
      "max(A, B)\0A < B\0cos(A*B)\0sin(A*B)\0tan(A*B)\0sqrt(1-A*A)\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("Op##math", &m_core.m_op, ops);
  Hint("Per-pixel scalar operation on the grayscale inputs");
  SliderFloatW("Default A##math", &m_core.m_def1, 0.0f, 1.0f);
  Hint("Constant used when input A is not connected");
  SliderFloatW("Default B##math", &m_core.m_def2, 0.0f, 1.0f);
  Hint("Constant used when input B is not connected");
  ImGui::Checkbox("Clamp##math", &m_core.m_clamp);
  Hint("Clamp the result to [0, 1]");
  ImGui::PopItemWidth();
}

// ============================================================
// GradientMMNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> GradientMMNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> GradientMMNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void GradientMMNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##gmm", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##gmm", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("Repeat##gmm", &m_core.m_repeat, 0.0f, 16.0f);
  Hint("Number of ramp repetitions across the texture");
  SliderFloatW("Rotate##gmm", &m_core.m_rotate, -180.0f, 180.0f);
  Hint("Ramp direction in degrees");
  ImGui::Checkbox("Mirror##gmm", &m_core.m_mirror);
  Hint("Mirror each repetition (triangle wave)");
  ImGui::Text("Gradient stops:");
  int removeIdx = -1;
  for (int i = 0; i < (int)m_core.m_stops.size(); i++) {
    auto& s = m_core.m_stops[i];
    ImGui::PushID(i + 1000);
    float col[4] = {s.r, s.g, s.b, s.a};
    if (ImGui::ColorEdit4("##gstopcol", col, ImGuiColorEditFlags_NoInputs)) {
      s.r = col[0];
      s.g = col[1];
      s.b = col[2];
      s.a = col[3];
    }
    ImGui::SameLine();
    SliderFloatW("##gstoppos", &s.pos, 0.0f, 1.0f);
    if (m_core.m_stops.size() > 1) {
      ImGui::SameLine();
      if (ImGui::SmallButton("x"))
        removeIdx = i;
    }
    ImGui::PopID();
  }
  if (removeIdx >= 0)
    m_core.m_stops.erase(m_core.m_stops.begin() + removeIdx);
  if (m_core.m_stops.size() < 8 && ImGui::SmallButton("+ stop##gmm")) {
    MMGradientStop last = m_core.m_stops.back();
    m_core.m_stops.push_back(last);
  }
  ImGui::PopItemWidth();
}

// ============================================================
// TilerNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> TilerNode::inputSlotInfos() const {
  return {{"In", 1}, {"Mask", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> TilerNode::outputSlotInfos() const {
  return {{"Out", 1}, {"Color", 1}};
}

void TilerNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Tile X##til", &m_core.m_tx, 1.0f, 32.0f);
  SliderFloatW("Tile Y##til", &m_core.m_ty, 1.0f, 32.0f);
  Hint("Grid size (instances per axis)");
  ImGui::SliderInt("Overlap##til", &m_core.m_overlap, 0, 3);
  Hint("How far instances can spill into neighbor cells");
  static const char* tilesets = "1\0002x2\0004x4\0";
  int tsIdx = m_core.m_inputs == 4 ? 2 : (m_core.m_inputs == 2 ? 1 : 0);
  if (ImGui::Combo("Tileset##til", &tsIdx, tilesets))
    m_core.m_inputs = tsIdx == 2 ? 4 : (tsIdx == 1 ? 2 : 1);
  Hint("Treat the input as an NxN tileset and pick tiles at random");
  SliderFloatW("Scale X##til", &m_core.m_scaleX, 0.1f, 4.0f);
  SliderFloatW("Scale Y##til", &m_core.m_scaleY, 0.1f, 4.0f);
  SliderFloatW("Fixed offset##til", &m_core.m_fixedOffset, 0.0f, 1.0f);
  Hint("Per-row horizontal offset (brick-like layouts)");
  SliderFloatW("Offset##til", &m_core.m_offset, 0.0f, 1.0f);
  Hint("Random position jitter");
  SliderFloatW("Rotate##til", &m_core.m_rotate, 0.0f, 180.0f);
  Hint("Random rotation range in degrees");
  SliderFloatW("Scale jitter##til", &m_core.m_scale, 0.0f, 1.0f);
  Hint("Random scale variation");
  SliderFloatW("Value##til", &m_core.m_value, 0.0f, 1.0f);
  Hint("Random brightness attenuation per instance");
  SliderFloatW("Seed##til", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// MultiWarpNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MultiWarpNode::inputSlotInfos() const {
  return {{"In", 1}, {"Height", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MultiWarpNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void MultiWarpNode::renderParams() {
  static const char* modes = "Min\0Blur\0Max\0";
  ImGui::PushItemWidth(120);
  SliderFloatW("Size##mw", &m_core.m_size, 1.0f, 64.0f);
  Hint("Gradient sampling distance (1/size texels)");
  SliderFloatW("Intensity##mw", &m_core.m_intensity, 0.0f, 1.0f);
  Hint("Warp strength (also scales the iteration count)");
  SliderFloatW("Quality##mw", &m_core.m_quality, 1.0f, 100.0f);
  Hint("Maximum iterations at full intensity");
  ImGui::Combo("Mode##mw", &m_core.m_mode, modes);
  Hint("How samples accumulate along the walk");
  ImGui::PopItemWidth();
}

// ============================================================
// SlopeBlurNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SlopeBlurNode::inputSlotInfos() const {
  return {{"In", 1}, {"Height", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> SlopeBlurNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void SlopeBlurNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Size##sb", &m_core.m_size, 1.0f, 64.0f);
  Hint("Slope sampling distance (1/size texels)");
  SliderFloatW("Sigma##sb", &m_core.m_sigma, 0.0f, 50.0f);
  Hint("Blur strength, scaled by the local slope");
  ImGui::PopItemWidth();
}
