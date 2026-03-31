#include "AllNodes.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "Utils.h"

// ============================================================
// Helpers
// ============================================================

static const char* s_sizesStr = "32\00064\000128\000256\000512\0001024\000";
static int indexToSize(int idx) {
  return 32 << idx;
}

static sU32 colorToU32(const float c[4]) {
  sU32 a = (sU32)(sClamp(c[3], 0.0f, 1.0f) * 255.0f + 0.5f);
  sU32 r = (sU32)(sClamp(c[0], 0.0f, 1.0f) * 255.0f + 0.5f);
  sU32 g = (sU32)(sClamp(c[1], 0.0f, 1.0f) * 255.0f + 0.5f);
  sU32 b = (sU32)(sClamp(c[2], 0.0f, 1.0f) * 255.0f + 0.5f);
  return (a << 24) | (r << 16) | (g << 8) | b;
}

static void u32ToColor(sU32 col, float c[4]) {
  c[0] = ((col >> 16) & 0xff) / 255.0f;
  c[1] = ((col >> 8) & 0xff) / 255.0f;
  c[2] = (col & 0xff) / 255.0f;
  c[3] = ((col >> 24) & 0xff) / 255.0f;
}

static GenTexture& getFallback() {
  static GenTexture fallback;
  if (!fallback.Data)
    fallback.Init(256, 256);
  return fallback;
}

static GenTexture& getDefaultGradient() {
  static GenTexture grad;
  if (!grad.Data)
    grad = LinearGradient(0xff000000, 0xffffffff);
  return grad;
}

static GenTexture* ensureInput(GenTexture* ptr) {
  if (!ptr)
    return &getFallback();
  if (!ptr->Data)
    ptr->Init(256, 256);
  return ptr;
}

static GenTexture* ensureGradient(GenTexture* ptr) {
  if (!ptr || !ptr->Data)
    return &getDefaultGradient();
  return ptr;
}

static void applyBlend(std::vector<GenTexture>& outputs,
                       const std::vector<GenTexture*>& inputs,
                       int blendMode) {
  if (blendMode <= 0 || inputs.empty() || !inputs[0] || !inputs[0]->Data)
    return;
  if (outputs.empty() || !outputs[0].Data)
    return;
  static const GenTexture::CombineOp ops[] = {
      GenTexture::CombineAdd,      GenTexture::CombineSub,
      GenTexture::CombineMulC,     GenTexture::CombineMin,
      GenTexture::CombineMax,      GenTexture::CombineOver,
      GenTexture::CombineMultiply, GenTexture::CombineScreen,
      GenTexture::CombineDarken,   GenTexture::CombineLighten,
  };
  int idx = blendMode - 1;
  if (idx < 0 || idx >= 10)
    return;
  GenTexture blended;
  blended.Init(outputs[0].XRes, outputs[0].YRes);
  blended.Paste(*inputs[0], outputs[0], 0, 0, 1, 0, 0, 1, ops[idx], 0);
  outputs[0] = blended;
}

// ============================================================
// InputNode
// ============================================================

InputNode::InputNode() : m_widthIdx(3), m_heightIdx(3) {
  m_color[0] = 0.0f;
  m_color[1] = 0.0f;
  m_color[2] = 0.0f;
  m_color[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> InputNode::inputSlotInfos() const {
  return {};
}
std::vector<ImNodes::Ez::SlotInfo> InputNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> InputNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> InputNode::outputSlotNames() const {
  return {"Out"};
}

void InputNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                        std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  sU32 col = colorToU32(m_color);
  for (int i = 0; i < outputs[0].NPixels; i++)
    outputs[0].Data[i].Init(col);
}

void InputNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::Combo("W##inp", &m_widthIdx, s_sizesStr);
  ImGui::Combo("H##inp", &m_heightIdx, s_sizesStr);
  ImGui::ColorEdit4("##inpcol", m_color, ImGuiColorEditFlags_NoInputs);
  ImGui::PopItemWidth();
}

nlohmann::json InputNode::saveParams() const {
  return {{"widthIdx", m_widthIdx}, {"heightIdx", m_heightIdx},
          {"r", m_color[0]},        {"g", m_color[1]},
          {"b", m_color[2]},        {"a", m_color[3]}};
}

void InputNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("r"))
    m_color[0] = j["r"];
  if (j.contains("g"))
    m_color[1] = j["g"];
  if (j.contains("b"))
    m_color[2] = j["b"];
  if (j.contains("a"))
    m_color[3] = j["a"];
}

// ============================================================
// OutputNode
// ============================================================

OutputNode::OutputNode() {
  strncpy(m_filename, "output.tga", sizeof(m_filename));
}

std::vector<ImNodes::Ez::SlotInfo> OutputNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> OutputNode::outputSlotInfos() const {
  return {};
}
std::vector<std::string> OutputNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> OutputNode::outputSlotNames() const {
  return {};
}

void OutputNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  outputs.resize(0);
  if (!inputs.empty() && inputs[0] && inputs[0]->Data) {
    SaveImage(*inputs[0], m_filename);
  }
}

void OutputNode::renderParams() {
  ImGui::PushItemWidth(160);
  ImGui::InputText("File##out", m_filename, sizeof(m_filename));
  ImGui::PopItemWidth();
}

nlohmann::json OutputNode::saveParams() const {
  return {{"filename", m_filename}};
}

void OutputNode::loadParams(const nlohmann::json& j) {
  if (j.contains("filename")) {
    std::string s = j["filename"];
    strncpy(m_filename, s.c_str(), sizeof(m_filename) - 1);
    m_filename[sizeof(m_filename) - 1] = '\0';
  }
}

// ============================================================
// NoiseNode
// ============================================================

NoiseNode::NoiseNode()
    : m_freqX(4),
      m_freqY(4),
      m_oct(4),
      m_seed(0),
      m_mode(4),
      m_fadeoff(0.5f),
      m_sizeIdx(3),
      m_blendMode(0) {
  m_col1[0] = 0;
  m_col1[1] = 0;
  m_col1[2] = 0;
  m_col1[3] = 1;
  m_col2[0] = 1;
  m_col2[1] = 1;
  m_col2[2] = 1;
  m_col2[3] = 1;
}

std::vector<ImNodes::Ez::SlotInfo> NoiseNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> NoiseNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> NoiseNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> NoiseNode::outputSlotNames() const {
  return {"Out"};
}

void NoiseNode::execute(const std::vector<GenTexture*>& inputs,
                        std::vector<GenTexture>& outputs) {
  int sz = indexToSize(m_sizeIdx);
  outputs.resize(1);
  outputs[0].Init(sz, sz);
  GenTexture grad = LinearGradient(colorToU32(m_col1), colorToU32(m_col2));
  outputs[0].Noise(grad, m_freqX, m_freqY, m_oct, m_fadeoff, m_seed, m_mode);
  applyBlend(outputs, inputs, m_blendMode);
}

void NoiseNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Size##noise", &m_sizeIdx, s_sizesStr);
  ImGui::SliderInt("FreqX##noise", &m_freqX, 1, 32);
  ImGui::SliderInt("FreqY##noise", &m_freqY, 1, 32);
  ImGui::SliderInt("Octaves##noise", &m_oct, 1, 12);
  ImGui::SliderFloat("FadeOff##noise", &m_fadeoff, 0.0f, 1.0f);
  ImGui::SliderInt("Seed##noise", &m_seed, 0, 1023);
  static const char* modes[] = {"Direct",       "Abs",      "DirectNorm",
                                "AbsNorm",      "DirectBL", "AbsBL",
                                "DirectNormBL", "AbsNormBL"};
  ImGui::Combo("Mode##noise", &m_mode, modes, 8);
  ImGui::ColorEdit4("Color1##noise", m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##noise", m_col2, ImGuiColorEditFlags_NoInputs);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##noise", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json NoiseNode::saveParams() const {
  return {{"sizeIdx", m_sizeIdx}, {"freqX", m_freqX},
          {"freqY", m_freqY},     {"oct", m_oct},
          {"fadeoff", m_fadeoff}, {"seed", m_seed},
          {"mode", m_mode},       {"c1r", m_col1[0]},
          {"c1g", m_col1[1]},     {"c1b", m_col1[2]},
          {"c1a", m_col1[3]},     {"c2r", m_col2[0]},
          {"c2g", m_col2[1]},     {"c2b", m_col2[2]},
          {"c2a", m_col2[3]},     {"blendMode", m_blendMode}};
}

void NoiseNode::loadParams(const nlohmann::json& j) {
  if (j.contains("sizeIdx"))
    m_sizeIdx = j["sizeIdx"];
  if (j.contains("freqX"))
    m_freqX = j["freqX"];
  if (j.contains("freqY"))
    m_freqY = j["freqY"];
  if (j.contains("oct"))
    m_oct = j["oct"];
  if (j.contains("fadeoff"))
    m_fadeoff = j["fadeoff"];
  if (j.contains("seed"))
    m_seed = j["seed"];
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("c1r"))
    m_col1[0] = j["c1r"];
  if (j.contains("c1g"))
    m_col1[1] = j["c1g"];
  if (j.contains("c1b"))
    m_col1[2] = j["c1b"];
  if (j.contains("c1a"))
    m_col1[3] = j["c1a"];
  if (j.contains("c2r"))
    m_col2[0] = j["c2r"];
  if (j.contains("c2g"))
    m_col2[1] = j["c2g"];
  if (j.contains("c2b"))
    m_col2[2] = j["c2b"];
  if (j.contains("c2a"))
    m_col2[3] = j["c2a"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// CellsNode
// ============================================================

CellsNode::CellsNode()
    : m_nCenters(16),
      m_seed(42),
      m_mode(0),
      m_amp(1.0f),
      m_sizeIdx(3),
      m_blendMode(0) {
  m_col1[0] = 0;
  m_col1[1] = 0;
  m_col1[2] = 0;
  m_col1[3] = 1;
  m_col2[0] = 1;
  m_col2[1] = 1;
  m_col2[2] = 1;
  m_col2[3] = 1;
}

std::vector<ImNodes::Ez::SlotInfo> CellsNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CellsNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> CellsNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> CellsNode::outputSlotNames() const {
  return {"Out"};
}

void CellsNode::execute(const std::vector<GenTexture*>& inputs,
                        std::vector<GenTexture>& outputs) {
  int sz = indexToSize(m_sizeIdx);
  outputs.resize(1);
  outputs[0].Init(sz, sz);

  GenTexture grad = LinearGradient(colorToU32(m_col1), colorToU32(m_col2));

  // Auto-generate CellCenter array from seed+count using LCG
  std::vector<CellCenter> centers(m_nCenters);
  sU32 rng = (sU32)m_seed;
  rng = rng * 0x8af6f2ac + 0x1757286;
  for (int i = 0; i < m_nCenters; i++) {
    rng = rng * 0x15a4e35 + 1;
    centers[i].x = (float)((rng >> 8) & 0xffff) / 65536.0f;
    rng = rng * 0x15a4e35 + 1;
    centers[i].y = (float)((rng >> 8) & 0xffff) / 65536.0f;
    sInt gx = (sInt)(centers[i].x * (1 << 24));
    grad.SampleGradient(centers[i].color, gx);
  }

  outputs[0].Cells(grad, centers.data(), m_nCenters, m_amp, m_mode);
  applyBlend(outputs, inputs, m_blendMode);
}

void CellsNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Size##cells", &m_sizeIdx, s_sizesStr);
  ImGui::SliderInt("Centers##cells", &m_nCenters, 1, 256);
  ImGui::SliderInt("Seed##cells", &m_seed, 0, 9999);
  ImGui::SliderFloat("Amp##cells", &m_amp, 0.0f, 4.0f);
  static const char* modes[] = {"Inner", "Outer"};
  ImGui::Combo("Mode##cells", &m_mode, modes, 2);
  ImGui::ColorEdit4("Color1##cells", m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##cells", m_col2, ImGuiColorEditFlags_NoInputs);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##cells", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json CellsNode::saveParams() const {
  return {{"sizeIdx", m_sizeIdx}, {"nCenters", m_nCenters},  {"seed", m_seed},
          {"amp", m_amp},         {"mode", m_mode},          {"c1r", m_col1[0]},
          {"c1g", m_col1[1]},     {"c1b", m_col1[2]},        {"c1a", m_col1[3]},
          {"c2r", m_col2[0]},     {"c2g", m_col2[1]},        {"c2b", m_col2[2]},
          {"c2a", m_col2[3]},     {"blendMode", m_blendMode}};
}

void CellsNode::loadParams(const nlohmann::json& j) {
  if (j.contains("sizeIdx"))
    m_sizeIdx = j["sizeIdx"];
  if (j.contains("nCenters"))
    m_nCenters = j["nCenters"];
  if (j.contains("seed"))
    m_seed = j["seed"];
  if (j.contains("amp"))
    m_amp = j["amp"];
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("c1r"))
    m_col1[0] = j["c1r"];
  if (j.contains("c1g"))
    m_col1[1] = j["c1g"];
  if (j.contains("c1b"))
    m_col1[2] = j["c1b"];
  if (j.contains("c1a"))
    m_col1[3] = j["c1a"];
  if (j.contains("c2r"))
    m_col2[0] = j["c2r"];
  if (j.contains("c2g"))
    m_col2[1] = j["c2g"];
  if (j.contains("c2b"))
    m_col2[2] = j["c2b"];
  if (j.contains("c2a"))
    m_col2[3] = j["c2a"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// GlowRectNode
// ============================================================

GlowRectNode::GlowRectNode()
    : m_orgx(0.5f),
      m_orgy(0.5f),
      m_ux(0.3f),
      m_uy(0.0f),
      m_vx(0.0f),
      m_vy(0.3f),
      m_rectu(0.8f),
      m_rectv(0.8f) {}

std::vector<ImNodes::Ez::SlotInfo> GlowRectNode::inputSlotInfos() const {
  return {{"Background", 1}, {"Gradient", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> GlowRectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> GlowRectNode::inputSlotNames() const {
  return {"Background", "Gradient"};
}
std::vector<std::string> GlowRectNode::outputSlotNames() const {
  return {"Out"};
}

void GlowRectNode::execute(const std::vector<GenTexture*>& inputs,
                           std::vector<GenTexture>& outputs) {
  GenTexture* bg = ensureInput(inputs[0]);
  GenTexture* grad = ensureGradient(inputs[1]);
  outputs.resize(1);
  outputs[0].Init(bg->XRes, bg->YRes);
  outputs[0].GlowRect(*bg, *grad, m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy,
                      m_rectu, m_rectv);
}

void GlowRectNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::SliderFloat("OrgX##gr", &m_orgx, 0.0f, 1.0f);
  ImGui::SliderFloat("OrgY##gr", &m_orgy, 0.0f, 1.0f);
  ImGui::SliderFloat("UX##gr", &m_ux, -1.0f, 1.0f);
  ImGui::SliderFloat("UY##gr", &m_uy, -1.0f, 1.0f);
  ImGui::SliderFloat("VX##gr", &m_vx, -1.0f, 1.0f);
  ImGui::SliderFloat("VY##gr", &m_vy, -1.0f, 1.0f);
  ImGui::SliderFloat("RectU##gr", &m_rectu, 0.0f, 1.0f);
  ImGui::SliderFloat("RectV##gr", &m_rectv, 0.0f, 1.0f);
  ImGui::PopItemWidth();
}

nlohmann::json GlowRectNode::saveParams() const {
  return {{"orgx", m_orgx},   {"orgy", m_orgy},  {"ux", m_ux},
          {"uy", m_uy},       {"vx", m_vx},      {"vy", m_vy},
          {"rectu", m_rectu}, {"rectv", m_rectv}};
}

void GlowRectNode::loadParams(const nlohmann::json& j) {
  if (j.contains("orgx"))
    m_orgx = j["orgx"];
  if (j.contains("orgy"))
    m_orgy = j["orgy"];
  if (j.contains("ux"))
    m_ux = j["ux"];
  if (j.contains("uy"))
    m_uy = j["uy"];
  if (j.contains("vx"))
    m_vx = j["vx"];
  if (j.contains("vy"))
    m_vy = j["vy"];
  if (j.contains("rectu"))
    m_rectu = j["rectu"];
  if (j.contains("rectv"))
    m_rectv = j["rectv"];
}

// ============================================================
// ColorMatrixNode
// ============================================================

ColorMatrixNode::ColorMatrixNode() : m_clampPremult(false) {
  // Identity matrix
  for (int i = 0; i < 16; i++)
    m_matrix[i] = 0.0f;
  m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> ColorMatrixNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> ColorMatrixNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> ColorMatrixNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> ColorMatrixNode::outputSlotNames() const {
  return {"Out"};
}

void ColorMatrixNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  Matrix44 mat;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      mat[r][c] = m_matrix[r * 4 + c];
  outputs[0].ColorMatrixTransform(*in, mat, m_clampPremult ? 1 : 0);
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
      ImGui::DragFloat("##m", &m_matrix[r * 4 + c], 0.01f, -4.0f, 4.0f, "%.2f");
      ImGui::PopID();
    }
    ImGui::PopID();
  }
  ImGui::Checkbox("ClampPremult##cm", &m_clampPremult);
  ImGui::PopItemWidth();
}

nlohmann::json ColorMatrixNode::saveParams() const {
  nlohmann::json j;
  for (int i = 0; i < 16; i++)
    j["matrix"][i] = m_matrix[i];
  j["clampPremult"] = m_clampPremult;
  return j;
}

void ColorMatrixNode::loadParams(const nlohmann::json& j) {
  if (j.contains("matrix"))
    for (int i = 0; i < 16; i++)
      m_matrix[i] = j["matrix"][i];
  if (j.contains("clampPremult"))
    m_clampPremult = j["clampPremult"];
}

// ============================================================
// CoordMatrixNode
// ============================================================

CoordMatrixNode::CoordMatrixNode() : m_filterMode(0) {
  for (int i = 0; i < 16; i++)
    m_matrix[i] = 0.0f;
  m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> CoordMatrixNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CoordMatrixNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> CoordMatrixNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> CoordMatrixNode::outputSlotNames() const {
  return {"Out"};
}

void CoordMatrixNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  Matrix44 mat;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      mat[r][c] = m_matrix[r * 4 + c];
  outputs[0].CoordMatrixTransform(*in, mat, m_filterMode);
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
      ImGui::DragFloat("##cm", &m_matrix[r * 4 + c], 0.01f, -4.0f, 4.0f,
                       "%.2f");
      ImGui::PopID();
    }
    ImGui::PopID();
  }
  static const char* fmodes[] = {"WrapNearest", "ClampNearest", "WrapBilinear",
                                 "ClampBilinear"};
  ImGui::Combo("Filter##coordm", &m_filterMode, fmodes, 4);
  ImGui::PopItemWidth();
}

nlohmann::json CoordMatrixNode::saveParams() const {
  nlohmann::json j;
  for (int i = 0; i < 16; i++)
    j["matrix"][i] = m_matrix[i];
  j["filterMode"] = m_filterMode;
  return j;
}

void CoordMatrixNode::loadParams(const nlohmann::json& j) {
  if (j.contains("matrix"))
    for (int i = 0; i < 16; i++)
      m_matrix[i] = j["matrix"][i];
  if (j.contains("filterMode"))
    m_filterMode = j["filterMode"];
}

// ============================================================
// ColorRemapNode
// ============================================================

ColorRemapNode::ColorRemapNode() {}

std::vector<ImNodes::Ez::SlotInfo> ColorRemapNode::inputSlotInfos() const {
  return {{"In", 1}, {"MapR", 1}, {"MapG", 1}, {"MapB", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> ColorRemapNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> ColorRemapNode::inputSlotNames() const {
  return {"Input", "MapR", "MapG", "MapB"};
}
std::vector<std::string> ColorRemapNode::outputSlotNames() const {
  return {"Out"};
}

void ColorRemapNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  GenTexture* mapR = ensureInput(inputs[1]);
  GenTexture* mapG = ensureInput(inputs[2]);
  GenTexture* mapB = ensureInput(inputs[3]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[0].ColorRemap(*in, *mapR, *mapG, *mapB);
}

// ============================================================
// CoordRemapNode
// ============================================================

CoordRemapNode::CoordRemapNode()
    : m_strengthU(1.0f), m_strengthV(1.0f), m_filterMode(0) {}

std::vector<ImNodes::Ez::SlotInfo> CoordRemapNode::inputSlotInfos() const {
  return {{"In", 1}, {"Remap", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CoordRemapNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> CoordRemapNode::inputSlotNames() const {
  return {"Input", "Remap"};
}
std::vector<std::string> CoordRemapNode::outputSlotNames() const {
  return {"Out"};
}

void CoordRemapNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  GenTexture* remap = ensureInput(inputs[1]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[0].CoordRemap(*in, *remap, m_strengthU, m_strengthV, m_filterMode);
}

void CoordRemapNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::SliderFloat("StrengthU##cr", &m_strengthU, 0.0f, 4.0f);
  ImGui::SliderFloat("StrengthV##cr", &m_strengthV, 0.0f, 4.0f);
  static const char* fmodes[] = {"WrapNearest", "ClampNearest", "WrapBilinear",
                                 "ClampBilinear"};
  ImGui::Combo("Filter##cr", &m_filterMode, fmodes, 4);
  ImGui::PopItemWidth();
}

nlohmann::json CoordRemapNode::saveParams() const {
  return {{"strengthU", m_strengthU},
          {"strengthV", m_strengthV},
          {"filterMode", m_filterMode}};
}

void CoordRemapNode::loadParams(const nlohmann::json& j) {
  if (j.contains("strengthU"))
    m_strengthU = j["strengthU"];
  if (j.contains("strengthV"))
    m_strengthV = j["strengthV"];
  if (j.contains("filterMode"))
    m_filterMode = j["filterMode"];
}

// ============================================================
// DeriveNode
// ============================================================

DeriveNode::DeriveNode() : m_op(0), m_strength(4.0f) {}

std::vector<ImNodes::Ez::SlotInfo> DeriveNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> DeriveNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> DeriveNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> DeriveNode::outputSlotNames() const {
  return {"Out"};
}

void DeriveNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[0].Derive(*in, (GenTexture::DeriveOp)m_op, m_strength);
}

void DeriveNode::renderParams() {
  ImGui::PushItemWidth(120);
  static const char* ops[] = {"Gradient", "Normals"};
  ImGui::Combo("Op##derive", &m_op, ops, 2);
  ImGui::SliderFloat("Strength##derive", &m_strength, 0.0f, 32.0f);
  ImGui::PopItemWidth();
}

nlohmann::json DeriveNode::saveParams() const {
  return {{"op", m_op}, {"strength", m_strength}};
}

void DeriveNode::loadParams(const nlohmann::json& j) {
  if (j.contains("op"))
    m_op = j["op"];
  if (j.contains("strength"))
    m_strength = j["strength"];
}

// ============================================================
// BlurNode
// ============================================================

BlurNode::BlurNode() : m_sizex(0.05f), m_sizey(0.05f), m_order(1), m_mode(0) {}

std::vector<ImNodes::Ez::SlotInfo> BlurNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> BlurNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> BlurNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> BlurNode::outputSlotNames() const {
  return {"Out"};
}

void BlurNode::execute(const std::vector<GenTexture*>& inputs,
                       std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[0].Blur(*in, m_sizex, m_sizey, m_order, m_mode);
}

void BlurNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::SliderFloat("SizeX##blur", &m_sizex, 0.0f, 0.5f);
  ImGui::SliderFloat("SizeY##blur", &m_sizey, 0.0f, 0.5f);
  ImGui::SliderInt("Order##blur", &m_order, 1, 8);
  ImGui::SliderInt("Mode##blur", &m_mode, 0, 3);
  ImGui::PopItemWidth();
}

nlohmann::json BlurNode::saveParams() const {
  return {{"sizex", m_sizex},
          {"sizey", m_sizey},
          {"order", m_order},
          {"mode", m_mode}};
}

void BlurNode::loadParams(const nlohmann::json& j) {
  if (j.contains("sizex"))
    m_sizex = j["sizex"];
  if (j.contains("sizey"))
    m_sizey = j["sizey"];
  if (j.contains("order"))
    m_order = j["order"];
  if (j.contains("mode"))
    m_mode = j["mode"];
}

// ============================================================
// TernaryNode
// ============================================================

TernaryNode::TernaryNode() : m_op(0) {}

std::vector<ImNodes::Ez::SlotInfo> TernaryNode::inputSlotInfos() const {
  return {{"Image1", 1}, {"Image2", 1}, {"Mask", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> TernaryNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> TernaryNode::inputSlotNames() const {
  return {"Image1", "Image2", "Mask"};
}
std::vector<std::string> TernaryNode::outputSlotNames() const {
  return {"Out"};
}

void TernaryNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  GenTexture* in1 = ensureInput(inputs[0]);
  GenTexture* in2 = ensureInput(inputs[1]);
  GenTexture* mask = ensureInput(inputs[2]);
  outputs.resize(1);
  outputs[0].Init(in1->XRes, in1->YRes);
  outputs[0].Ternary(*in1, *in2, *mask, (GenTexture::TernaryOp)m_op);
}

void TernaryNode::renderParams() {
  ImGui::PushItemWidth(120);
  static const char* ops[] = {"Lerp", "Select"};
  ImGui::Combo("Op##tern", &m_op, ops, 2);
  ImGui::PopItemWidth();
}

nlohmann::json TernaryNode::saveParams() const {
  return {{"op", m_op}};
}

void TernaryNode::loadParams(const nlohmann::json& j) {
  if (j.contains("op"))
    m_op = j["op"];
}

// ============================================================
// PasteNode
// ============================================================

PasteNode::PasteNode()
    : m_orgx(0.5f),
      m_orgy(0.5f),
      m_ux(0.25f),
      m_uy(0.0f),
      m_vx(0.0f),
      m_vy(0.25f),
      m_op(0),
      m_mode(0) {}

std::vector<ImNodes::Ez::SlotInfo> PasteNode::inputSlotInfos() const {
  return {{"Background", 1}, {"Snippet", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> PasteNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> PasteNode::inputSlotNames() const {
  return {"Background", "Snippet"};
}
std::vector<std::string> PasteNode::outputSlotNames() const {
  return {"Out"};
}

void PasteNode::execute(const std::vector<GenTexture*>& inputs,
                        std::vector<GenTexture>& outputs) {
  GenTexture* bg = ensureInput(inputs[0]);
  GenTexture* snippet = ensureInput(inputs[1]);
  outputs.resize(1);
  outputs[0].Init(bg->XRes, bg->YRes);
  outputs[0].Paste(*bg, *snippet, m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy,
                   (GenTexture::CombineOp)m_op, m_mode);
}

void PasteNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::SliderFloat("OrgX##paste", &m_orgx, 0.0f, 1.0f);
  ImGui::SliderFloat("OrgY##paste", &m_orgy, 0.0f, 1.0f);
  ImGui::SliderFloat("UX##paste", &m_ux, -1.0f, 1.0f);
  ImGui::SliderFloat("UY##paste", &m_uy, -1.0f, 1.0f);
  ImGui::SliderFloat("VX##paste", &m_vx, -1.0f, 1.0f);
  ImGui::SliderFloat("VY##paste", &m_vy, -1.0f, 1.0f);
  static const char* ops[] = {"Add",      "Sub",      "MulC",     "Min",
                              "Max",      "SetAlpha", "PreAlpha", "Over",
                              "Multiply", "Screen",   "Darken",   "Lighten"};
  ImGui::Combo("Op##paste", &m_op, ops, 12);
  ImGui::SliderInt("Mode##paste", &m_mode, 0, 3);
  ImGui::PopItemWidth();
}

nlohmann::json PasteNode::saveParams() const {
  return {{"orgx", m_orgx}, {"orgy", m_orgy}, {"ux", m_ux}, {"uy", m_uy},
          {"vx", m_vx},     {"vy", m_vy},     {"op", m_op}, {"mode", m_mode}};
}

void PasteNode::loadParams(const nlohmann::json& j) {
  if (j.contains("orgx"))
    m_orgx = j["orgx"];
  if (j.contains("orgy"))
    m_orgy = j["orgy"];
  if (j.contains("ux"))
    m_ux = j["ux"];
  if (j.contains("uy"))
    m_uy = j["uy"];
  if (j.contains("vx"))
    m_vx = j["vx"];
  if (j.contains("vy"))
    m_vy = j["vy"];
  if (j.contains("op"))
    m_op = j["op"];
  if (j.contains("mode"))
    m_mode = j["mode"];
}

// ============================================================
// BumpNode
// ============================================================

BumpNode::BumpNode()
    : m_px(0.5f),
      m_py(0.5f),
      m_pz(1.0f),
      m_dx(0.0f),
      m_dy(0.0f),
      m_dz(-1.0f),
      m_directional(false) {
  m_ambient[0] = 0.1f;
  m_ambient[1] = 0.1f;
  m_ambient[2] = 0.1f;
  m_ambient[3] = 1.0f;
  m_diffuse[0] = 0.8f;
  m_diffuse[1] = 0.8f;
  m_diffuse[2] = 0.8f;
  m_diffuse[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> BumpNode::inputSlotInfos() const {
  return {{"Surface", 1}, {"Normals", 1}, {"Specular", 1}, {"Falloff", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> BumpNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> BumpNode::inputSlotNames() const {
  return {"Surface", "Normals", "Specular", "Falloff"};
}
std::vector<std::string> BumpNode::outputSlotNames() const {
  return {"Out"};
}

void BumpNode::execute(const std::vector<GenTexture*>& inputs,
                       std::vector<GenTexture>& outputs) {
  GenTexture* surface = ensureInput(inputs[0]);
  GenTexture* normals = ensureInput(inputs[1]);
  GenTexture* specular = inputs[2];  // optional
  GenTexture* falloff = inputs[3];   // optional

  outputs.resize(1);
  outputs[0].Init(surface->XRes, surface->YRes);

  Pixel ambPx, difPx;
  ambPx.Init(colorToU32(m_ambient));
  difPx.Init(colorToU32(m_diffuse));

  outputs[0].Bump(*surface, *normals, specular, falloff, m_px, m_py, m_pz, m_dx,
                  m_dy, m_dz, ambPx, difPx, m_directional ? 1 : 0);
}

void BumpNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Text("Light Position:");
  ImGui::SliderFloat("PX##bump", &m_px, -2.0f, 2.0f);
  ImGui::SliderFloat("PY##bump", &m_py, -2.0f, 2.0f);
  ImGui::SliderFloat("PZ##bump", &m_pz, -2.0f, 2.0f);
  ImGui::Text("Light Direction:");
  ImGui::SliderFloat("DX##bump", &m_dx, -1.0f, 1.0f);
  ImGui::SliderFloat("DY##bump", &m_dy, -1.0f, 1.0f);
  ImGui::SliderFloat("DZ##bump", &m_dz, -1.0f, 1.0f);
  ImGui::ColorEdit4("Ambient##bump", m_ambient, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Diffuse##bump", m_diffuse, ImGuiColorEditFlags_NoInputs);
  ImGui::Checkbox("Directional##bump", &m_directional);
  ImGui::PopItemWidth();
}

nlohmann::json BumpNode::saveParams() const {
  return {{"px", m_px},
          {"py", m_py},
          {"pz", m_pz},
          {"dx", m_dx},
          {"dy", m_dy},
          {"dz", m_dz},
          {"ambient", {m_ambient[0], m_ambient[1], m_ambient[2], m_ambient[3]}},
          {"diffuse", {m_diffuse[0], m_diffuse[1], m_diffuse[2], m_diffuse[3]}},
          {"directional", m_directional}};
}

void BumpNode::loadParams(const nlohmann::json& j) {
  if (j.contains("px"))
    m_px = j["px"];
  if (j.contains("py"))
    m_py = j["py"];
  if (j.contains("pz"))
    m_pz = j["pz"];
  if (j.contains("dx"))
    m_dx = j["dx"];
  if (j.contains("dy"))
    m_dy = j["dy"];
  if (j.contains("dz"))
    m_dz = j["dz"];
  if (j.contains("ambient"))
    for (int i = 0; i < 4; i++)
      m_ambient[i] = j["ambient"][i];
  if (j.contains("diffuse"))
    for (int i = 0; i < 4; i++)
      m_diffuse[i] = j["diffuse"][i];
  if (j.contains("directional"))
    m_directional = j["directional"];
}

// ============================================================
// LinearCombineNode
// ============================================================

LinearCombineNode::LinearCombineNode() : m_constWeight(0.0f) {
  m_constColor[0] = 0.0f;
  m_constColor[1] = 0.0f;
  m_constColor[2] = 0.0f;
  m_constColor[3] = 1.0f;
  for (int i = 0; i < 4; i++) {
    m_weights[i] = i == 0 ? 1.0f : 0.0f;
    m_uShift[i] = 0.0f;
    m_vShift[i] = 0.0f;
    m_filterMode[i] = 0;
  }
}

std::vector<ImNodes::Ez::SlotInfo> LinearCombineNode::inputSlotInfos() const {
  return {{"Image1", 1}, {"Image2", 1}, {"Image3", 1}, {"Image4", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> LinearCombineNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> LinearCombineNode::inputSlotNames() const {
  return {"Image1", "Image2", "Image3", "Image4"};
}
std::vector<std::string> LinearCombineNode::outputSlotNames() const {
  return {"Out"};
}

void LinearCombineNode::execute(const std::vector<GenTexture*>& inputs,
                                std::vector<GenTexture>& outputs) {
  // find first non-null input to get size
  int w = 256, h = 256;
  for (int i = 0; i < 4; i++) {
    if (inputs[i] && inputs[i]->Data) {
      w = inputs[i]->XRes;
      h = inputs[i]->YRes;
      break;
    }
  }
  outputs.resize(1);
  outputs[0].Init(w, h);

  // Build LinearInput array for connected inputs
  std::vector<LinearInput> linInputs;
  for (int i = 0; i < 4; i++) {
    if (inputs[i] && inputs[i]->Data) {
      LinearInput li;
      li.Tex = inputs[i];
      li.Weight = m_weights[i];
      li.UShift = m_uShift[i];
      li.VShift = m_vShift[i];
      li.FilterMode = m_filterMode[i];
      linInputs.push_back(li);
    }
  }

  Pixel constPx;
  constPx.Init(colorToU32(m_constColor));
  outputs[0].LinearCombine(constPx, m_constWeight, linInputs.data(),
                           (sInt)linInputs.size());
}

void LinearCombineNode::renderParams() {
  ImGui::PushItemWidth(100);
  ImGui::ColorEdit4("ConstColor##lc", m_constColor,
                    ImGuiColorEditFlags_NoInputs);
  ImGui::SliderFloat("ConstW##lc", &m_constWeight, 0.0f, 1.0f);
  for (int i = 0; i < 4; i++) {
    ImGui::PushID(i);
    ImGui::Text("Input %d:", i + 1);
    ImGui::SliderFloat("W##lc", &m_weights[i], 0.0f, 1.0f);
    ImGui::SliderFloat("US##lc", &m_uShift[i], -1.0f, 1.0f);
    ImGui::SliderFloat("VS##lc", &m_vShift[i], -1.0f, 1.0f);
    static const char* fmodes[] = {"WrapNearest", "ClampNearest",
                                   "WrapBilinear", "ClampBilinear"};
    ImGui::Combo("F##lc", &m_filterMode[i], fmodes, 4);
    ImGui::PopID();
  }
  ImGui::PopItemWidth();
}

nlohmann::json LinearCombineNode::saveParams() const {
  nlohmann::json j;
  j["constColor"] = {m_constColor[0], m_constColor[1], m_constColor[2],
                     m_constColor[3]};
  j["constWeight"] = m_constWeight;
  for (int i = 0; i < 4; i++) {
    j["weights"][i] = m_weights[i];
    j["uShift"][i] = m_uShift[i];
    j["vShift"][i] = m_vShift[i];
    j["filterMode"][i] = m_filterMode[i];
  }
  return j;
}

void LinearCombineNode::loadParams(const nlohmann::json& j) {
  if (j.contains("constColor"))
    for (int i = 0; i < 4; i++)
      m_constColor[i] = j["constColor"][i];
  if (j.contains("constWeight"))
    m_constWeight = j["constWeight"];
  if (j.contains("weights"))
    for (int i = 0; i < 4; i++)
      m_weights[i] = j["weights"][i];
  if (j.contains("uShift"))
    for (int i = 0; i < 4; i++)
      m_uShift[i] = j["uShift"][i];
  if (j.contains("vShift"))
    for (int i = 0; i < 4; i++)
      m_vShift[i] = j["vShift"][i];
  if (j.contains("filterMode"))
    for (int i = 0; i < 4; i++)
      m_filterMode[i] = j["filterMode"][i];
}

// ============================================================
// CrystalNode
// ============================================================

CrystalNode::CrystalNode()
    : m_widthIdx(3), m_heightIdx(3), m_seed(42), m_count(16), m_blendMode(0) {
  m_colorNear[0] = 1.0f;
  m_colorNear[1] = 1.0f;
  m_colorNear[2] = 1.0f;
  m_colorNear[3] = 1.0f;
  m_colorFar[0] = 0.0f;
  m_colorFar[1] = 0.0f;
  m_colorFar[2] = 0.0f;
  m_colorFar[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> CrystalNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> CrystalNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> CrystalNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> CrystalNode::outputSlotNames() const {
  return {"Out"};
}

void CrystalNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  Crystal(outputs[0], (sU32)m_seed, m_count, colorToU32(m_colorNear),
          colorToU32(m_colorFar));
  applyBlend(outputs, inputs, m_blendMode);
}

void CrystalNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##cry", &m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##cry", &m_heightIdx, s_sizesStr);
  ImGui::SliderInt("Seed##cry", &m_seed, 0, 9999);
  ImGui::SliderInt("Count##cry", &m_count, 1, 256);
  ImGui::ColorEdit4("Near##cry", m_colorNear, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Far##cry", m_colorFar, ImGuiColorEditFlags_NoInputs);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##crystal", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json CrystalNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"seed", m_seed},
          {"count", m_count},
          {"colorNear",
           {m_colorNear[0], m_colorNear[1], m_colorNear[2], m_colorNear[3]}},
          {"colorFar",
           {m_colorFar[0], m_colorFar[1], m_colorFar[2], m_colorFar[3]}},
          {"blendMode", m_blendMode}};
}

void CrystalNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("seed"))
    m_seed = j["seed"];
  if (j.contains("count"))
    m_count = j["count"];
  if (j.contains("colorNear"))
    for (int i = 0; i < 4; i++)
      m_colorNear[i] = j["colorNear"][i];
  if (j.contains("colorFar"))
    for (int i = 0; i < 4; i++)
      m_colorFar[i] = j["colorFar"][i];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// DirectionalGradientNode
// ============================================================

DirectionalGradientNode::DirectionalGradientNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.0f),
      m_y1(0.5f),
      m_x2(1.0f),
      m_y2(0.5f),
      m_blendMode(0) {
  m_col1[0] = 0.0f;
  m_col1[1] = 0.0f;
  m_col1[2] = 0.0f;
  m_col1[3] = 1.0f;
  m_col2[0] = 1.0f;
  m_col2[1] = 1.0f;
  m_col2[2] = 1.0f;
  m_col2[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> DirectionalGradientNode::inputSlotInfos()
    const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> DirectionalGradientNode::outputSlotInfos()
    const {
  return {{"Out", 1}};
}
std::vector<std::string> DirectionalGradientNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> DirectionalGradientNode::outputSlotNames() const {
  return {"Out"};
}

void DirectionalGradientNode::execute(const std::vector<GenTexture*>& inputs,
                                      std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  DirectionalGradient(outputs[0], m_x1, m_y1, m_x2, m_y2, colorToU32(m_col1),
                      colorToU32(m_col2));
  applyBlend(outputs, inputs, m_blendMode);
}

void DirectionalGradientNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##dg", &m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##dg", &m_heightIdx, s_sizesStr);
  ImGui::SliderFloat("X1##dg", &m_x1, 0.0f, 1.0f);
  ImGui::SliderFloat("Y1##dg", &m_y1, 0.0f, 1.0f);
  ImGui::SliderFloat("X2##dg", &m_x2, 0.0f, 1.0f);
  ImGui::SliderFloat("Y2##dg", &m_y2, 0.0f, 1.0f);
  ImGui::ColorEdit4("Color1##dg", m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##dg", m_col2, ImGuiColorEditFlags_NoInputs);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##dg", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json DirectionalGradientNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"x1", m_x1},
          {"y1", m_y1},
          {"x2", m_x2},
          {"y2", m_y2},
          {"col1", {m_col1[0], m_col1[1], m_col1[2], m_col1[3]}},
          {"col2", {m_col2[0], m_col2[1], m_col2[2], m_col2[3]}},
          {"blendMode", m_blendMode}};
}

void DirectionalGradientNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("col1"))
    for (int i = 0; i < 4; i++)
      m_col1[i] = j["col1"][i];
  if (j.contains("col2"))
    for (int i = 0; i < 4; i++)
      m_col2[i] = j["col2"][i];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// GlowEffectNode
// ============================================================

GlowEffectNode::GlowEffectNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_cx(0.5f),
      m_cy(0.5f),
      m_scale(1.0f),
      m_exponent(2.0f),
      m_intensity(1.0f),
      m_blendMode(0) {
  m_bgCol[0] = 0.0f;
  m_bgCol[1] = 0.0f;
  m_bgCol[2] = 0.0f;
  m_bgCol[3] = 1.0f;
  m_glowCol[0] = 1.0f;
  m_glowCol[1] = 1.0f;
  m_glowCol[2] = 1.0f;
  m_glowCol[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> GlowEffectNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> GlowEffectNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> GlowEffectNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> GlowEffectNode::outputSlotNames() const {
  return {"Out"};
}

void GlowEffectNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  GlowEffect(outputs[0], m_cx, m_cy, m_scale, m_exponent, m_intensity,
             colorToU32(m_bgCol), colorToU32(m_glowCol));
  applyBlend(outputs, inputs, m_blendMode);
}

void GlowEffectNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##ge", &m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##ge", &m_heightIdx, s_sizesStr);
  ImGui::SliderFloat("CX##ge", &m_cx, 0.0f, 1.0f);
  ImGui::SliderFloat("CY##ge", &m_cy, 0.0f, 1.0f);
  ImGui::SliderFloat("Scale##ge", &m_scale, 0.01f, 5.0f);
  ImGui::SliderFloat("Exponent##ge", &m_exponent, 0.1f, 10.0f);
  ImGui::SliderFloat("Intensity##ge", &m_intensity, 0.0f, 4.0f);
  ImGui::ColorEdit4("BG##ge", m_bgCol, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Glow##ge", m_glowCol, ImGuiColorEditFlags_NoInputs);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##glow", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json GlowEffectNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"cx", m_cx},
          {"cy", m_cy},
          {"scale", m_scale},
          {"exponent", m_exponent},
          {"intensity", m_intensity},
          {"bgCol", {m_bgCol[0], m_bgCol[1], m_bgCol[2], m_bgCol[3]}},
          {"glowCol", {m_glowCol[0], m_glowCol[1], m_glowCol[2], m_glowCol[3]}},
          {"blendMode", m_blendMode}};
}

void GlowEffectNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("cx"))
    m_cx = j["cx"];
  if (j.contains("cy"))
    m_cy = j["cy"];
  if (j.contains("scale"))
    m_scale = j["scale"];
  if (j.contains("exponent"))
    m_exponent = j["exponent"];
  if (j.contains("intensity"))
    m_intensity = j["intensity"];
  if (j.contains("bgCol"))
    for (int i = 0; i < 4; i++)
      m_bgCol[i] = j["bgCol"][i];
  if (j.contains("glowCol"))
    for (int i = 0; i < 4; i++)
      m_glowCol[i] = j["glowCol"][i];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// PerlinNoiseRG2Node
// ============================================================

PerlinNoiseRG2Node::PerlinNoiseRG2Node()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_octaves(4),
      m_freqScale(2),
      m_seed(0),
      m_startOctave(0),
      m_persistence(0.5f),
      m_contrast(1.0f),
      m_blendMode(0) {
  m_col1[0] = 0.0f;
  m_col1[1] = 0.0f;
  m_col1[2] = 0.0f;
  m_col1[3] = 1.0f;
  m_col2[0] = 1.0f;
  m_col2[1] = 1.0f;
  m_col2[2] = 1.0f;
  m_col2[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> PerlinNoiseRG2Node::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> PerlinNoiseRG2Node::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> PerlinNoiseRG2Node::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> PerlinNoiseRG2Node::outputSlotNames() const {
  return {"Out"};
}

void PerlinNoiseRG2Node::execute(const std::vector<GenTexture*>& inputs,
                                 std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  PerlinNoiseRG2(outputs[0], m_octaves, m_persistence, m_freqScale,
                 (sU32)m_seed, m_contrast, colorToU32(m_col1),
                 colorToU32(m_col2), m_startOctave);
  applyBlend(outputs, inputs, m_blendMode);
}

void PerlinNoiseRG2Node::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##pn", &m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##pn", &m_heightIdx, s_sizesStr);
  ImGui::SliderInt("Octaves##pn", &m_octaves, 1, 12);
  ImGui::SliderFloat("Persistence##pn", &m_persistence, 0.0f, 1.0f);
  ImGui::SliderInt("FreqScale##pn", &m_freqScale, 0, 10);
  ImGui::SliderInt("Seed##pn", &m_seed, 0, 9999);
  ImGui::SliderFloat("Contrast##pn", &m_contrast, 0.01f, 4.0f);
  ImGui::SliderInt("StartOct##pn", &m_startOctave, 0, 11);
  ImGui::ColorEdit4("Color1##pn", m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color2##pn", m_col2, ImGuiColorEditFlags_NoInputs);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##perlin", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json PerlinNoiseRG2Node::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"octaves", m_octaves},
          {"freqScale", m_freqScale},
          {"seed", m_seed},
          {"startOctave", m_startOctave},
          {"persistence", m_persistence},
          {"contrast", m_contrast},
          {"col1", {m_col1[0], m_col1[1], m_col1[2], m_col1[3]}},
          {"col2", {m_col2[0], m_col2[1], m_col2[2], m_col2[3]}},
          {"blendMode", m_blendMode}};
}

void PerlinNoiseRG2Node::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("octaves"))
    m_octaves = j["octaves"];
  if (j.contains("freqScale"))
    m_freqScale = j["freqScale"];
  if (j.contains("seed"))
    m_seed = j["seed"];
  if (j.contains("startOctave"))
    m_startOctave = j["startOctave"];
  if (j.contains("persistence"))
    m_persistence = j["persistence"];
  if (j.contains("contrast"))
    m_contrast = j["contrast"];
  if (j.contains("col1"))
    for (int i = 0; i < 4; i++)
      m_col1[i] = j["col1"][i];
  if (j.contains("col2"))
    for (int i = 0; i < 4; i++)
      m_col2[i] = j["col2"][i];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}

// ============================================================
// BlurKernelNode
// ============================================================

BlurKernelNode::BlurKernelNode()
    : m_radiusX(0.05f), m_radiusY(0.05f), m_kernelType(0), m_wrapMode(0) {}

std::vector<ImNodes::Ez::SlotInfo> BlurKernelNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> BlurKernelNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> BlurKernelNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> BlurKernelNode::outputSlotNames() const {
  return {"Out"};
}

void BlurKernelNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  BlurKernel(outputs[0], *in, m_radiusX, m_radiusY, m_kernelType, m_wrapMode);
}

void BlurKernelNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::SliderFloat("RadiusX##bk", &m_radiusX, 0.0f, 0.5f);
  ImGui::SliderFloat("RadiusY##bk", &m_radiusY, 0.0f, 0.5f);
  static const char* ktypes[] = {"Box", "Triangle", "Gaussian"};
  ImGui::Combo("Kernel##bk", &m_kernelType, ktypes, 3);
  static const char* wmodes[] = {"WrapUV", "ClampU", "ClampV", "ClampUV"};
  ImGui::Combo("Wrap##bk", &m_wrapMode, wmodes, 4);
  ImGui::PopItemWidth();
}

nlohmann::json BlurKernelNode::saveParams() const {
  return {{"radiusX", m_radiusX},
          {"radiusY", m_radiusY},
          {"kernelType", m_kernelType},
          {"wrapMode", m_wrapMode}};
}

void BlurKernelNode::loadParams(const nlohmann::json& j) {
  if (j.contains("radiusX"))
    m_radiusX = j["radiusX"];
  if (j.contains("radiusY"))
    m_radiusY = j["radiusY"];
  if (j.contains("kernelType"))
    m_kernelType = j["kernelType"];
  if (j.contains("wrapMode"))
    m_wrapMode = j["wrapMode"];
}

// ============================================================
// HSCBNode
// ============================================================

HSCBNode::HSCBNode()
    : m_hue(0.0f), m_sat(1.0f), m_contrast(1.0f), m_brightness(1.0f) {}

std::vector<ImNodes::Ez::SlotInfo> HSCBNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> HSCBNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> HSCBNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> HSCBNode::outputSlotNames() const {
  return {"Out"};
}

void HSCBNode::execute(const std::vector<GenTexture*>& inputs,
                       std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  HSCB(outputs[0], *in, m_hue, m_sat, m_contrast, m_brightness);
}

void HSCBNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::SliderFloat("Hue##hscb", &m_hue, 0.0f, 1.0f);
  ImGui::SliderFloat("Sat##hscb", &m_sat, 0.0f, 4.0f);
  ImGui::SliderFloat("Contrast##hscb", &m_contrast, 0.01f, 4.0f);
  ImGui::SliderFloat("Brightness##hscb", &m_brightness, 0.0f, 4.0f);
  ImGui::PopItemWidth();
}

nlohmann::json HSCBNode::saveParams() const {
  return {{"hue", m_hue},
          {"sat", m_sat},
          {"contrast", m_contrast},
          {"brightness", m_brightness}};
}

void HSCBNode::loadParams(const nlohmann::json& j) {
  if (j.contains("hue"))
    m_hue = j["hue"];
  if (j.contains("sat"))
    m_sat = j["sat"];
  if (j.contains("contrast"))
    m_contrast = j["contrast"];
  if (j.contains("brightness"))
    m_brightness = j["brightness"];
}

// ============================================================
// WaveletNode
// ============================================================

WaveletNode::WaveletNode() : m_mode(0), m_count(1) {}

std::vector<ImNodes::Ez::SlotInfo> WaveletNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> WaveletNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> WaveletNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> WaveletNode::outputSlotNames() const {
  return {"Out"};
}

void WaveletNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0] = *in;  // copy
  Wavelet(outputs[0], m_mode, m_count);
}

void WaveletNode::renderParams() {
  ImGui::PushItemWidth(120);
  static const char* modes[] = {"Forward", "Inverse"};
  ImGui::Combo("Mode##wav", &m_mode, modes, 2);
  ImGui::SliderInt("Count##wav", &m_count, 1, 8);
  ImGui::PopItemWidth();
}

nlohmann::json WaveletNode::saveParams() const {
  return {{"mode", m_mode}, {"count", m_count}};
}

void WaveletNode::loadParams(const nlohmann::json& j) {
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("count"))
    m_count = j["count"];
}

// ============================================================
// ColorBalanceNode
// ============================================================

ColorBalanceNode::ColorBalanceNode() {
  for (int i = 0; i < 3; i++) {
    m_shadow[i] = 0.0f;
    m_mid[i] = 0.0f;
    m_highlight[i] = 0.0f;
  }
}

std::vector<ImNodes::Ez::SlotInfo> ColorBalanceNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> ColorBalanceNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> ColorBalanceNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> ColorBalanceNode::outputSlotNames() const {
  return {"Out"};
}

void ColorBalanceNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  ColorBalance(outputs[0], *in, m_shadow[0], m_shadow[1], m_shadow[2], m_mid[0],
               m_mid[1], m_mid[2], m_highlight[0], m_highlight[1],
               m_highlight[2]);
}

void ColorBalanceNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Text("Shadows (R/G/B):");
  ImGui::SliderFloat("SR##cb", &m_shadow[0], -1.0f, 1.0f);
  ImGui::SliderFloat("SG##cb", &m_shadow[1], -1.0f, 1.0f);
  ImGui::SliderFloat("SB##cb", &m_shadow[2], -1.0f, 1.0f);
  ImGui::Text("Midtones (R/G/B):");
  ImGui::SliderFloat("MR##cb", &m_mid[0], -1.0f, 1.0f);
  ImGui::SliderFloat("MG##cb", &m_mid[1], -1.0f, 1.0f);
  ImGui::SliderFloat("MB##cb", &m_mid[2], -1.0f, 1.0f);
  ImGui::Text("Highlights (R/G/B):");
  ImGui::SliderFloat("HR##cb", &m_highlight[0], -1.0f, 1.0f);
  ImGui::SliderFloat("HG##cb", &m_highlight[1], -1.0f, 1.0f);
  ImGui::SliderFloat("HB##cb", &m_highlight[2], -1.0f, 1.0f);
  ImGui::PopItemWidth();
}

nlohmann::json ColorBalanceNode::saveParams() const {
  return {{"shadow", {m_shadow[0], m_shadow[1], m_shadow[2]}},
          {"mid", {m_mid[0], m_mid[1], m_mid[2]}},
          {"highlight", {m_highlight[0], m_highlight[1], m_highlight[2]}}};
}

void ColorBalanceNode::loadParams(const nlohmann::json& j) {
  if (j.contains("shadow"))
    for (int i = 0; i < 3; i++)
      m_shadow[i] = j["shadow"][i];
  if (j.contains("mid"))
    for (int i = 0; i < 3; i++)
      m_mid[i] = j["mid"][i];
  if (j.contains("highlight"))
    for (int i = 0; i < 3; i++)
      m_highlight[i] = j["highlight"][i];
}

// ============================================================
// BricksNode
// ============================================================

BricksNode::BricksNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_fugeX(0.05f),
      m_fugeY(0.05f),
      m_tileX(8),
      m_tileY(4),
      m_seed(0),
      m_heads(128),
      m_colorBalance(1.0f),
      m_blendMode(0) {
  m_col0[0] = 0.6f;
  m_col0[1] = 0.3f;
  m_col0[2] = 0.1f;
  m_col0[3] = 1.0f;
  m_col1[0] = 0.8f;
  m_col1[1] = 0.5f;
  m_col1[2] = 0.2f;
  m_col1[3] = 1.0f;
  m_colFuge[0] = 0.9f;
  m_colFuge[1] = 0.9f;
  m_colFuge[2] = 0.85f;
  m_colFuge[3] = 1.0f;
}

std::vector<ImNodes::Ez::SlotInfo> BricksNode::inputSlotInfos() const {
  return {{"In", 1}};
}
std::vector<ImNodes::Ez::SlotInfo> BricksNode::outputSlotInfos() const {
  return {{"Out", 1}};
}
std::vector<std::string> BricksNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> BricksNode::outputSlotNames() const {
  return {"Out"};
}

void BricksNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  Bricks(outputs[0], colorToU32(m_col0), colorToU32(m_col1),
         colorToU32(m_colFuge), m_fugeX, m_fugeY, m_tileX, m_tileY,
         (sU32)m_seed, m_heads, m_colorBalance);
  applyBlend(outputs, inputs, m_blendMode);
}

void BricksNode::renderParams() {
  ImGui::PushItemWidth(120);
  ImGui::Combo("Width##bricks", &m_widthIdx, s_sizesStr);
  ImGui::Combo("Height##bricks", &m_heightIdx, s_sizesStr);
  ImGui::ColorEdit4("Color0##bricks", m_col0, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Color1##bricks", m_col1, ImGuiColorEditFlags_NoInputs);
  ImGui::ColorEdit4("Fuge##bricks", m_colFuge, ImGuiColorEditFlags_NoInputs);
  ImGui::SliderFloat("FugeX##bricks", &m_fugeX, 0.0f, 0.5f);
  ImGui::SliderFloat("FugeY##bricks", &m_fugeY, 0.0f, 0.5f);
  ImGui::SliderInt("TileX##bricks", &m_tileX, 1, 32);
  ImGui::SliderInt("TileY##bricks", &m_tileY, 1, 32);
  ImGui::SliderInt("Seed##bricks", &m_seed, 0, 9999);
  ImGui::SliderInt("Heads##bricks", &m_heads, 0, 255);
  ImGui::SliderFloat("ColorBalance##bricks", &m_colorBalance, 0.01f, 4.0f);
  static const char* blendModes =
      "Replace\0Add\0Sub\0MulC\0Min\0Max\0Over\0Multiply\0Screen\0Darken\0Light"
      "en\0";
  ImGui::Combo("Blend##bricks", &m_blendMode, blendModes);
  ImGui::PopItemWidth();
}

nlohmann::json BricksNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"col0", {m_col0[0], m_col0[1], m_col0[2], m_col0[3]}},
          {"col1", {m_col1[0], m_col1[1], m_col1[2], m_col1[3]}},
          {"colFuge", {m_colFuge[0], m_colFuge[1], m_colFuge[2], m_colFuge[3]}},
          {"fugeX", m_fugeX},
          {"fugeY", m_fugeY},
          {"tileX", m_tileX},
          {"tileY", m_tileY},
          {"seed", m_seed},
          {"heads", m_heads},
          {"colorBalance", m_colorBalance},
          {"blendMode", m_blendMode}};
}

void BricksNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("col0"))
    for (int i = 0; i < 4; i++)
      m_col0[i] = j["col0"][i];
  if (j.contains("col1"))
    for (int i = 0; i < 4; i++)
      m_col1[i] = j["col1"][i];
  if (j.contains("colFuge"))
    for (int i = 0; i < 4; i++)
      m_colFuge[i] = j["colFuge"][i];
  if (j.contains("fugeX"))
    m_fugeX = j["fugeX"];
  if (j.contains("fugeY"))
    m_fugeY = j["fugeY"];
  if (j.contains("tileX"))
    m_tileX = j["tileX"];
  if (j.contains("tileY"))
    m_tileY = j["tileY"];
  if (j.contains("seed"))
    m_seed = j["seed"];
  if (j.contains("heads"))
    m_heads = j["heads"];
  if (j.contains("colorBalance"))
    m_colorBalance = j["colorBalance"];
  if (j.contains("blendMode"))
    m_blendMode = j["blendMode"];
}
