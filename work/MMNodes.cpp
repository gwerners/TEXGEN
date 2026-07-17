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
  return {{"Color", 1}, {"F1", 1}, {"Edge", 1}, {"Fill", 1}};
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
  static const char* warpModes = "Slope\0Distance to top\0";
  ImGui::PushItemWidth(120);
  SliderFloatW("Amount##warp", &m_core.m_amount, -1.0f, 1.0f);
  Hint("The strength of the warp effect");
  SliderFloatW("Epsilon##warp", &m_core.m_epsilon, 0.001f, 0.1f);
  Hint("Sampling distance for the height map slope");
  ImGui::Combo("Mode##warp", &m_core.m_mode, warpModes);
  Hint("Distance to top scales the warp by (1 - height)");
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
          {"Depth", 1},  {"AO", 1},     {"Emission", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MaterialNode::outputSlotInfos() const {
  return {{"Preview", 1}};
}

void MaterialNode::renderParams() {
  ImGui::PushItemWidth(160);
  ImGui::InputText("Base##mat", m_core.m_baseName, sizeof(m_core.m_baseName));
  Hint(
      "Base filename: each connected channel is saved as\n"
      "<base>_<channel>.png (e.g. material_albedo.png)");
  ImGui::PopItemWidth();
  ImGui::PushItemWidth(120);
  SliderFloatW("Light dir##mat", &m_core.m_lightAzimuth, 0.0f, 360.0f);
  Hint("Preview light azimuth (degrees, counter-clockwise from right)");
  SliderFloatW("Light height##mat", &m_core.m_lightElevation, 5.0f, 90.0f);
  Hint("Preview light elevation (90 = overhead)");
  SliderFloatW("Intensity##mat", &m_core.m_lightIntensity, 0.0f, 3.0f);
  SliderFloatW("Ambient##mat", &m_core.m_ambient, 0.0f, 1.0f);
  Hint("Unlit base level of the preview");
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
  return {{"In", 1}, {"TX", 1}, {"TY", 1}, {"Rot", 1}, {"SX", 1}, {"SY", 1}};
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
  return {{"RadiusMap", 1}, {"EdgeMap", 1}};
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
      "max(A, B)\0A < B\0cos(A*B)\0sin(A*B)\0tan(A*B)\0sqrt(1-A*A)\0"
      "smoothstep(A)\0ping-pong(A, B)\0sign(A)\0mod(A, B)\0atan2(A, B)\0"
      "asin(A)\0acos(A)\0atan(A)\0sinh(A)\0cosh(A)\0tanh(A)\0exp(A)\0"
      "snap(A, B)\0radians(A)\0degrees(A)\0log(A, B)\0sqrt(A)\0";
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
  ImGui::Combo("Shape##gmm", &m_core.m_shape, "Linear\0Radial\0Circular\0");
  Hint("Linear ramp, distance to center, or angle around the center");
  SliderFloatW("Repeat##gmm", &m_core.m_repeat, 0.0f, 16.0f);
  Hint("Number of ramp repetitions across the texture");
  SliderFloatW("Rotate##gmm", &m_core.m_rotate, -180.0f, 180.0f);
  Hint("Ramp direction in degrees (linear shape only)");
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

// ============================================================
// DotNoiseNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> DotNoiseNode::inputSlotInfos() const {
  return {{"Density", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> DotNoiseNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void DotNoiseNode::renderParams() {
  static const char* dnModes = "Dots\0Raw random\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##dn", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##dn", &m_core.m_heightIdx, s_sizesStr);
  ImGui::Combo("Mode##dn", &m_core.m_mode, dnModes);
  Hint("Dots thresholds against density; Raw outputs the cell value");
  ImGui::SliderInt("Grid##dn", &m_core.m_grid, 2, 2048);
  Hint("Number of random cells per axis");
  SliderFloatW("Density##dn", &m_core.m_density, 0.0f, 1.0f);
  Hint("Probability of a cell being white (Density input overrides)");
  SliderFloatW("Seed##dn", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// ScratchesNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ScratchesNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> ScratchesNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ScratchesNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##scr", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##scr", &m_core.m_heightIdx, s_sizesStr);
  ImGui::SliderInt("Layers##scr", &m_core.m_layers, 1, 16);
  Hint("Number of scratch layers (max composited)");
  SliderFloatW("Length##scr", &m_core.m_length, 0.01f, 1.0f);
  SliderFloatW("Width##scr", &m_core.m_width, 0.001f, 1.0f);
  SliderFloatW("Waviness##scr", &m_core.m_waviness, 0.0f, 1.0f);
  SliderFloatW("Angle##scr", &m_core.m_angle, -180.0f, 180.0f);
  SliderFloatW("Randomness##scr", &m_core.m_randomness, 0.0f, 1.0f);
  Hint("Random angle variation per scratch");
  SliderFloatW("Seed##scr", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// MirrorNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MirrorNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MirrorNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void MirrorNode::renderParams() {
  static const char* dirs = "Horizontal\0Vertical\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("Direction##mir", &m_core.m_direction, dirs);
  SliderFloatW("Offset##mir", &m_core.m_offset, -1.0f, 1.0f);
  Hint("Offset of the mirror axis from the center");
  ImGui::Checkbox("Flip sides##mir", &m_core.m_flipSides);
  ImGui::PopItemWidth();
}

// ============================================================
// EdgeDetectNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> EdgeDetectNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> EdgeDetectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void EdgeDetectNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Size##ed", &m_core.m_size, 16.0f, 2048.0f);
  Hint("Sampling distance (1/size texels)");
  ImGui::SliderInt("Width##ed", &m_core.m_width, 1, 32);
  Hint("Edge thickness in rings");
  SliderFloatW("Threshold##ed", &m_core.m_threshold, 0.0f, 1.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// CreateMapNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CreateMapNode::inputSlotInfos() const {
  return {{"Height", 1}, {"Offset", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CreateMapNode::outputSlotInfos() const {
  return {{"Map", 1}};
}

void CreateMapNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Height##cm", &m_core.m_height, 0.0f, 2.0f);
  Hint("Height scale packed into the map");
  SliderFloatW("Angle##cm", &m_core.m_angle, -180.0f, 180.0f);
  Hint("Rotation applied by the MatMap consumer");
  SliderFloatW("Seed##cm", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// MatMapNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> MatMapNode::inputSlotInfos() const {
  return {{"Map", 1}, {"C", 1}, {"ORM", 1}, {"EM", 1}, {"NM", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> MatMapNode::outputSlotInfos() const {
  return {{"H", 1}, {"C", 1}, {"ORM", 1}, {"EM", 1}, {"NM", 1}};
}

// ============================================================
// Fill family
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> FillNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> FillToUVNode::inputSlotInfos() const {
  return {{"Fill", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillToUVNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void FillToUVNode::renderParams() {
  static const char* modes = "Stretch\0Square\0";
  ImGui::PushItemWidth(120);
  ImGui::Combo("Mode##fuv", &m_core.m_mode, modes);
  Hint("Square keeps the region aspect ratio; Stretch fills the bbox");
  SliderFloatW("Seed##fuv", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> FillToRandomGrayNode::inputSlotInfos()
    const {
  return {{"Fill", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillToRandomGrayNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void FillToRandomGrayNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Edge##frg", &m_core.m_edgecolor, 0.0f, 1.0f);
  Hint("Grey level used for the edges between regions");
  SliderFloatW("Seed##frg", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> FillToRandomColorNode::inputSlotInfos()
    const {
  return {{"Fill", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillToRandomColorNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void FillToRandomColorNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::ColorEdit3("Edge##frc", m_core.m_edge, ImGuiColorEditFlags_NoInputs);
  Hint("Color used for the edges between regions");
  SliderFloatW("Seed##frc", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> FillToColorNode::inputSlotInfos() const {
  return {{"Fill", 1}, {"Map", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillToColorNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void FillToColorNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::ColorEdit4("Edge##ftc", m_core.m_edge, ImGuiColorEditFlags_NoInputs);
  Hint("Color used for the edges between regions");
  ImGui::PopItemWidth();
}

// ============================================================
// RemapNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> RemapNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> RemapNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void RemapNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Min##rmp", &m_core.m_min, -1.0f, 1.0f);
  SliderFloatW("Max##rmp", &m_core.m_max, -1.0f, 2.0f);
  Hint("Output range for input 0..1");
  SliderFloatW("Step##rmp", &m_core.m_step, 0.0f, 0.5f);
  Hint("Quantization step (0 = continuous)");
  ImGui::PopItemWidth();
}

// ============================================================
// Tile2x2Node
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> Tile2x2Node::inputSlotInfos() const {
  return {{"In1", 1}, {"In2", 1}, {"In3", 1}, {"In4", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> Tile2x2Node::outputSlotInfos() const {
  return {{"Out", 1}};
}

// ============================================================
// NormalConvertNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> NormalConvertNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> NormalConvertNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void NormalConvertNode::renderParams() {
  static const char* ops = "From/To OpenGL\0From/To DirectX\0";
  ImGui::PushItemWidth(140);
  ImGui::Combo("Op##nmc", &m_core.m_op, ops);
  Hint("Flips normal map channels between conventions");
  ImGui::PopItemWidth();
}

// ============================================================
// CustomUVNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CustomUVNode::inputSlotInfos() const {
  return {{"In", 1}, {"Map", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CustomUVNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void CustomUVNode::renderParams() {
  static const char* tilesets = "1\0002x2\0004x4\0";
  ImGui::PushItemWidth(120);
  int tsIdx = m_core.m_inputs == 4 ? 2 : (m_core.m_inputs == 2 ? 1 : 0);
  if (ImGui::Combo("Tileset##cuv", &tsIdx, tilesets))
    m_core.m_inputs = tsIdx == 2 ? 4 : (tsIdx == 1 ? 2 : 1);
  SliderFloatW("Scale X##cuv", &m_core.m_sx, 0.1f, 4.0f);
  SliderFloatW("Scale Y##cuv", &m_core.m_sy, 0.1f, 4.0f);
  SliderFloatW("Rotate##cuv", &m_core.m_rotate, 0.0f, 180.0f);
  Hint("Random rotation range per region");
  SliderFloatW("Scale jitter##cuv", &m_core.m_scale, 0.0f, 1.0f);
  SliderFloatW("Seed##cuv", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// SmoothCurvatureNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SmoothCurvatureNode::inputSlotInfos() const {
  return {{"Height", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> SmoothCurvatureNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void SmoothCurvatureNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Quality##sc", &m_core.m_quality, 1.0f, 8.0f);
  Hint("Samples per axis (quality^2 taps)");
  SliderFloatW("Strength##sc", &m_core.m_strength, 0.0f, 4.0f);
  SliderFloatW("Radius##sc", &m_core.m_radius, 0.1f, 4.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// AmbientOcclusionNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AmbientOcclusionNode::inputSlotInfos()
    const {
  return {{"Height", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> AmbientOcclusionNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void AmbientOcclusionNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Radius##ao", &m_core.m_radius, 0.01f, 0.25f);
  Hint("Blur radius as a fraction of the texture width");
  SliderFloatW("Strength##ao", &m_core.m_strength, 0.0f, 4.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// LevelsNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> LevelsNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> LevelsNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void LevelsNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::ColorEdit4("In min##lv", m_core.m_inMin, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("In mid##lv", m_core.m_inMid, ImGuiColorEditFlags_NoInputs);
  Hint("Midtone pivot (gamma-like)");
  ImGui::ColorEdit4("In max##lv", m_core.m_inMax, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Out min##lv", m_core.m_outMin,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Out max##lv", m_core.m_outMax,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// SphereNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> SphereNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> SphereNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void SphereNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##sph", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##sph", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("Center X##sph", &m_core.m_cx, 0.0f, 1.0f);
  SliderFloatW("Center Y##sph", &m_core.m_cy, 0.0f, 1.0f);
  SliderFloatW("Radius##sph", &m_core.m_r, 0.01f, 1.0f);
  ImGui::Checkbox("Normalized##sph", &m_core.m_normalized);
  Hint("Divide by 2r so the dome top is exactly 1");
  ImGui::PopItemWidth();
}

// ============================================================
// AnisotropicNoiseNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> AnisotropicNoiseNode::inputSlotInfos()
    const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> AnisotropicNoiseNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void AnisotropicNoiseNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##ani", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##ani", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("Scale X##ani", &m_core.m_scaleX, 1.0f, 32.0f);
  SliderFloatW("Scale Y##ani", &m_core.m_scaleY, 1.0f, 1024.0f);
  Hint("High Y scale gives thin horizontal stripes");
  SliderFloatW("Smooth##ani", &m_core.m_smoothness, 0.0f, 1.0f);
  SliderFloatW("Interp##ani", &m_core.m_interpolation, 0.0f, 1.0f);
  SliderFloatW("Seed##ani", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// TilerAdvancedNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> TilerAdvancedNode::inputSlotInfos() const {
  return {{"In", 1},  {"Mask", 1}, {"Color1", 1}, {"Color2", 1}, {"TrX", 1},
          {"TrY", 1}, {"Rot", 1},  {"ScX", 1},    {"ScY", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> TilerAdvancedNode::outputSlotInfos() const {
  return {{"Out", 1}, {"Color1", 1}, {"Color2", 1}, {"UV", 1}};
}

void TilerAdvancedNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Tile X##tla", &m_core.m_tx, 1.0f, 32.0f);
  SliderFloatW("Tile Y##tla", &m_core.m_ty, 1.0f, 32.0f);
  ImGui::SliderInt("Overlap##tla", &m_core.m_overlap, 0, 3);
  SliderFloatW("Translate X##tla", &m_core.m_translateX, -1.0f, 1.0f);
  SliderFloatW("Translate Y##tla", &m_core.m_translateY, -1.0f, 1.0f);
  Hint("Per-instance offset, modulated by the TrX/TrY maps");
  SliderFloatW("Rotate##tla", &m_core.m_rotate, -180.0f, 180.0f);
  SliderFloatW("Scale X##tla", &m_core.m_scaleX, 0.1f, 4.0f);
  SliderFloatW("Scale Y##tla", &m_core.m_scaleY, 0.1f, 4.0f);
  SliderFloatW("Seed##tla", &m_core.m_seed, 0.0f, 64.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// HeightToOffsetNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> HeightToOffsetNode::inputSlotInfos() const {
  return {{"Height", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> HeightToOffsetNode::outputSlotInfos() const {
  return {{"X", 1}, {"Y", 1}};
}

void HeightToOffsetNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Target##hto", &m_core.m_target, 0.0f, 1.0f);
  Hint("Height level the offsets push toward");
  ImGui::PopItemWidth();
}

// ============================================================
// BevelNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> BevelNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> BevelNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void BevelNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Distance##bevel", &m_core.m_distance, 0.0f, 0.5f);
  Hint("Width of the bevel ramp around the mask (UV units)");
  if (!m_core.m_curve.empty())
    ImGui::TextDisabled("curve: %d points", (int)m_core.m_curve.size());
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> WeaveNode::inputSlotInfos() const {
  return {{"WidthMap", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> WeaveNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void WeaveNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##weave", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##weave", &m_core.m_heightIdx, s_sizesStr);
  SliderIntW("Columns##weave", &m_core.m_columns, 1, 32);
  SliderIntW("Rows##weave", &m_core.m_rows, 1, 32);
  SliderFloatW("Width##weave", &m_core.m_width, 0.0f, 1.0f);
  Hint("Stripe width (scaled per pixel by the WidthMap input)");
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> Weave2Node::inputSlotInfos() const {
  return {{"WidthMap", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> Weave2Node::outputSlotInfos() const {
  return {{"Out", 1}, {"Horizontal", 1}, {"Vertical", 1}};
}

void Weave2Node::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##weave2", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##weave2", &m_core.m_heightIdx, s_sizesStr);
  SliderIntW("Columns##weave2", &m_core.m_columns, 1, 32);
  SliderIntW("Rows##weave2", &m_core.m_rows, 1, 32);
  SliderFloatW("WidthX##weave2", &m_core.m_widthX, 0.0f, 1.0f);
  SliderFloatW("WidthY##weave2", &m_core.m_widthY, 0.0f, 1.0f);
  SliderFloatW("Stitch##weave2", &m_core.m_stitch, 1.0f, 8.0f);
  Hint("Stitch length of the weave pattern");
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> EdgeDetect2Node::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> EdgeDetect2Node::outputSlotInfos() const {
  return {{"Out", 1}};
}

void EdgeDetect2Node::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Size##ed2", &m_core.m_size, 16.0f, 2048.0f);
  Hint("Reference resolution: neighbours sampled 1/size apart");
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> SmoothMinMaxNode::inputSlotInfos() const {
  return {{"A", 1}, {"B", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> SmoothMinMaxNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void SmoothMinMaxNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Op##sminmax", &m_core.m_op, "Smooth Min\0Smooth Max\0");
  SliderFloatW("K##sminmax", &m_core.m_k, 0.0f, 1.0f);
  Hint("Smoothing amount of the min/max transition");
  SliderFloatW("Default A##sminmax", &m_core.m_def1, 0.0f, 1.0f);
  SliderFloatW("Default B##sminmax", &m_core.m_def2, 0.0f, 1.0f);
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> FillToGradientNode::inputSlotInfos()
    const {
  return {{"Fill", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillToGradientNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void FillToGradientNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Mode##ftg", &m_core.m_mode, "Stretch\0Square\0");
  SliderIntW("Layers##ftg", &m_core.m_layers, 1, 8);
  Hint("Overlapping gradients combined with min()");
  SliderFloatW("Rotate##ftg", &m_core.m_rotate, -180.0f, 180.0f);
  SliderFloatW("RndRotate##ftg", &m_core.m_rndRotate, 0.0f, 180.0f);
  Hint("Random rotation per region");
  SliderFloatW("RndOffset##ftg", &m_core.m_rndOffset, 0.0f, 1.0f);
  Hint("Random offset per region");
  SliderFloatW("Seed##ftg", &m_core.m_seed, 0.0f, 10.0f);
  if (!m_core.m_stops.empty())
    ImGui::TextDisabled("gradient: %d stops", (int)m_core.m_stops.size());
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> FillToSizeNode::inputSlotInfos() const {
  return {{"Fill", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> FillToSizeNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void FillToSizeNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Formula##fts", &m_core.m_formula,
               "Area\0Width\0Height\0Max(W, H)\0");
  Hint("Region size measure written as grayscale");
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> ColorNoiseNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> ColorNoiseNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ColorNoiseNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("W##cnoise", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##cnoise", &m_core.m_heightIdx, s_sizesStr);
  SliderIntW("Grid##cnoise", &m_core.m_grid, 1, 1024);
  Hint("Cells per axis; each cell gets one random color");
  SliderFloatW("Seed##cnoise", &m_core.m_seed, 0.0f, 10.0f);
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> DirectionalBlurNode::inputSlotInfos() const {
  return {{"In", 1}, {"Amount", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> DirectionalBlurNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void DirectionalBlurNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Sigma##dblur", &m_core.m_sigma, 0.0f, 50.0f);
  Hint("Blur strength in texels (modulated by the Amount input)");
  SliderFloatW("Angle##dblur", &m_core.m_angle, 0.0f, 360.0f);
  SliderFloatW("Size##dblur", &m_core.m_size, 16.0f, 2048.0f);
  Hint("Reference resolution: one tap = 1/size UV");
  ImGui::Combo("Mode##dblur", &m_core.m_mode, "Include center\0Skip center\0");
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> NormalBlendNode::inputSlotInfos() const {
  return {{"Foreground", 1}, {"Background", 1}, {"Mask", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> NormalBlendNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void NormalBlendNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Amount##nblend", &m_core.m_amount, 0.0f, 1.0f);
  Hint("How much of the foreground normal detail is applied");
  ImGui::PopItemWidth();
}

std::vector<ImNodes::Ez::SlotInfo> DilateNode::inputSlotInfos() const {
  return {{"Mask", 1}, {"Source", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> DilateNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void DilateNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Length##dilate", &m_core.m_length, 0.0f, 1.0f);
  Hint("How far the white areas spread (UV units)");
  SliderFloatW("Fill##dilate", &m_core.m_fill, 0.0f, 1.0f);
  Hint("0 = fade to black with distance, 1 = flat fill with the source color");
  ImGui::Combo("Metric##dilate", &m_core.m_metric,
               "Euclidean\0Manhattan\0Chebyshev\0");
  ImGui::PopItemWidth();
}
