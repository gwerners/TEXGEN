#include "AllNodes.h"
#include <imgui.h>
#include <cstring>
#include "FileDialog.h"
#include "Utils.h"

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
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      *v = sMin(*v + step, mx);
      changed = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
      *v = sMax(*v - step, mn);
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
      *v += wheel * step;
      if (*v < mn)
        *v = mn;
      if (*v > mx)
        *v = mx;
      changed = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      *v = sMin(*v + step, mx);
      changed = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
      *v = sMax(*v - step, mn);
      changed = true;
    }
  }
  return changed;
}

// ============================================================
// ColorNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ColorNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> ColorNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ColorNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##inp", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("H##inp", &m_core.m_heightIdx, s_sizesStr);
  ImGui::ColorEdit4("##inpcol", m_core.m_color, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// OutputNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> OutputNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> OutputNode::outputSlotInfos() const {
  return {};
}

void OutputNode::renderParams() {
  ImGui::PushItemWidth(160);
  ImGui::InputText("File##out", m_core.m_filename, sizeof(m_core.m_filename));
  ImGui::PopItemWidth();
}

// ============================================================
// NoiseNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> NoiseNode::inputSlotInfos() const {
  return {{"Gradient", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> NoiseNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void NoiseNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Size##noise", &m_core.m_sizeIdx, s_sizesStr);
  SliderIntW("FreqX##noise", &m_core.m_freqX, 1, 32);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Base frequency X (tile repetitions)");
  SliderIntW("FreqY##noise", &m_core.m_freqY, 1, 32);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Base frequency Y (tile repetitions)");
  SliderIntW("Octaves##noise", &m_core.m_oct, 1, 12);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Number of noise layers (detail levels)");
  SliderFloatW("FadeOff##noise", &m_core.m_fadeoff, 0.0f, 1.0f);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Amplitude decay per octave (1.0 = no decay)");
  SliderIntW("Seed##noise", &m_core.m_seed, 0, 1023);
  // Mode is a bitfield of 3 pairs from GenTexture::NoiseMode
  int signal = (m_core.m_mode & 1);      // 0=NoiseDirect, 1=NoiseAbs
  int scale = (m_core.m_mode & 2) >> 1;  // 0=NoiseUnnorm, 1=NoiseNormalize
  int type = (m_core.m_mode & 4) >> 2;   // 0=NoiseWhite,  1=NoiseBandlimit
  static const char* signals[] = {"Direct", "Abs"};
  static const char* scales[] = {"Unnormalized", "Normalize"};
  static const char* types[] = {"White", "Bandlimit (Perlin)"};
  if (ImGui::Combo("Signal##noise", &signal, signals, 2))
    m_core.m_mode = (m_core.m_mode & ~1) | signal;
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Direct: raw noise values\nAbs: absolute values only");
  if (ImGui::Combo("Scale##noise", &scale, scales, 2))
    m_core.m_mode = (m_core.m_mode & ~2) | (scale << 1);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Normalize: scale to [0,1] range");
  if (ImGui::Combo("Type##noise", &type, types, 2))
    m_core.m_mode = (m_core.m_mode & ~4) | (type << 2);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip(
        "White: random per pixel\nBandlimit: smooth Perlin noise");
  ImGui::ColorEdit3("Color1##noise", m_core.m_col1,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit3("Color2##noise", m_core.m_col2,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// CellsNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CellsNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> CellsNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void CellsNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Size##cells", &m_core.m_sizeIdx, s_sizesStr);
  SliderIntW("Centers##cells", &m_core.m_nCenters, 1, 256);
  SliderIntW("Seed##cells", &m_core.m_seed, 0, 9999);
  SliderFloatW("Amp##cells", &m_core.m_amp, 0.0f, 4.0f);
  static const char* modes[] = {"Inner", "Outer"};
  ImGui::Combo("Mode##cells", &m_core.m_mode, modes, 2);
  static const char* colorModes[] = {"Gradient", "Random"};
  ImGui::Combo("ColorMode##cells", &m_core.m_colorMode, colorModes, 2);
  ImGui::ColorEdit3("Color1##cells", m_core.m_col1,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit3("Color2##cells", m_core.m_col2,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// GlowRectNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> GlowRectNode::inputSlotInfos() const {
  return {{"Background", 1}, {"Gradient", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> GlowRectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void GlowRectNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("OrgX##gr", &m_core.m_orgx, 0.0f, 1.0f);
  SliderFloatW("OrgY##gr", &m_core.m_orgy, 0.0f, 1.0f);
  SliderFloatW("UX##gr", &m_core.m_ux, -1.0f, 1.0f);
  SliderFloatW("UY##gr", &m_core.m_uy, -1.0f, 1.0f);
  SliderFloatW("VX##gr", &m_core.m_vx, -1.0f, 1.0f);
  SliderFloatW("VY##gr", &m_core.m_vy, -1.0f, 1.0f);
  SliderFloatW("RectU##gr", &m_core.m_rectu, 0.0f, 1.0f);
  SliderFloatW("RectV##gr", &m_core.m_rectv, 0.0f, 1.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// ColorMatrixNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ColorMatrixNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> ColorMatrixNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ColorMatrixNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Text("Matrix (row-major):");
  for (int r = 0; r < 4; r++) {
    ImGui::PushID(r);
    for (int c = 0; c < 4; c++) {
      ImGui::PushID(c);
      if (c > 0)
        ImGui::SameLine();
      ImGui::DragFloat("##m", &m_core.m_matrix[r * 4 + c], 0.01f, -4.0f, 4.0f,
                       "%.2f");
      ImGui::PopID();
    }
    ImGui::PopID();
  }
  ImGui::Checkbox("ClampPremult##cm", &m_core.m_clampPremult);
  ImGui::PopItemWidth();
}

// ============================================================
// CoordMatrixNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CoordMatrixNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> CoordMatrixNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void CoordMatrixNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Text("Matrix (row-major):");
  for (int r = 0; r < 4; r++) {
    ImGui::PushID(r + 100);
    for (int c = 0; c < 4; c++) {
      ImGui::PushID(c);
      if (c > 0)
        ImGui::SameLine();
      ImGui::DragFloat("##cm", &m_core.m_matrix[r * 4 + c], 0.01f, -4.0f, 4.0f,
                       "%.2f");
      ImGui::PopID();
    }
    ImGui::PopID();
  }
  static const char* fmodes[] = {"WrapNearest", "ClampNearest", "WrapBilinear",
                                 "ClampBilinear"};
  ImGui::Combo("Filter##coordm", &m_core.m_filterMode, fmodes, 4);
  ImGui::PopItemWidth();
}

// ============================================================
// ColorRemapNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ColorRemapNode::inputSlotInfos() const {
  return {{"In", 1}, {"MapR", 1}, {"MapG", 1}, {"MapB", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> ColorRemapNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

// ============================================================
// CoordRemapNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CoordRemapNode::inputSlotInfos() const {
  return {{"In", 1}, {"Remap", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> CoordRemapNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void CoordRemapNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("StrengthU##cr", &m_core.m_strengthU, 0.0f, 4.0f);
  SliderFloatW("StrengthV##cr", &m_core.m_strengthV, 0.0f, 4.0f);
  static const char* fmodes[] = {"WrapNearest", "ClampNearest", "WrapBilinear",
                                 "ClampBilinear"};
  ImGui::Combo("Filter##cr", &m_core.m_filterMode, fmodes, 4);
  ImGui::PopItemWidth();
}

// ============================================================
// DeriveNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> DeriveNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> DeriveNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void DeriveNode::renderParams() {
  ImGui::PushItemWidth(120);
  static const char* ops[] = {"Gradient", "Normals"};
  ImGui::Combo("Op##derive", &m_core.m_op, ops, 2);
  SliderFloatW("Strength##derive", &m_core.m_strength, 0.0f, 32.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// BlurNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> BlurNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> BlurNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void BlurNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("SizeX##blur", &m_core.m_sizex, 0.0f, 0.5f);
  SliderFloatW("SizeY##blur", &m_core.m_sizey, 0.0f, 0.5f);
  SliderIntW("Order##blur", &m_core.m_order, 1, 8);
  SliderIntW("Mode##blur", &m_core.m_mode, 0, 3);
  ImGui::PopItemWidth();
}

// ============================================================
// TernaryNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> TernaryNode::inputSlotInfos() const {
  return {{"Image1", 1}, {"Image2", 1}, {"Mask", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> TernaryNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void TernaryNode::renderParams() {
  ImGui::PushItemWidth(120);
  static const char* ops[] = {"Lerp", "Select"};
  ImGui::Combo("Op##tern", &m_core.m_op, ops, 2);
  ImGui::PopItemWidth();
}

// ============================================================
// PasteNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> PasteNode::inputSlotInfos() const {
  return {{"Background", 1}, {"Snippet", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> PasteNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void PasteNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("OrgX##paste", &m_core.m_orgx, 0.0f, 1.0f);
  SliderFloatW("OrgY##paste", &m_core.m_orgy, 0.0f, 1.0f);
  SliderFloatW("UX##paste", &m_core.m_ux, -1.0f, 1.0f);
  SliderFloatW("UY##paste", &m_core.m_uy, -1.0f, 1.0f);
  SliderFloatW("VX##paste", &m_core.m_vx, -1.0f, 1.0f);
  SliderFloatW("VY##paste", &m_core.m_vy, -1.0f, 1.0f);
  static const char* ops[] = {"Add",      "Sub",      "MulC",     "Min",
                              "Max",      "SetAlpha", "PreAlpha", "Over",
                              "Multiply", "Screen",   "Darken",   "Lighten"};
  ImGui::Combo("Op##paste", &m_core.m_op, ops, 12);
  SliderIntW("Mode##paste", &m_core.m_mode, 0, 3);
  ImGui::PopItemWidth();
}

// ============================================================
// BumpNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> BumpNode::inputSlotInfos() const {
  return {{"Surface", 1}, {"Normals", 1}, {"Specular", 1}, {"Falloff", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> BumpNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void BumpNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Text("Light Position:");
  SliderFloatW("PX##bump", &m_core.m_px, -2.0f, 2.0f);
  SliderFloatW("PY##bump", &m_core.m_py, -2.0f, 2.0f);
  SliderFloatW("PZ##bump", &m_core.m_pz, -2.0f, 2.0f);
  ImGui::Text("Light Direction:");
  SliderFloatW("DX##bump", &m_core.m_dx, -1.0f, 1.0f);
  SliderFloatW("DY##bump", &m_core.m_dy, -1.0f, 1.0f);
  SliderFloatW("DZ##bump", &m_core.m_dz, -1.0f, 1.0f);
  ImGui::ColorEdit4("Ambient##bump", m_core.m_ambient,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Diffuse##bump", m_core.m_diffuse,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Directional##bump", &m_core.m_directional);
  ImGui::PopItemWidth();
}

// ============================================================
// LinearCombineNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> LinearCombineNode::inputSlotInfos() const {
  return {{"Image1", 1}, {"Image2", 1}, {"Image3", 1}, {"Image4", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> LinearCombineNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void LinearCombineNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::ColorEdit4("ConstColor##lc", m_core.m_constColor,
                    ImGuiColorEditFlags_NoInputs);
  SliderFloatW("ConstW##lc", &m_core.m_constWeight, 0.0f, 1.0f);
  for (int i = 0; i < 4; i++) {
    ImGui::PushID(i);
    ImGui::Text("Input %d:", i + 1);
    SliderFloatW("W##lc", &m_core.m_weights[i], 0.0f, 1.0f);
    SliderFloatW("US##lc", &m_core.m_uShift[i], -1.0f, 1.0f);
    SliderFloatW("VS##lc", &m_core.m_vShift[i], -1.0f, 1.0f);
    static const char* fmodes[] = {"WrapNearest", "ClampNearest",
                                   "WrapBilinear", "ClampBilinear"};
    ImGui::Combo("F##lc", &m_core.m_filterMode[i], fmodes, 4);
    ImGui::PopID();
  }
  ImGui::PopItemWidth();
}

// ============================================================
// CrystalNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CrystalNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> CrystalNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void CrystalNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##cry", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##cry", &m_core.m_heightIdx, s_sizesStr);
  SliderIntW("Seed##cry", &m_core.m_seed, 0, 9999);
  SliderIntW("Count##cry", &m_core.m_count, 1, 256);
  ImGui::ColorEdit4("Near##cry", m_core.m_colorNear,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Far##cry", m_core.m_colorFar,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// DirectionalGradientNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> DirectionalGradientNode::inputSlotInfos()
    const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> DirectionalGradientNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}

void DirectionalGradientNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##dg", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##dg", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("X1##dg", &m_core.m_x1, 0.0f, 1.0f);
  SliderFloatW("Y1##dg", &m_core.m_y1, 0.0f, 1.0f);
  SliderFloatW("X2##dg", &m_core.m_x2, 0.0f, 1.0f);
  SliderFloatW("Y2##dg", &m_core.m_y2, 0.0f, 1.0f);
  ImGui::ColorEdit4("Color1##dg", m_core.m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##dg", m_core.m_col2, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// GlowEffectNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> GlowEffectNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> GlowEffectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void GlowEffectNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##ge", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##ge", &m_core.m_heightIdx, s_sizesStr);
  SliderFloatW("CX##ge", &m_core.m_cx, 0.0f, 1.0f);
  SliderFloatW("CY##ge", &m_core.m_cy, 0.0f, 1.0f);
  SliderFloatW("Scale##ge", &m_core.m_scale, 0.01f, 5.0f);
  SliderFloatW("Exponent##ge", &m_core.m_exponent, 0.1f, 10.0f);
  SliderFloatW("Intensity##ge", &m_core.m_intensity, 0.0f, 4.0f);
  ImGui::ColorEdit4("BG##ge", m_core.m_bgCol, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Glow##ge", m_core.m_glowCol, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// PerlinNoiseRG2Node
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> PerlinNoiseRG2Node::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> PerlinNoiseRG2Node::outputSlotInfos() const {
  return {{"Out", 1}};
}

void PerlinNoiseRG2Node::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##pn", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##pn", &m_core.m_heightIdx, s_sizesStr);
  SliderIntW("Octaves##pn", &m_core.m_octaves, 1, 12);
  SliderFloatW("Persistence##pn", &m_core.m_persistence, 0.0f, 1.0f);
  SliderIntW("FreqScale##pn", &m_core.m_freqScale, 0, 10);
  SliderIntW("Seed##pn", &m_core.m_seed, 0, 9999);
  SliderFloatW("Contrast##pn", &m_core.m_contrast, 0.01f, 4.0f);
  SliderIntW("StartOct##pn", &m_core.m_startOctave, 0, 11);
  ImGui::ColorEdit4("Color1##pn", m_core.m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##pn", m_core.m_col2, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// BlurKernelNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> BlurKernelNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> BlurKernelNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void BlurKernelNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("RadiusX##bk", &m_core.m_radiusX, 0.0f, 0.5f);
  SliderFloatW("RadiusY##bk", &m_core.m_radiusY, 0.0f, 0.5f);
  static const char* ktypes[] = {"Box", "Triangle", "Gaussian"};
  ImGui::Combo("Kernel##bk", &m_core.m_kernelType, ktypes, 3);
  static const char* wmodes[] = {"WrapUV", "ClampU", "ClampV", "ClampUV"};
  ImGui::Combo("Wrap##bk", &m_core.m_wrapMode, wmodes, 4);
  ImGui::PopItemWidth();
}

// ============================================================
// HSCBNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> HSCBNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> HSCBNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void HSCBNode::renderParams() {
  ImGui::PushItemWidth(120);
  SliderFloatW("Hue##hscb", &m_core.m_hue, 0.0f, 1.0f);
  SliderFloatW("Sat##hscb", &m_core.m_sat, 0.0f, 4.0f);
  SliderFloatW("Contrast##hscb", &m_core.m_contrast, 0.01f, 4.0f);
  SliderFloatW("Brightness##hscb", &m_core.m_brightness, 0.0f, 4.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// WaveletNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> WaveletNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> WaveletNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void WaveletNode::renderParams() {
  ImGui::PushItemWidth(120);
  static const char* modes[] = {"Forward", "Inverse"};
  ImGui::Combo("Mode##wav", &m_core.m_mode, modes, 2);
  SliderIntW("Count##wav", &m_core.m_count, 1, 8);
  ImGui::PopItemWidth();
}

// ============================================================
// ColorBalanceNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ColorBalanceNode::inputSlotInfos() const {
  return {{"In", 1}};
}

std::vector<ImNodes::Ez::SlotInfo> ColorBalanceNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ColorBalanceNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Text("Shadows (R/G/B):");
  SliderFloatW("SR##cb", &m_core.m_shadow[0], -1.0f, 1.0f);
  SliderFloatW("SG##cb", &m_core.m_shadow[1], -1.0f, 1.0f);
  SliderFloatW("SB##cb", &m_core.m_shadow[2], -1.0f, 1.0f);
  ImGui::Text("Midtones (R/G/B):");
  SliderFloatW("MR##cb", &m_core.m_mid[0], -1.0f, 1.0f);
  SliderFloatW("MG##cb", &m_core.m_mid[1], -1.0f, 1.0f);
  SliderFloatW("MB##cb", &m_core.m_mid[2], -1.0f, 1.0f);
  ImGui::Text("Highlights (R/G/B):");
  SliderFloatW("HR##cb", &m_core.m_highlight[0], -1.0f, 1.0f);
  SliderFloatW("HG##cb", &m_core.m_highlight[1], -1.0f, 1.0f);
  SliderFloatW("HB##cb", &m_core.m_highlight[2], -1.0f, 1.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// BricksNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> BricksNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> BricksNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void BricksNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##bricks", &m_core.m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##bricks", &m_core.m_heightIdx, s_sizesStr);
  ImGui::ColorEdit4("Color0##bricks", m_core.m_col0,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color1##bricks", m_core.m_col1,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Fuge##bricks", m_core.m_colFuge,
                    ImGuiColorEditFlags_NoInputs);
  SliderFloatW("FugeX##bricks", &m_core.m_fugeX, 0.0f, 0.5f);
  SliderFloatW("FugeY##bricks", &m_core.m_fugeY, 0.0f, 0.5f);
  SliderIntW("TileX##bricks", &m_core.m_tileX, 1, 32);
  SliderIntW("TileY##bricks", &m_core.m_tileY, 1, 32);
  SliderIntW("Seed##bricks", &m_core.m_seed, 0, 9999);
  SliderIntW("Heads##bricks", &m_core.m_heads, 0, 255);
  SliderFloatW("ColorBalance##bricks", &m_core.m_colorBalance, 0.01f, 4.0f);
  ImGui::PopItemWidth();
}

// ============================================================
// GradientNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> GradientNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> GradientNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void GradientNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::ColorEdit4("Color1##grad", m_core.m_col1,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##grad", m_core.m_col2,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

// ============================================================
// ImageNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> ImageNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> ImageNode::outputSlotInfos() const {
  return {{"Out", 1}};
}

void ImageNode::renderParams() {
  static FileDialog s_imgDialog;

  ImGui::PushItemWidth(140);
  ImGui::InputText("##imgfile", m_core.m_filename, sizeof(m_core.m_filename));
  ImGui::PopItemWidth();
  ImGui::SameLine();
  if (ImGui::SmallButton("...##imgbrowse")) {
    s_imgDialog.open(m_core.m_filename,
                     ".png,.jpg,.jpeg,.tga,.bmp,.psd,.gif,.hdr");
  }
  if (s_imgDialog.show("Select Image", FileDialog::Load, nullptr)) {
    std::string path = s_imgDialog.getResultPath();
    strncpy(m_core.m_filename, path.c_str(), sizeof(m_core.m_filename) - 1);
    m_core.m_filename[sizeof(m_core.m_filename) - 1] = '\0';
    m_core.m_loaded = false;
  }
  if (m_core.m_loaded) {
    ImGui::Text("%dx%d", m_core.m_cache.XRes, m_core.m_cache.YRes);
  } else if (m_core.m_filename[0] != '\0') {
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Not loaded");
  }
}

// ============================================================
// CommentNode
// ============================================================

std::vector<ImNodes::Ez::SlotInfo> CommentNode::inputSlotInfos() const {
  return {};
}

std::vector<ImNodes::Ez::SlotInfo> CommentNode::outputSlotInfos() const {
  return {};
}

void CommentNode::renderParams() {
  ImGui::PushItemWidth(200);
  ImGui::InputTextMultiline("##comment", m_core.m_text, sizeof(m_core.m_text),
                            ImVec2(200, 80));
  ImGui::PopItemWidth();
  ImGui::ColorEdit3("Color##comment", m_core.m_color,
                    ImGuiColorEditFlags_NoInputs);
}
