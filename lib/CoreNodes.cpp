#include "CoreNodes.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include "texgen_utils.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ============================================================
// Helpers
// ============================================================






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

// Map combo index to actual GenTexture::FilterMode flags.
// Combo: 0=WrapNearest, 1=ClampNearest, 2=WrapBilinear, 3=ClampBilinear
// Flags: WrapU=0, ClampU=1, WrapV=0, ClampV=2, FilterNearest=0,
// FilterBilinear=4
static int filterModeFlag(int comboIdx) {
  static const int table[] = {0, 3, 4, 7};
  if (comboIdx < 0 || comboIdx > 3)
    return 0;
  return table[comboIdx];
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
  if (!inputs[0]->SizeMatchesWith(outputs[0]))
    return;
  GenTexture blended;
  blended.Init(outputs[0].XRes, outputs[0].YRes);
  blended.Paste(*inputs[0], outputs[0], 0, 0, 1, 0, 0, 1, ops[idx], 0);
  outputs[0] = blended;
}

// ============================================================
// ColorCoreNode
// ============================================================

ColorCoreNode::ColorCoreNode() : m_widthIdx(3), m_heightIdx(3) {
  m_color[0] = 0.0f;
  m_color[1] = 0.0f;
  m_color[2] = 0.0f;
  m_color[3] = 1.0f;
}

std::vector<std::string> ColorCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> ColorCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ColorCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                        std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  sU32 col = colorToU32(m_color);
  for (int i = 0; i < outputs[0].NPixels; i++)
    outputs[0].Data[i].Init(col);
}

nlohmann::json ColorCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx}, {"heightIdx", m_heightIdx},
          {"r", m_color[0]},        {"g", m_color[1]},
          {"b", m_color[2]},        {"a", m_color[3]}};
}

void ColorCoreNode::loadParams(const nlohmann::json& j) {
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
// OutputCoreNode
// ============================================================

OutputCoreNode::OutputCoreNode() {
  strncpy(m_filename, "output.tga", sizeof(m_filename));
}

std::vector<std::string> OutputCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> OutputCoreNode::outputSlotNames() const {
  return {};
}

void OutputCoreNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  outputs.resize(0);
  if (!inputs.empty() && inputs[0] && inputs[0]->Data) {
    SaveImage(*inputs[0], m_filename);
  }
}

nlohmann::json OutputCoreNode::saveParams() const {
  return {{"filename", m_filename}};
}

void OutputCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("filename")) {
    std::string s = j["filename"];
    strncpy(m_filename, s.c_str(), sizeof(m_filename) - 1);
    m_filename[sizeof(m_filename) - 1] = '\0';
  }
}

// ============================================================
// NoiseCoreNode
// ============================================================

NoiseCoreNode::NoiseCoreNode()
    : m_freqX(4),
      m_freqY(4),
      m_oct(4),
      m_seed(0),
      m_mode(4),
      m_fadeoff(0.5f),
      m_sizeIdx(3) {
  m_col1[0] = 0;
  m_col1[1] = 0;
  m_col1[2] = 0;
  m_col1[3] = 1;
  m_col2[0] = 1;
  m_col2[1] = 1;
  m_col2[2] = 1;
  m_col2[3] = 1;
}

std::vector<std::string> NoiseCoreNode::inputSlotNames() const {
  return {"Gradient"};
}

std::vector<std::string> NoiseCoreNode::outputSlotNames() const {
  return {"Out"};
}

void NoiseCoreNode::execute(const std::vector<GenTexture*>& inputs,
                        std::vector<GenTexture>& outputs) {
  int sz = indexToSize(m_sizeIdx);
  outputs.resize(1);
  outputs[0].Init(sz, sz);

  // Use connected gradient if available, otherwise generate from c1/c2
  GenTexture defaultGrad;
  GenTexture* grad;
  if (inputs.size() > 0 && inputs[0] && inputs[0]->Data) {
    grad = inputs[0];
  } else {
    float c1[4] = {m_col1[0], m_col1[1], m_col1[2], 1.0f};
    float c2[4] = {m_col2[0], m_col2[1], m_col2[2], 1.0f};
    defaultGrad = LinearGradient(colorToU32(c1), colorToU32(c2));
    grad = &defaultGrad;
  }

  outputs[0].Noise(*grad, m_freqX, m_freqY, m_oct, m_fadeoff, m_seed, m_mode);
}

nlohmann::json NoiseCoreNode::saveParams() const {
  return {{"sizeIdx", m_sizeIdx}, {"freqX", m_freqX},     {"freqY", m_freqY},
          {"oct", m_oct},         {"fadeoff", m_fadeoff}, {"seed", m_seed},
          {"mode", m_mode},       {"c1r", m_col1[0]},     {"c1g", m_col1[1]},
          {"c1b", m_col1[2]},     {"c1a", m_col1[3]},     {"c2r", m_col2[0]},
          {"c2g", m_col2[1]},     {"c2b", m_col2[2]},     {"c2a", m_col2[3]}};
}

void NoiseCoreNode::loadParams(const nlohmann::json& j) {
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
}

// ============================================================
// CellsCoreNode
// ============================================================

CellsCoreNode::CellsCoreNode()
    : m_nCenters(16),
      m_seed(42),
      m_mode(0),
      m_amp(1.0f),
      m_sizeIdx(3),
      m_colorMode(0) {
  m_col1[0] = 0;
  m_col1[1] = 0;
  m_col1[2] = 0;
  m_col1[3] = 1;
  m_col2[0] = 1;
  m_col2[1] = 1;
  m_col2[2] = 1;
  m_col2[3] = 1;
}

std::vector<std::string> CellsCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> CellsCoreNode::outputSlotNames() const {
  return {"Out"};
}

void CellsCoreNode::execute(const std::vector<GenTexture*>& inputs,
                        std::vector<GenTexture>& outputs) {
  int sz = indexToSize(m_sizeIdx);
  outputs.resize(1);
  outputs[0].Init(sz, sz);

  float gc1[4] = {m_col1[0], m_col1[1], m_col1[2], 1.0f};
  float gc2[4] = {m_col2[0], m_col2[1], m_col2[2], 1.0f};
  GenTexture grad = LinearGradient(colorToU32(gc1), colorToU32(gc2));

  // Auto-generate CellCenter array from seed+count using LCG
  std::vector<CellCenter> centers(m_nCenters);
  sU32 rng = (sU32)m_seed;
  rng = rng * 0x8af6f2ac + 0x1757286;
  for (int i = 0; i < m_nCenters; i++) {
    rng = rng * 0x15a4e35 + 1;
    centers[i].x = (float)((rng >> 8) & 0xffff) / 65536.0f;
    rng = rng * 0x15a4e35 + 1;
    centers[i].y = (float)((rng >> 8) & 0xffff) / 65536.0f;

    if (m_colorMode == 1) {
      // Random: each cell gets a random color from the seed
      rng = rng * 0x15a4e35 + 1;
      sU32 r = (rng >> 8) & 0xff;
      rng = rng * 0x15a4e35 + 1;
      sU32 g = (rng >> 8) & 0xff;
      rng = rng * 0x15a4e35 + 1;
      sU32 b = (rng >> 8) & 0xff;
      rng = rng * 0x15a4e35 + 1;
      sU32 a = (rng >> 8) & 0xff;
      centers[i].color.Init((sU8)r, (sU8)g, (sU8)b, (sU8)a);
    } else {
      // Gradient: sample color from gradient at cell X position
      sInt gx = (sInt)(centers[i].x * (1 << 24));
      grad.SampleGradient(centers[i].color, gx);
    }
  }

  outputs[0].Cells(grad, centers.data(), m_nCenters, m_amp, m_mode);
}

nlohmann::json CellsCoreNode::saveParams() const {
  return {{"sizeIdx", m_sizeIdx}, {"nCenters", m_nCenters},  {"seed", m_seed},
          {"amp", m_amp},         {"mode", m_mode},          {"c1r", m_col1[0]},
          {"c1g", m_col1[1]},     {"c1b", m_col1[2]},        {"c1a", m_col1[3]},
          {"c2r", m_col2[0]},     {"c2g", m_col2[1]},        {"c2b", m_col2[2]},
          {"c2a", m_col2[3]},     {"colorMode", m_colorMode}};
}

void CellsCoreNode::loadParams(const nlohmann::json& j) {
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
  if (j.contains("colorMode"))
    m_colorMode = j["colorMode"];
}

// ============================================================
// GlowRectCoreNode
// ============================================================

GlowRectCoreNode::GlowRectCoreNode()
    : m_orgx(0.5f),
      m_orgy(0.5f),
      m_ux(0.3f),
      m_uy(0.0f),
      m_vx(0.0f),
      m_vy(0.3f),
      m_rectu(0.8f),
      m_rectv(0.8f) {}

std::vector<std::string> GlowRectCoreNode::inputSlotNames() const {
  return {"Background", "Gradient"};
}

std::vector<std::string> GlowRectCoreNode::outputSlotNames() const {
  return {"Out"};
}

void GlowRectCoreNode::execute(const std::vector<GenTexture*>& inputs,
                           std::vector<GenTexture>& outputs) {
  GenTexture* bg = ensureInput(inputs[0]);
  GenTexture* grad = ensureGradient(inputs[1]);
  outputs.resize(1);
  outputs[0].Init(bg->XRes, bg->YRes);
  outputs[0].GlowRect(*bg, *grad, m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy,
                      m_rectu, m_rectv);
}

nlohmann::json GlowRectCoreNode::saveParams() const {
  return {{"orgx", m_orgx},   {"orgy", m_orgy},  {"ux", m_ux},
          {"uy", m_uy},       {"vx", m_vx},      {"vy", m_vy},
          {"rectu", m_rectu}, {"rectv", m_rectv}};
}

void GlowRectCoreNode::loadParams(const nlohmann::json& j) {
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
// ColorMatrixCoreNode
// ============================================================

ColorMatrixCoreNode::ColorMatrixCoreNode() : m_clampPremult(false) {
  // Identity matrix
  for (int i = 0; i < 16; i++)
    m_matrix[i] = 0.0f;
  m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0f;
}

std::vector<std::string> ColorMatrixCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> ColorMatrixCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ColorMatrixCoreNode::execute(const std::vector<GenTexture*>& inputs,
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

nlohmann::json ColorMatrixCoreNode::saveParams() const {
  nlohmann::json j;
  for (int i = 0; i < 16; i++)
    j["matrix"][i] = m_matrix[i];
  j["clampPremult"] = m_clampPremult;
  return j;
}

void ColorMatrixCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("matrix"))
    for (int i = 0; i < 16; i++)
      m_matrix[i] = j["matrix"][i];
  if (j.contains("clampPremult"))
    m_clampPremult = j["clampPremult"];
}

// ============================================================
// CoordMatrixCoreNode
// ============================================================

CoordMatrixCoreNode::CoordMatrixCoreNode() : m_filterMode(0) {
  for (int i = 0; i < 16; i++)
    m_matrix[i] = 0.0f;
  m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0f;
}

std::vector<std::string> CoordMatrixCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> CoordMatrixCoreNode::outputSlotNames() const {
  return {"Out"};
}

void CoordMatrixCoreNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  Matrix44 mat;
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 4; c++)
      mat[r][c] = m_matrix[r * 4 + c];
  outputs[0].CoordMatrixTransform(*in, mat, filterModeFlag(m_filterMode));
}

nlohmann::json CoordMatrixCoreNode::saveParams() const {
  nlohmann::json j;
  for (int i = 0; i < 16; i++)
    j["matrix"][i] = m_matrix[i];
  j["filterMode"] = m_filterMode;
  return j;
}

void CoordMatrixCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("matrix"))
    for (int i = 0; i < 16; i++)
      m_matrix[i] = j["matrix"][i];
  if (j.contains("filterMode"))
    m_filterMode = j["filterMode"];
}

// ============================================================
// ColorRemapCoreNode
// ============================================================

ColorRemapCoreNode::ColorRemapCoreNode() {}

std::vector<std::string> ColorRemapCoreNode::inputSlotNames() const {
  return {"Input", "MapR", "MapG", "MapB"};
}

std::vector<std::string> ColorRemapCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ColorRemapCoreNode::execute(const std::vector<GenTexture*>& inputs,
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
// CoordRemapCoreNode
// ============================================================

CoordRemapCoreNode::CoordRemapCoreNode()
    : m_strengthU(1.0f), m_strengthV(1.0f), m_filterMode(0) {}

std::vector<std::string> CoordRemapCoreNode::inputSlotNames() const {
  return {"Input", "Remap"};
}

std::vector<std::string> CoordRemapCoreNode::outputSlotNames() const {
  return {"Out"};
}

void CoordRemapCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  GenTexture* remap = ensureInput(inputs[1]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[0].CoordRemap(*in, *remap, m_strengthU, m_strengthV,
                        filterModeFlag(m_filterMode));
}

nlohmann::json CoordRemapCoreNode::saveParams() const {
  return {{"strengthU", m_strengthU},
          {"strengthV", m_strengthV},
          {"filterMode", m_filterMode}};
}

void CoordRemapCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("strengthU"))
    m_strengthU = j["strengthU"];
  if (j.contains("strengthV"))
    m_strengthV = j["strengthV"];
  if (j.contains("filterMode"))
    m_filterMode = j["filterMode"];
}

// ============================================================
// DeriveCoreNode
// ============================================================

DeriveCoreNode::DeriveCoreNode() : m_op(0), m_strength(4.0f) {}

std::vector<std::string> DeriveCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> DeriveCoreNode::outputSlotNames() const {
  return {"Out"};
}

void DeriveCoreNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[0].Derive(*in, (GenTexture::DeriveOp)m_op, m_strength);
}

nlohmann::json DeriveCoreNode::saveParams() const {
  return {{"op", m_op}, {"strength", m_strength}};
}

void DeriveCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("op"))
    m_op = j["op"];
  if (j.contains("strength"))
    m_strength = j["strength"];
}

// ============================================================
// BlurCoreNode
// ============================================================

BlurCoreNode::BlurCoreNode() : m_sizex(0.05f), m_sizey(0.05f), m_order(1), m_mode(0) {}

std::vector<std::string> BlurCoreNode::inputSlotNames() const {
  return {"In", "Sigma"};
}

std::vector<std::string> BlurCoreNode::outputSlotNames() const {
  return {"Out"};
}

void BlurCoreNode::execute(const std::vector<GenTexture*>& inputs,
                       std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  // The Sigma map scales the blur size by its average value — a cheap
  // stand-in for MM's per-pixel sigma modulation.
  float scale = 1.0f;
  if (inputs.size() > 1 && inputs[1] && inputs[1]->Data) {
    const GenTexture* m = inputs[1];
    double acc = 0.0;
    for (sInt i = 0; i < m->NPixels; i++) {
      const Pixel& p = m->Data[i];
      acc += ((double)p.r + p.g + p.b) / (3.0 * 65535.0);
    }
    scale = (float)(acc / m->NPixels);
  }
  outputs[0].Blur(*in, m_sizex * scale, m_sizey * scale, m_order, m_mode);
}

nlohmann::json BlurCoreNode::saveParams() const {
  return {{"sizex", m_sizex},
          {"sizey", m_sizey},
          {"order", m_order},
          {"mode", m_mode}};
}

void BlurCoreNode::loadParams(const nlohmann::json& j) {
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
// TernaryCoreNode
// ============================================================

TernaryCoreNode::TernaryCoreNode() : m_op(0) {}

std::vector<std::string> TernaryCoreNode::inputSlotNames() const {
  return {"Image1", "Image2", "Mask"};
}

std::vector<std::string> TernaryCoreNode::outputSlotNames() const {
  return {"Out"};
}

void TernaryCoreNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  GenTexture* in1 = ensureInput(inputs[0]);
  GenTexture* in2 = ensureInput(inputs[1]);
  GenTexture* mask = ensureInput(inputs[2]);
  outputs.resize(1);
  outputs[0].Init(in1->XRes, in1->YRes);
  outputs[0].Ternary(*in1, *in2, *mask, (GenTexture::TernaryOp)m_op);
}

nlohmann::json TernaryCoreNode::saveParams() const {
  return {{"op", m_op}};
}

void TernaryCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("op"))
    m_op = j["op"];
}

// ============================================================
// PasteCoreNode
// ============================================================

PasteCoreNode::PasteCoreNode()
    : m_orgx(0.5f),
      m_orgy(0.5f),
      m_ux(0.25f),
      m_uy(0.0f),
      m_vx(0.0f),
      m_vy(0.25f),
      m_op(0),
      m_mode(0) {}

std::vector<std::string> PasteCoreNode::inputSlotNames() const {
  return {"Background", "Snippet"};
}

std::vector<std::string> PasteCoreNode::outputSlotNames() const {
  return {"Out"};
}

void PasteCoreNode::execute(const std::vector<GenTexture*>& inputs,
                        std::vector<GenTexture>& outputs) {
  GenTexture* bg = ensureInput(inputs[0]);
  GenTexture* snippet = ensureInput(inputs[1]);
  outputs.resize(1);
  outputs[0].Init(bg->XRes, bg->YRes);
  outputs[0].Paste(*bg, *snippet, m_orgx, m_orgy, m_ux, m_uy, m_vx, m_vy,
                   (GenTexture::CombineOp)m_op, m_mode);
}

nlohmann::json PasteCoreNode::saveParams() const {
  return {{"orgx", m_orgx}, {"orgy", m_orgy}, {"ux", m_ux}, {"uy", m_uy},
          {"vx", m_vx},     {"vy", m_vy},     {"op", m_op}, {"mode", m_mode}};
}

void PasteCoreNode::loadParams(const nlohmann::json& j) {
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
// BumpCoreNode
// ============================================================

BumpCoreNode::BumpCoreNode()
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

std::vector<std::string> BumpCoreNode::inputSlotNames() const {
  return {"Surface", "Normals", "Specular", "Falloff"};
}

std::vector<std::string> BumpCoreNode::outputSlotNames() const {
  return {"Out"};
}

void BumpCoreNode::execute(const std::vector<GenTexture*>& inputs,
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

nlohmann::json BumpCoreNode::saveParams() const {
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

void BumpCoreNode::loadParams(const nlohmann::json& j) {
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
// LinearCombineCoreNode
// ============================================================

LinearCombineCoreNode::LinearCombineCoreNode() : m_constWeight(0.0f) {
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

std::vector<std::string> LinearCombineCoreNode::inputSlotNames() const {
  return {"Image1", "Image2", "Image3", "Image4"};
}

std::vector<std::string> LinearCombineCoreNode::outputSlotNames() const {
  return {"Out"};
}

void LinearCombineCoreNode::execute(const std::vector<GenTexture*>& inputs,
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
      li.FilterMode = filterModeFlag(m_filterMode[i]);
      linInputs.push_back(li);
    }
  }

  Pixel constPx;
  constPx.Init(colorToU32(m_constColor));
  outputs[0].LinearCombine(constPx, m_constWeight, linInputs.data(),
                           (sInt)linInputs.size());
}

nlohmann::json LinearCombineCoreNode::saveParams() const {
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

void LinearCombineCoreNode::loadParams(const nlohmann::json& j) {
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
// CrystalCoreNode
// ============================================================

CrystalCoreNode::CrystalCoreNode()
    : m_widthIdx(3), m_heightIdx(3), m_seed(42), m_count(16) {
  m_colorNear[0] = 1.0f;
  m_colorNear[1] = 1.0f;
  m_colorNear[2] = 1.0f;
  m_colorNear[3] = 1.0f;
  m_colorFar[0] = 0.0f;
  m_colorFar[1] = 0.0f;
  m_colorFar[2] = 0.0f;
  m_colorFar[3] = 1.0f;
}

std::vector<std::string> CrystalCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> CrystalCoreNode::outputSlotNames() const {
  return {"Out"};
}

void CrystalCoreNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  Crystal(outputs[0], (sU32)m_seed, m_count, colorToU32(m_colorNear),
          colorToU32(m_colorFar));
}

nlohmann::json CrystalCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"seed", m_seed},
          {"count", m_count},
          {"colorNear",
           {m_colorNear[0], m_colorNear[1], m_colorNear[2], m_colorNear[3]}},
          {"colorFar",
           {m_colorFar[0], m_colorFar[1], m_colorFar[2], m_colorFar[3]}}};
}

void CrystalCoreNode::loadParams(const nlohmann::json& j) {
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
}

// ============================================================
// DirectionalGradientCoreNode
// ============================================================

DirectionalGradientCoreNode::DirectionalGradientCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_x1(0.0f),
      m_y1(0.5f),
      m_x2(1.0f),
      m_y2(0.5f) {
  m_col1[0] = 0.0f;
  m_col1[1] = 0.0f;
  m_col1[2] = 0.0f;
  m_col1[3] = 1.0f;
  m_col2[0] = 1.0f;
  m_col2[1] = 1.0f;
  m_col2[2] = 1.0f;
  m_col2[3] = 1.0f;
}

std::vector<std::string> DirectionalGradientCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> DirectionalGradientCoreNode::outputSlotNames() const {
  return {"Out"};
}

void DirectionalGradientCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                      std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  DirectionalGradient(outputs[0], m_x1, m_y1, m_x2, m_y2, colorToU32(m_col1),
                      colorToU32(m_col2));
}

nlohmann::json DirectionalGradientCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"x1", m_x1},
          {"y1", m_y1},
          {"x2", m_x2},
          {"y2", m_y2},
          {"col1", {m_col1[0], m_col1[1], m_col1[2], m_col1[3]}},
          {"col2", {m_col2[0], m_col2[1], m_col2[2], m_col2[3]}}};
}

void DirectionalGradientCoreNode::loadParams(const nlohmann::json& j) {
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
}

// ============================================================
// GlowEffectCoreNode
// ============================================================

GlowEffectCoreNode::GlowEffectCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_cx(0.5f),
      m_cy(0.5f),
      m_scale(1.0f),
      m_exponent(2.0f),
      m_intensity(1.0f) {
  m_bgCol[0] = 0.0f;
  m_bgCol[1] = 0.0f;
  m_bgCol[2] = 0.0f;
  m_bgCol[3] = 1.0f;
  m_glowCol[0] = 1.0f;
  m_glowCol[1] = 1.0f;
  m_glowCol[2] = 1.0f;
  m_glowCol[3] = 1.0f;
}

std::vector<std::string> GlowEffectCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> GlowEffectCoreNode::outputSlotNames() const {
  return {"Out"};
}

void GlowEffectCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  GlowEffect(outputs[0], m_cx, m_cy, m_scale, m_exponent, m_intensity,
             colorToU32(m_bgCol), colorToU32(m_glowCol));
}

nlohmann::json GlowEffectCoreNode::saveParams() const {
  return {
      {"widthIdx", m_widthIdx},
      {"heightIdx", m_heightIdx},
      {"cx", m_cx},
      {"cy", m_cy},
      {"scale", m_scale},
      {"exponent", m_exponent},
      {"intensity", m_intensity},
      {"bgCol", {m_bgCol[0], m_bgCol[1], m_bgCol[2], m_bgCol[3]}},
      {"glowCol", {m_glowCol[0], m_glowCol[1], m_glowCol[2], m_glowCol[3]}}};
}

void GlowEffectCoreNode::loadParams(const nlohmann::json& j) {
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
}

// ============================================================
// PerlinNoiseRG2CoreNode
// ============================================================

PerlinNoiseRG2CoreNode::PerlinNoiseRG2CoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_octaves(4),
      m_freqScale(2),
      m_seed(0),
      m_startOctave(0),
      m_persistence(0.5f),
      m_contrast(1.0f) {
  m_col1[0] = 0.0f;
  m_col1[1] = 0.0f;
  m_col1[2] = 0.0f;
  m_col1[3] = 1.0f;
  m_col2[0] = 1.0f;
  m_col2[1] = 1.0f;
  m_col2[2] = 1.0f;
  m_col2[3] = 1.0f;
}

std::vector<std::string> PerlinNoiseRG2CoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> PerlinNoiseRG2CoreNode::outputSlotNames() const {
  return {"Out"};
}

void PerlinNoiseRG2CoreNode::execute(const std::vector<GenTexture*>& inputs,
                                 std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  PerlinNoiseRG2(outputs[0], m_octaves, m_persistence, m_freqScale,
                 (sU32)m_seed, m_contrast, colorToU32(m_col1),
                 colorToU32(m_col2), m_startOctave);
}

nlohmann::json PerlinNoiseRG2CoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"octaves", m_octaves},
          {"freqScale", m_freqScale},
          {"seed", m_seed},
          {"startOctave", m_startOctave},
          {"persistence", m_persistence},
          {"contrast", m_contrast},
          {"col1", {m_col1[0], m_col1[1], m_col1[2], m_col1[3]}},
          {"col2", {m_col2[0], m_col2[1], m_col2[2], m_col2[3]}}};
}

void PerlinNoiseRG2CoreNode::loadParams(const nlohmann::json& j) {
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
}

// ============================================================
// BlurKernelCoreNode
// ============================================================

BlurKernelCoreNode::BlurKernelCoreNode()
    : m_radiusX(0.05f), m_radiusY(0.05f), m_kernelType(0), m_wrapMode(0) {}

std::vector<std::string> BlurKernelCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> BlurKernelCoreNode::outputSlotNames() const {
  return {"Out"};
}

void BlurKernelCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  BlurKernel(outputs[0], *in, m_radiusX, m_radiusY, m_kernelType, m_wrapMode);
}

nlohmann::json BlurKernelCoreNode::saveParams() const {
  return {{"radiusX", m_radiusX},
          {"radiusY", m_radiusY},
          {"kernelType", m_kernelType},
          {"wrapMode", m_wrapMode}};
}

void BlurKernelCoreNode::loadParams(const nlohmann::json& j) {
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
// HSCBCoreNode
// ============================================================

HSCBCoreNode::HSCBCoreNode()
    : m_hue(0.0f), m_sat(1.0f), m_contrast(1.0f), m_brightness(1.0f) {}

std::vector<std::string> HSCBCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> HSCBCoreNode::outputSlotNames() const {
  return {"Out"};
}

void HSCBCoreNode::execute(const std::vector<GenTexture*>& inputs,
                       std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  HSCB(outputs[0], *in, m_hue, m_sat, m_contrast, m_brightness);
}

nlohmann::json HSCBCoreNode::saveParams() const {
  return {{"hue", m_hue},
          {"sat", m_sat},
          {"contrast", m_contrast},
          {"brightness", m_brightness}};
}

void HSCBCoreNode::loadParams(const nlohmann::json& j) {
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
// WaveletCoreNode
// ============================================================

WaveletCoreNode::WaveletCoreNode() : m_mode(0), m_count(1) {}

std::vector<std::string> WaveletCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> WaveletCoreNode::outputSlotNames() const {
  return {"Out"};
}

void WaveletCoreNode::execute(const std::vector<GenTexture*>& inputs,
                          std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0] = *in;  // copy
  Wavelet(outputs[0], m_mode, m_count);
}

nlohmann::json WaveletCoreNode::saveParams() const {
  return {{"mode", m_mode}, {"count", m_count}};
}

void WaveletCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("count"))
    m_count = j["count"];
}

// ============================================================
// ColorBalanceCoreNode
// ============================================================

ColorBalanceCoreNode::ColorBalanceCoreNode() {
  for (int i = 0; i < 3; i++) {
    m_shadow[i] = 0.0f;
    m_mid[i] = 0.0f;
    m_highlight[i] = 0.0f;
  }
}

std::vector<std::string> ColorBalanceCoreNode::inputSlotNames() const {
  return {"In"};
}

std::vector<std::string> ColorBalanceCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ColorBalanceCoreNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  GenTexture* in = ensureInput(inputs[0]);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  ColorBalance(outputs[0], *in, m_shadow[0], m_shadow[1], m_shadow[2], m_mid[0],
               m_mid[1], m_mid[2], m_highlight[0], m_highlight[1],
               m_highlight[2]);
}

nlohmann::json ColorBalanceCoreNode::saveParams() const {
  return {{"shadow", {m_shadow[0], m_shadow[1], m_shadow[2]}},
          {"mid", {m_mid[0], m_mid[1], m_mid[2]}},
          {"highlight", {m_highlight[0], m_highlight[1], m_highlight[2]}}};
}

void ColorBalanceCoreNode::loadParams(const nlohmann::json& j) {
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
// BricksCoreNode
// ============================================================

BricksCoreNode::BricksCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_fugeX(0.05f),
      m_fugeY(0.05f),
      m_tileX(8),
      m_tileY(4),
      m_seed(0),
      m_heads(128),
      m_colorBalance(1.0f) {
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

std::vector<std::string> BricksCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> BricksCoreNode::outputSlotNames() const {
  return {"Out"};
}

void BricksCoreNode::execute(const std::vector<GenTexture*>& inputs,
                         std::vector<GenTexture>& outputs) {
  int w = indexToSize(m_widthIdx);
  int h = indexToSize(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  Bricks(outputs[0], colorToU32(m_col0), colorToU32(m_col1),
         colorToU32(m_colFuge), m_fugeX, m_fugeY, m_tileX, m_tileY,
         (sU32)m_seed, m_heads, m_colorBalance);
}

nlohmann::json BricksCoreNode::saveParams() const {
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
          {"colorBalance", m_colorBalance}};
}

void BricksCoreNode::loadParams(const nlohmann::json& j) {
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
  // Legacy flat-key format (older project files)
  static const char* comps = "rgba";
  for (int i = 0; i < 4; i++) {
    std::string c(1, comps[i]);
    if (j.contains("col0" + c))
      m_col0[i] = j["col0" + c];
    if (j.contains("col1" + c))
      m_col1[i] = j["col1" + c];
    if (j.contains("colFuge" + c))
      m_colFuge[i] = j["colFuge" + c];
  }
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
}

// ============================================================
// GradientCoreNode
// ============================================================

GradientCoreNode::GradientCoreNode() {
  m_col1[0] = 0.0f;
  m_col1[1] = 0.0f;
  m_col1[2] = 0.0f;
  m_col1[3] = 1.0f;
  m_col2[0] = 1.0f;
  m_col2[1] = 1.0f;
  m_col2[2] = 1.0f;
  m_col2[3] = 1.0f;
}

std::vector<std::string> GradientCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> GradientCoreNode::outputSlotNames() const {
  return {"Out"};
}

void GradientCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                           std::vector<GenTexture>& outputs) {
  outputs.resize(1);
  outputs[0] = LinearGradient(colorToU32(m_col1), colorToU32(m_col2));
}

nlohmann::json GradientCoreNode::saveParams() const {
  return {{"c1r", m_col1[0]}, {"c1g", m_col1[1]}, {"c1b", m_col1[2]},
          {"c1a", m_col1[3]}, {"c2r", m_col2[0]}, {"c2g", m_col2[1]},
          {"c2b", m_col2[2]}, {"c2a", m_col2[3]}};
}

void GradientCoreNode::loadParams(const nlohmann::json& j) {
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
}

// ============================================================
// ImageCoreNode
// ============================================================

ImageCoreNode::ImageCoreNode() : m_loaded(false) {
  m_filename[0] = '\0';
}

std::vector<std::string> ImageCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> ImageCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ImageCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                        std::vector<GenTexture>& outputs) {
  outputs.resize(1);

  // Reload if filename changed
  std::string path(m_filename);
  if (path != m_loadedPath) {
    m_loaded = false;
    m_loadedPath.clear();
  }

  if (!m_loaded && !path.empty()) {
    int imgW = 0, imgH = 0, channels = 0;
    // stbi_load: force 4 channels (RGBA), handles TGA/PNG/JPG/BMP/PSD/GIF/HDR
    stbi_set_flip_vertically_on_load(0);
    unsigned char* pixels = stbi_load(m_filename, &imgW, &imgH, &channels, 4);
    if (!pixels) {
      // imported projects carry the author's absolute path; fall back
      // to the basename in the working dir and the Material folder
      size_t slash = path.find_last_of("/\\");
      if (slash != std::string::npos) {
        std::string base = path.substr(slash + 1);
        pixels = stbi_load(base.c_str(), &imgW, &imgH, &channels, 4);
        if (!pixels)
          pixels = stbi_load(("Material/" + base).c_str(), &imgW, &imgH,
                             &channels, 4);
      }
    }
    if (pixels) {
      // Resize to nearest power of 2 (GenTexture requirement)
      int pw = 1;
      while (pw < imgW)
        pw <<= 1;
      int ph = 1;
      while (ph < imgH)
        ph <<= 1;

      bool didResize = (pw != imgW || ph != imgH);
      if (didResize) {
        unsigned char* resized = (unsigned char*)malloc(pw * ph * 4);
        for (int y = 0; y < ph; y++) {
          int srcY = y * imgH / ph;
          for (int x = 0; x < pw; x++) {
            int srcX = x * imgW / pw;
            memcpy(&resized[(y * pw + x) * 4],
                   &pixels[(srcY * imgW + srcX) * 4], 4);
          }
        }
        stbi_image_free(pixels);
        pixels = resized;
        imgW = pw;
        imgH = ph;
      }

      m_cache.Init(imgW, imgH);
      for (int i = 0; i < imgW * imgH; i++) {
        m_cache.Data[i].Init(pixels[i * 4 + 0], pixels[i * 4 + 1],
                             pixels[i * 4 + 2], pixels[i * 4 + 3]);
      }

      if (didResize)
        free(pixels);
      else
        stbi_image_free(pixels);

      m_loaded = true;
      m_loadedPath = path;
    }
  }

  if (m_loaded && m_cache.Data) {
    outputs[0] = m_cache;
  } else {
    outputs[0].Init(256, 256);
  }
}

nlohmann::json ImageCoreNode::saveParams() const {
  return {{"filename", std::string(m_filename)}};
}

void ImageCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("filename")) {
    std::string f = j["filename"];
    strncpy(m_filename, f.c_str(), sizeof(m_filename) - 1);
    m_filename[sizeof(m_filename) - 1] = '\0';
    m_loaded = false;
  }
}

// ============================================================
// CommentCoreNode
// ============================================================

CommentCoreNode::CommentCoreNode() {
  strncpy(m_text, "Note", sizeof(m_text));
  m_color[0] = 0.3f;
  m_color[1] = 0.3f;
  m_color[2] = 0.3f;
}

std::string CommentCoreNode::displayTitle() const {
  // Show first line of text as title
  std::string t(m_text);
  auto nl = t.find('\n');
  if (nl != std::string::npos)
    t = t.substr(0, nl);
  if (t.size() > 30)
    t = t.substr(0, 30) + "...";
  return t;
}

std::vector<std::string> CommentCoreNode::inputSlotNames() const {
  return {};
}

std::vector<std::string> CommentCoreNode::outputSlotNames() const {
  return {};
}

void CommentCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                          std::vector<GenTexture>& /*outputs*/) {
  // No-op: comment nodes don't process anything
}

nlohmann::json CommentCoreNode::saveParams() const {
  return {{"text", std::string(m_text)},
          {"color", {m_color[0], m_color[1], m_color[2]}}};
}

void CommentCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("text")) {
    std::string t = j["text"];
    strncpy(m_text, t.c_str(), sizeof(m_text) - 1);
    m_text[sizeof(m_text) - 1] = '\0';
  }
  if (j.contains("color"))
    for (int i = 0; i < 3; i++)
      m_color[i] = j["color"][i];
}
