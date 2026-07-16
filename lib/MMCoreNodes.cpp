#include "MMCoreNodes.h"

#include <cctype>
#include <cstring>
#include "mm_workflow.h"
#include "texgen_utils.h"

namespace {

int mmSizeFromIdx(int idx) { return 32 << idx; }

sU32 mmColorToU32(const float c[4]) {
  auto cl = [](float v) {
    return (sU32)((v < 0 ? 0 : (v > 1 ? 1 : v)) * 255.0f + 0.5f);
  };
  return (cl(c[3]) << 24) | (cl(c[0]) << 16) | (cl(c[1]) << 8) | cl(c[2]);
}

GenTexture& mmFallback() {
  static GenTexture fallback;
  if (!fallback.Data)
    fallback.Init(256, 256);
  return fallback;
}

GenTexture* mmEnsure(GenTexture* p) {
  if (!p || !p->Data)
    return &mmFallback();
  return p;
}

} // namespace

// ============================================================
// VoronoiCoreNode
// ============================================================

VoronoiCoreNode::VoronoiCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_scaleX(4),
      m_scaleY(4),
      m_stretchX(1.0f),
      m_stretchY(1.0f),
      m_intensity(0.75f),
      m_randomness(1.0f),
      m_seed(0.0f) {}

std::vector<std::string> VoronoiCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> VoronoiCoreNode::outputSlotNames() const {
  return {"Color", "F1", "Edge"};
}

void VoronoiCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                              std::vector<GenTexture>& outputs) {
  int w = mmSizeFromIdx(m_widthIdx);
  int h = mmSizeFromIdx(m_heightIdx);
  outputs.resize(3);
  outputs[0].Init(w, h);
  outputs[1].Init(w, h);
  outputs[2].Init(w, h);
  MMVoronoi(&outputs[0], &outputs[1], &outputs[2], m_scaleX, m_scaleY,
            m_stretchX, m_stretchY, m_intensity, m_randomness, m_seed);
}

nlohmann::json VoronoiCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},   {"heightIdx", m_heightIdx},
          {"scaleX", m_scaleX},       {"scaleY", m_scaleY},
          {"stretchX", m_stretchX},   {"stretchY", m_stretchY},
          {"intensity", m_intensity}, {"randomness", m_randomness},
          {"seed", m_seed}};
}

void VoronoiCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("scaleX"))
    m_scaleX = j["scaleX"];
  if (j.contains("scaleY"))
    m_scaleY = j["scaleY"];
  if (j.contains("stretchX"))
    m_stretchX = j["stretchX"];
  if (j.contains("stretchY"))
    m_stretchY = j["stretchY"];
  if (j.contains("intensity"))
    m_intensity = j["intensity"];
  if (j.contains("randomness"))
    m_randomness = j["randomness"];
  if (j.contains("seed"))
    m_seed = j["seed"];
}

// ============================================================
// FBMCoreNode
// ============================================================

FBMCoreNode::FBMCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_mode(MMFbmPerlin),
      m_scaleX(4),
      m_scaleY(4),
      m_folds(0),
      m_octaves(4),
      m_persistence(0.5f),
      m_seed(0.0f) {}

std::vector<std::string> FBMCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> FBMCoreNode::outputSlotNames() const {
  return {"Out"};
}

void FBMCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                          std::vector<GenTexture>& outputs) {
  int w = mmSizeFromIdx(m_widthIdx);
  int h = mmSizeFromIdx(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  MMFbm(outputs[0], m_mode, m_scaleX, m_scaleY, m_folds, m_octaves,
        m_persistence, m_seed);
}

nlohmann::json FBMCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx}, {"heightIdx", m_heightIdx},
          {"mode", m_mode},         {"scaleX", m_scaleX},
          {"scaleY", m_scaleY},     {"folds", m_folds},
          {"octaves", m_octaves},   {"persistence", m_persistence},
          {"seed", m_seed}};
}

void FBMCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("scaleX"))
    m_scaleX = j["scaleX"];
  if (j.contains("scaleY"))
    m_scaleY = j["scaleY"];
  if (j.contains("folds"))
    m_folds = j["folds"];
  if (j.contains("octaves"))
    m_octaves = j["octaves"];
  if (j.contains("persistence"))
    m_persistence = j["persistence"];
  if (j.contains("seed"))
    m_seed = j["seed"];
}

// ============================================================
// BlendCoreNode
// ============================================================

BlendCoreNode::BlendCoreNode() : m_mode(MMBlendNormal), m_opacity(1.0f) {}

std::vector<std::string> BlendCoreNode::inputSlotNames() const {
  return {"A", "B", "Mask"};
}
std::vector<std::string> BlendCoreNode::outputSlotNames() const {
  return {"Out"};
}

void BlendCoreNode::execute(const std::vector<GenTexture*>& inputs,
                            std::vector<GenTexture>& outputs) {
  GenTexture* a = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  GenTexture* b = mmEnsure(inputs.size() > 1 ? inputs[1] : nullptr);
  GenTexture* mask =
      (inputs.size() > 2 && inputs[2] && inputs[2]->Data) ? inputs[2]
                                                          : nullptr;
  outputs.resize(1);
  outputs[0].Init(b->XRes, b->YRes);
  MMBlend(outputs[0], *a, *b, mask, m_mode, m_opacity);
}

nlohmann::json BlendCoreNode::saveParams() const {
  return {{"mode", m_mode}, {"opacity", m_opacity}};
}

void BlendCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("opacity"))
    m_opacity = j["opacity"];
}

// ============================================================
// WarpCoreNode
// ============================================================

WarpCoreNode::WarpCoreNode() : m_amount(0.1f), m_epsilon(0.005f) {}

std::vector<std::string> WarpCoreNode::inputSlotNames() const {
  return {"In", "Height", "Strength"};
}
std::vector<std::string> WarpCoreNode::outputSlotNames() const {
  return {"Out"};
}

void WarpCoreNode::execute(const std::vector<GenTexture*>& inputs,
                           std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  GenTexture* height = mmEnsure(inputs.size() > 1 ? inputs[1] : nullptr);
  GenTexture* strength =
      (inputs.size() > 2 && inputs[2] && inputs[2]->Data) ? inputs[2]
                                                          : nullptr;
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMWarp(outputs[0], *in, *height, strength, m_amount, m_epsilon);
}

nlohmann::json WarpCoreNode::saveParams() const {
  return {{"amount", m_amount}, {"epsilon", m_epsilon}};
}

void WarpCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("amount"))
    m_amount = j["amount"];
  if (j.contains("epsilon"))
    m_epsilon = j["epsilon"];
}

// ============================================================
// ColorizeCoreNode
// ============================================================

ColorizeCoreNode::ColorizeCoreNode() {
  m_stops.push_back({0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
  m_stops.push_back({1.0f, 1.0f, 1.0f, 1.0f, 1.0f});
}

std::vector<std::string> ColorizeCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> ColorizeCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ColorizeCoreNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  // stops must be sorted by position for the ramp lookup
  std::vector<MMGradientStop> sorted = m_stops;
  for (size_t i = 1; i < sorted.size(); i++)
    for (size_t k = i; k > 0 && sorted[k].pos < sorted[k - 1].pos; k--)
      std::swap(sorted[k], sorted[k - 1]);
  MMColorize(outputs[0], *in, sorted.data(), (sInt)sorted.size());
}

nlohmann::json ColorizeCoreNode::saveParams() const {
  nlohmann::json stops = nlohmann::json::array();
  for (auto& s : m_stops)
    stops.push_back({s.pos, s.r, s.g, s.b, s.a});
  return {{"stops", stops}};
}

void ColorizeCoreNode::loadParams(const nlohmann::json& j) {
  if (!j.contains("stops") || !j["stops"].is_array())
    return;
  m_stops.clear();
  for (auto& s : j["stops"]) {
    if (s.is_array() && s.size() >= 5)
      m_stops.push_back({s[0].get<float>(), s[1].get<float>(),
                         s[2].get<float>(), s[3].get<float>(),
                         s[4].get<float>()});
  }
  if (m_stops.empty()) {
    m_stops.push_back({0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
    m_stops.push_back({1.0f, 1.0f, 1.0f, 1.0f, 1.0f});
  }
}

// ============================================================
// MMBricksCoreNode
// ============================================================

MMBricksCoreNode::MMBricksCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_pattern(MMBricksRunningBond),
      m_countX(4),
      m_countY(8),
      m_repeat(1),
      m_offset(0.5f),
      m_mortar(0.1f),
      m_round(0.1f),
      m_bevel(0.2f),
      m_colorBalance(0.5f),
      m_seed(0.0f) {
  float col0[4] = {0.55f, 0.25f, 0.15f, 1.0f};
  float col1[4] = {0.65f, 0.35f, 0.20f, 1.0f};
  float colM[4] = {0.30f, 0.30f, 0.28f, 1.0f};
  memcpy(m_col0, col0, sizeof(col0));
  memcpy(m_col1, col1, sizeof(col1));
  memcpy(m_colMortar, colM, sizeof(colM));
}

std::vector<std::string> MMBricksCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> MMBricksCoreNode::outputSlotNames() const {
  return {"Out"};
}

void MMBricksCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                               std::vector<GenTexture>& outputs) {
  int w = mmSizeFromIdx(m_widthIdx);
  int h = mmSizeFromIdx(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  MMBricks(outputs[0], m_pattern, m_countX, m_countY, m_repeat, m_offset,
           m_mortar, m_round, m_bevel, mmColorToU32(m_col0),
           mmColorToU32(m_col1), mmColorToU32(m_colMortar), m_colorBalance,
           m_seed);
}

nlohmann::json MMBricksCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"pattern", m_pattern},
          {"countX", m_countX},
          {"countY", m_countY},
          {"repeat", m_repeat},
          {"offset", m_offset},
          {"mortar", m_mortar},
          {"round", m_round},
          {"bevel", m_bevel},
          {"col0", {m_col0[0], m_col0[1], m_col0[2], m_col0[3]}},
          {"col1", {m_col1[0], m_col1[1], m_col1[2], m_col1[3]}},
          {"colMortar",
           {m_colMortar[0], m_colMortar[1], m_colMortar[2], m_colMortar[3]}},
          {"colorBalance", m_colorBalance},
          {"seed", m_seed}};
}

void MMBricksCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("pattern"))
    m_pattern = j["pattern"];
  if (j.contains("countX"))
    m_countX = j["countX"];
  if (j.contains("countY"))
    m_countY = j["countY"];
  if (j.contains("repeat"))
    m_repeat = j["repeat"];
  if (j.contains("offset"))
    m_offset = j["offset"];
  if (j.contains("mortar"))
    m_mortar = j["mortar"];
  if (j.contains("round"))
    m_round = j["round"];
  if (j.contains("bevel"))
    m_bevel = j["bevel"];
  if (j.contains("col0"))
    for (int i = 0; i < 4; i++)
      m_col0[i] = j["col0"][i];
  if (j.contains("col1"))
    for (int i = 0; i < 4; i++)
      m_col1[i] = j["col1"][i];
  if (j.contains("colMortar"))
    for (int i = 0; i < 4; i++)
      m_colMortar[i] = j["colMortar"][i];
  if (j.contains("colorBalance"))
    m_colorBalance = j["colorBalance"];
  if (j.contains("seed"))
    m_seed = j["seed"];
}

// ============================================================
// MaterialCoreNode
// ============================================================

MaterialCoreNode::MaterialCoreNode() {
  strncpy(m_baseName, "material", sizeof(m_baseName));
}

std::vector<std::string> MaterialCoreNode::inputSlotNames() const {
  return {"Albedo", "Normal", "Roughness", "Metallic",
          "Height", "AO",     "Emission"};
}
std::vector<std::string> MaterialCoreNode::outputSlotNames() const {
  return {};
}

void MaterialCoreNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  outputs.resize(0);
  auto slots = inputSlotNames();
  for (size_t i = 0; i < slots.size() && i < inputs.size(); i++) {
    if (!inputs[i] || !inputs[i]->Data)
      continue;
    std::string channel = slots[i];
    for (auto& c : channel)
      c = (char)tolower(c);
    std::string path = std::string(m_baseName) + "_" + channel + ".png";
    SaveImage(*inputs[i], path.c_str());
  }
}

nlohmann::json MaterialCoreNode::saveParams() const {
  return {{"baseName", m_baseName}};
}

void MaterialCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("baseName")) {
    std::string s = j["baseName"];
    strncpy(m_baseName, s.c_str(), sizeof(m_baseName) - 1);
    m_baseName[sizeof(m_baseName) - 1] = '\0';
  }
}

// ============================================================
// NormalMapCoreNode
// ============================================================

NormalMapCoreNode::NormalMapCoreNode()
    : m_amount(1.0f), m_format(MMNormalOpenGL) {}

std::vector<std::string> NormalMapCoreNode::inputSlotNames() const {
  return {"Height"};
}
std::vector<std::string> NormalMapCoreNode::outputSlotNames() const {
  return {"Out"};
}

void NormalMapCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                std::vector<GenTexture>& outputs) {
  GenTexture* height = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(height->XRes, height->YRes);
  MMNormalMap(outputs[0], *height, m_amount, m_format);
}

nlohmann::json NormalMapCoreNode::saveParams() const {
  return {{"amount", m_amount}, {"format", m_format}};
}

void NormalMapCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("amount"))
    m_amount = j["amount"];
  if (j.contains("format"))
    m_format = j["format"];
}

// ============================================================
// SdfShapeCoreNode
// ============================================================

SdfShapeCoreNode::SdfShapeCoreNode() : m_widthIdx(3), m_heightIdx(3) {}

std::vector<std::string> SdfShapeCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> SdfShapeCoreNode::outputSlotNames() const {
  return {"Sdf"};
}

void SdfShapeCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                               std::vector<GenTexture>& outputs) {
  int w = mmSizeFromIdx(m_widthIdx);
  int h = mmSizeFromIdx(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  MMSdfShape(outputs[0], m_p);
}

nlohmann::json SdfShapeCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"shape", m_p.shape},
          {"cx", m_p.cx},
          {"cy", m_p.cy},
          {"w", m_p.w},
          {"h", m_p.h},
          {"n", m_p.n},
          {"ir", m_p.ir},
          {"rot", m_p.rot},
          {"ax", m_p.ax},
          {"ay", m_p.ay},
          {"bx", m_p.bx},
          {"by", m_p.by}};
}

void SdfShapeCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("shape"))
    m_p.shape = j["shape"];
  if (j.contains("cx"))
    m_p.cx = j["cx"];
  if (j.contains("cy"))
    m_p.cy = j["cy"];
  if (j.contains("w"))
    m_p.w = j["w"];
  if (j.contains("h"))
    m_p.h = j["h"];
  if (j.contains("n"))
    m_p.n = j["n"];
  if (j.contains("ir"))
    m_p.ir = j["ir"];
  if (j.contains("rot"))
    m_p.rot = j["rot"];
  if (j.contains("ax"))
    m_p.ax = j["ax"];
  if (j.contains("ay"))
    m_p.ay = j["ay"];
  if (j.contains("bx"))
    m_p.bx = j["bx"];
  if (j.contains("by"))
    m_p.by = j["by"];
}

// ============================================================
// SdfOpCoreNode
// ============================================================

SdfOpCoreNode::SdfOpCoreNode() : m_op(MMSdfUnion), m_k(0.1f) {}

std::vector<std::string> SdfOpCoreNode::inputSlotNames() const {
  return {"A", "B"};
}
std::vector<std::string> SdfOpCoreNode::outputSlotNames() const {
  return {"Sdf"};
}

void SdfOpCoreNode::execute(const std::vector<GenTexture*>& inputs,
                            std::vector<GenTexture>& outputs) {
  GenTexture* a = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  GenTexture* b = mmEnsure(inputs.size() > 1 ? inputs[1] : nullptr);
  outputs.resize(1);
  outputs[0].Init(a->XRes, a->YRes);
  MMSdfOp(outputs[0], *a, *b, m_op, m_k);
}

nlohmann::json SdfOpCoreNode::saveParams() const {
  return {{"op", m_op}, {"k", m_k}};
}

void SdfOpCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("op"))
    m_op = j["op"];
  if (j.contains("k"))
    m_k = j["k"];
}

// ============================================================
// SdfTransformCoreNode
// ============================================================

SdfTransformCoreNode::SdfTransformCoreNode()
    : m_tx(0.0f),
      m_ty(0.0f),
      m_rot(0.0f),
      m_scale(1.0f),
      m_round(0.0f),
      m_annularW(0.05f),
      m_annularCount(0) {}

std::vector<std::string> SdfTransformCoreNode::inputSlotNames() const {
  return {"Sdf"};
}
std::vector<std::string> SdfTransformCoreNode::outputSlotNames() const {
  return {"Sdf"};
}

void SdfTransformCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                   std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMSdfTransform(outputs[0], *in, m_tx, m_ty, m_rot, m_scale, m_round,
                 m_annularW, m_annularCount);
}

nlohmann::json SdfTransformCoreNode::saveParams() const {
  return {{"tx", m_tx},     {"ty", m_ty},
          {"rot", m_rot},   {"scale", m_scale},
          {"round", m_round}, {"annularW", m_annularW},
          {"annularCount", m_annularCount}};
}

void SdfTransformCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("tx"))
    m_tx = j["tx"];
  if (j.contains("ty"))
    m_ty = j["ty"];
  if (j.contains("rot"))
    m_rot = j["rot"];
  if (j.contains("scale"))
    m_scale = j["scale"];
  if (j.contains("round"))
    m_round = j["round"];
  if (j.contains("annularW"))
    m_annularW = j["annularW"];
  if (j.contains("annularCount"))
    m_annularCount = j["annularCount"];
}

// ============================================================
// SdfShowCoreNode
// ============================================================

SdfShowCoreNode::SdfShowCoreNode() : m_base(0.0f), m_bevel(0.01f) {}

std::vector<std::string> SdfShowCoreNode::inputSlotNames() const {
  return {"Sdf"};
}
std::vector<std::string> SdfShowCoreNode::outputSlotNames() const {
  return {"Out"};
}

void SdfShowCoreNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMSdfShow(outputs[0], *in, m_base, m_bevel);
}

nlohmann::json SdfShowCoreNode::saveParams() const {
  return {{"base", m_base}, {"bevel", m_bevel}};
}

void SdfShowCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("base"))
    m_base = j["base"];
  if (j.contains("bevel"))
    m_bevel = j["bevel"];
}

// ============================================================
// MakeTileableCoreNode
// ============================================================

MakeTileableCoreNode::MakeTileableCoreNode() : m_width(0.1f) {}

std::vector<std::string> MakeTileableCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> MakeTileableCoreNode::outputSlotNames() const {
  return {"Out"};
}

void MakeTileableCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                   std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMMakeTileable(outputs[0], *in, m_width);
}

nlohmann::json MakeTileableCoreNode::saveParams() const {
  return {{"width", m_width}};
}

void MakeTileableCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("width"))
    m_width = j["width"];
}

// ============================================================
// QuantizeCoreNode
// ============================================================

QuantizeCoreNode::QuantizeCoreNode() : m_steps(4) {}

std::vector<std::string> QuantizeCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> QuantizeCoreNode::outputSlotNames() const {
  return {"Out"};
}

void QuantizeCoreNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMQuantize(outputs[0], *in, m_steps);
}

nlohmann::json QuantizeCoreNode::saveParams() const {
  return {{"steps", m_steps}};
}

void QuantizeCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("steps"))
    m_steps = j["steps"];
}

// ============================================================
// EmbossCoreNode
// ============================================================

EmbossCoreNode::EmbossCoreNode()
    : m_angle(0.0f), m_amount(1.0f), m_width(1) {}

std::vector<std::string> EmbossCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> EmbossCoreNode::outputSlotNames() const {
  return {"Out"};
}

void EmbossCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMEmboss(outputs[0], *in, m_angle, m_amount, m_width);
}

nlohmann::json EmbossCoreNode::saveParams() const {
  return {{"angle", m_angle}, {"amount", m_amount}, {"width", m_width}};
}

void EmbossCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("angle"))
    m_angle = j["angle"];
  if (j.contains("amount"))
    m_amount = j["amount"];
  if (j.contains("width"))
    m_width = j["width"];
}

// ============================================================
// Transform2DCoreNode
// ============================================================

Transform2DCoreNode::Transform2DCoreNode()
    : m_tx(0.0f),
      m_ty(0.0f),
      m_rot(0.0f),
      m_scaleX(1.0f),
      m_scaleY(1.0f),
      m_repeat(true) {}

std::vector<std::string> Transform2DCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> Transform2DCoreNode::outputSlotNames() const {
  return {"Out"};
}

void Transform2DCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                  std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMTransform(outputs[0], *in, m_tx, m_ty, m_rot, m_scaleX, m_scaleY,
              m_repeat);
}

nlohmann::json Transform2DCoreNode::saveParams() const {
  return {{"tx", m_tx},         {"ty", m_ty},
          {"rot", m_rot},       {"scaleX", m_scaleX},
          {"scaleY", m_scaleY}, {"repeat", m_repeat}};
}

void Transform2DCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("tx"))
    m_tx = j["tx"];
  if (j.contains("ty"))
    m_ty = j["ty"];
  if (j.contains("rot"))
    m_rot = j["rot"];
  if (j.contains("scaleX"))
    m_scaleX = j["scaleX"];
  if (j.contains("scaleY"))
    m_scaleY = j["scaleY"];
  if (j.contains("repeat"))
    m_repeat = j["repeat"];
}

// ============================================================
// ShapeCoreNode
// ============================================================

ShapeCoreNode::ShapeCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_shape(MMShapeCircle),
      m_sides(3.0f),
      m_radius(1.0f),
      m_edge(0.2f) {}

std::vector<std::string> ShapeCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> ShapeCoreNode::outputSlotNames() const {
  return {"Out"};
}

void ShapeCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                            std::vector<GenTexture>& outputs) {
  int w = mmSizeFromIdx(m_widthIdx);
  int h = mmSizeFromIdx(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  MMShape(outputs[0], m_shape, m_sides, m_radius, m_edge);
}

nlohmann::json ShapeCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"shape", m_shape},
          {"sides", m_sides},
          {"radius", m_radius},
          {"edge", m_edge}};
}

void ShapeCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("shape"))
    m_shape = j["shape"];
  if (j.contains("sides"))
    m_sides = j["sides"];
  if (j.contains("radius"))
    m_radius = j["radius"];
  if (j.contains("edge"))
    m_edge = j["edge"];
}

// ============================================================
// PatternCoreNode
// ============================================================

PatternCoreNode::PatternCoreNode()
    : m_widthIdx(3),
      m_heightIdx(3),
      m_mix(MMWaveMixMultiply),
      m_xWave(MMWaveSine),
      m_yWave(MMWaveSine),
      m_xScale(4.0f),
      m_yScale(4.0f) {}

std::vector<std::string> PatternCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> PatternCoreNode::outputSlotNames() const {
  return {"Out"};
}

void PatternCoreNode::execute(const std::vector<GenTexture*>& /*inputs*/,
                              std::vector<GenTexture>& outputs) {
  int w = mmSizeFromIdx(m_widthIdx);
  int h = mmSizeFromIdx(m_heightIdx);
  outputs.resize(1);
  outputs[0].Init(w, h);
  MMPattern(outputs[0], m_mix, m_xWave, m_xScale, m_yWave, m_yScale);
}

nlohmann::json PatternCoreNode::saveParams() const {
  return {{"widthIdx", m_widthIdx},
          {"heightIdx", m_heightIdx},
          {"mix", m_mix},
          {"xWave", m_xWave},
          {"yWave", m_yWave},
          {"xScale", m_xScale},
          {"yScale", m_yScale}};
}

void PatternCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
  if (j.contains("mix"))
    m_mix = j["mix"];
  if (j.contains("xWave"))
    m_xWave = j["xWave"];
  if (j.contains("yWave"))
    m_yWave = j["yWave"];
  if (j.contains("xScale"))
    m_xScale = j["xScale"];
  if (j.contains("yScale"))
    m_yScale = j["yScale"];
}

// ============================================================
// CombineCoreNode
// ============================================================

std::vector<std::string> CombineCoreNode::inputSlotNames() const {
  return {"R", "G", "B", "A"};
}
std::vector<std::string> CombineCoreNode::outputSlotNames() const {
  return {"Out"};
}

void CombineCoreNode::execute(const std::vector<GenTexture*>& inputs,
                              std::vector<GenTexture>& outputs) {
  GenTexture* first = nullptr;
  for (auto* in : inputs)
    if (in && in->Data) {
      first = in;
      break;
    }
  outputs.resize(1);
  outputs[0].Init(first ? first->XRes : 256, first ? first->YRes : 256);
  auto get = [&](size_t i) -> GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  MMCombine(outputs[0], get(0), get(1), get(2), get(3));
}

// ============================================================
// DecomposeCoreNode
// ============================================================

std::vector<std::string> DecomposeCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> DecomposeCoreNode::outputSlotNames() const {
  return {"R", "G", "B", "A"};
}

void DecomposeCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(4);
  for (int i = 0; i < 4; i++)
    outputs[i].Init(in->XRes, in->YRes);
  MMDecompose(outputs[0], outputs[1], outputs[2], outputs[3], *in);
}

// ============================================================
// InvertCoreNode
// ============================================================

std::vector<std::string> InvertCoreNode::inputSlotNames() const {
  return {"In"};
}
std::vector<std::string> InvertCoreNode::outputSlotNames() const {
  return {"Out"};
}

void InvertCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  GenTexture* in = mmEnsure(inputs.size() > 0 ? inputs[0] : nullptr);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  MMInvert(outputs[0], *in);
}

// ============================================================
// LayerMixCoreNode
// ============================================================

std::vector<std::string> LayerMixCoreNode::inputSlotNames() const {
  return {"H1", "C1", "ORM1", "EM1", "NM1",
          "H2", "C2", "ORM2", "EM2", "NM2"};
}
std::vector<std::string> LayerMixCoreNode::outputSlotNames() const {
  return {"H", "C", "ORM", "EM", "NM"};
}

nlohmann::json LayerMixCoreNode::saveParams() const {
  return {{"mode", m_mode}, {"width", m_width}};
}

void LayerMixCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("mode"))
    m_mode = j["mode"];
  if (j.contains("width"))
    m_width = j["width"];
}

void LayerMixCoreNode::execute(const std::vector<GenTexture*>& inputs,
                               std::vector<GenTexture>& outputs) {
  auto get = [&](size_t i) -> const GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  int w = 256, h = 256;
  for (size_t i = 0; i < 10; i++)
    if (const GenTexture* t = get(i)) {
      w = t->XRes;
      h = t->YRes;
      break;
    }
  outputs.resize(5);
  GenTexture* outs[5];
  for (int i = 0; i < 5; i++) {
    outputs[i].Init(w, h);
    outs[i] = &outputs[i];
  }
  const GenTexture* l1[5] = {get(0), get(1), get(2), get(3), get(4)};
  const GenTexture* l2[5] = {get(5), get(6), get(7), get(8), get(9)};
  MMLayerMix(outs, l1, l2, m_mode, m_width);
}

// ============================================================
// WorkflowOutputCoreNode
// ============================================================

std::vector<std::string> WorkflowOutputCoreNode::inputSlotNames() const {
  return {"Height", "Albedo", "ORM", "Emission", "Normal"};
}
std::vector<std::string> WorkflowOutputCoreNode::outputSlotNames() const {
  return {"Albedo",  "Metallic",  "Roughness", "Emission",
          "Normal",  "Occlusion", "Depth"};
}

nlohmann::json WorkflowOutputCoreNode::saveParams() const {
  return {{"matNormal", m_matNormal}, {"occlusion", m_occlusion}};
}

void WorkflowOutputCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("matNormal"))
    m_matNormal = j["matNormal"];
  if (j.contains("occlusion"))
    m_occlusion = j["occlusion"];
}

void WorkflowOutputCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                     std::vector<GenTexture>& outputs) {
  auto get = [&](size_t i) -> const GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  int w = 256, h = 256;
  for (size_t i = 0; i < 5; i++)
    if (const GenTexture* t = get(i)) {
      w = t->XRes;
      h = t->YRes;
      break;
    }
  outputs.resize(7);
  for (int i = 0; i < 7; i++)
    outputs[i].Init(w, h);
  MMWorkflowOutput(outputs[0], outputs[1], outputs[2], outputs[3], outputs[4],
                   outputs[5], outputs[6], get(0), get(1), get(2), get(3),
                   get(4), m_matNormal, m_occlusion);
}

// ============================================================
// MathOpCoreNode
// ============================================================

std::vector<std::string> MathOpCoreNode::inputSlotNames() const {
  return {"A", "B"};
}
std::vector<std::string> MathOpCoreNode::outputSlotNames() const {
  return {"Out"};
}

nlohmann::json MathOpCoreNode::saveParams() const {
  return {{"op", m_op},
          {"def1", m_def1},
          {"def2", m_def2},
          {"clamp", m_clamp}};
}

void MathOpCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("op"))
    m_op = j["op"];
  if (j.contains("def1"))
    m_def1 = j["def1"];
  if (j.contains("def2"))
    m_def2 = j["def2"];
  if (j.contains("clamp"))
    m_clamp = j["clamp"];
}

void MathOpCoreNode::execute(const std::vector<GenTexture*>& inputs,
                             std::vector<GenTexture>& outputs) {
  auto get = [&](size_t i) -> const GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  const GenTexture* a = get(0);
  const GenTexture* b = get(1);
  const GenTexture* sz = a ? a : b;
  outputs.resize(1);
  outputs[0].Init(sz ? sz->XRes : 256, sz ? sz->YRes : 256);
  MMMath(outputs[0], a, b, m_op, m_def1, m_def2, m_clamp);
}

// ============================================================
// GradientMMCoreNode
// ============================================================

GradientMMCoreNode::GradientMMCoreNode() {
  m_stops.push_back({0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
  m_stops.push_back({1.0f, 1.0f, 1.0f, 1.0f, 1.0f});
}

std::vector<std::string> GradientMMCoreNode::inputSlotNames() const {
  return {};
}
std::vector<std::string> GradientMMCoreNode::outputSlotNames() const {
  return {"Out"};
}

nlohmann::json GradientMMCoreNode::saveParams() const {
  nlohmann::json stops = nlohmann::json::array();
  for (auto& s : m_stops)
    stops.push_back({s.pos, s.r, s.g, s.b, s.a});
  return {{"stops", stops},   {"repeat", m_repeat},
          {"rotate", m_rotate}, {"mirror", m_mirror},
          {"widthIdx", m_widthIdx}, {"heightIdx", m_heightIdx}};
}

void GradientMMCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("stops") && j["stops"].is_array()) {
    m_stops.clear();
    for (auto& s : j["stops"])
      if (s.is_array() && s.size() >= 5)
        m_stops.push_back({s[0].get<float>(), s[1].get<float>(),
                           s[2].get<float>(), s[3].get<float>(),
                           s[4].get<float>()});
    if (m_stops.empty()) {
      m_stops.push_back({0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
      m_stops.push_back({1.0f, 1.0f, 1.0f, 1.0f, 1.0f});
    }
  }
  if (j.contains("repeat"))
    m_repeat = j["repeat"];
  if (j.contains("rotate"))
    m_rotate = j["rotate"];
  if (j.contains("mirror"))
    m_mirror = j["mirror"];
  if (j.contains("widthIdx"))
    m_widthIdx = j["widthIdx"];
  if (j.contains("heightIdx"))
    m_heightIdx = j["heightIdx"];
}

void GradientMMCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                 std::vector<GenTexture>& outputs) {
  (void)inputs;
  outputs.resize(1);
  outputs[0].Init(mmSizeFromIdx(m_widthIdx), mmSizeFromIdx(m_heightIdx));
  std::vector<MMGradientStop> sorted = m_stops;
  for (size_t i = 1; i < sorted.size(); i++)
    for (size_t k = i; k > 0 && sorted[k].pos < sorted[k - 1].pos; k--)
      std::swap(sorted[k], sorted[k - 1]);
  MMGradientRamp(outputs[0], sorted.data(), (sInt)sorted.size(), m_repeat,
                 m_rotate, m_mirror);
}

// ============================================================
// TilerCoreNode
// ============================================================

std::vector<std::string> TilerCoreNode::inputSlotNames() const {
  return {"In", "Mask"};
}
std::vector<std::string> TilerCoreNode::outputSlotNames() const {
  return {"Out", "Color"};
}

nlohmann::json TilerCoreNode::saveParams() const {
  return {{"tx", m_tx},
          {"ty", m_ty},
          {"overlap", m_overlap},
          {"inputs", m_inputs},
          {"scaleX", m_scaleX},
          {"scaleY", m_scaleY},
          {"fixedOffset", m_fixedOffset},
          {"offset", m_offset},
          {"rotate", m_rotate},
          {"scale", m_scale},
          {"value", m_value},
          {"seed", m_seed}};
}

void TilerCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("tx"))
    m_tx = j["tx"];
  if (j.contains("ty"))
    m_ty = j["ty"];
  if (j.contains("overlap"))
    m_overlap = j["overlap"];
  if (j.contains("inputs"))
    m_inputs = j["inputs"];
  if (j.contains("scaleX"))
    m_scaleX = j["scaleX"];
  if (j.contains("scaleY"))
    m_scaleY = j["scaleY"];
  if (j.contains("fixedOffset"))
    m_fixedOffset = j["fixedOffset"];
  if (j.contains("offset"))
    m_offset = j["offset"];
  if (j.contains("rotate"))
    m_rotate = j["rotate"];
  if (j.contains("scale"))
    m_scale = j["scale"];
  if (j.contains("value"))
    m_value = j["value"];
  if (j.contains("seed"))
    m_seed = j["seed"];
}

void TilerCoreNode::execute(const std::vector<GenTexture*>& inputs,
                            std::vector<GenTexture>& outputs) {
  auto get = [&](size_t i) -> GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  GenTexture* in = mmEnsure(get(0));
  outputs.resize(2);
  outputs[0].Init(in->XRes, in->YRes);
  outputs[1].Init(in->XRes, in->YRes);
  MMTiler(outputs[0], &outputs[1], *in, get(1), m_tx, m_ty, m_overlap,
          m_inputs, m_scaleX, m_scaleY, m_fixedOffset, m_offset, m_rotate,
          m_scale, m_value, m_seed);
}

// ============================================================
// MultiWarpCoreNode
// ============================================================

std::vector<std::string> MultiWarpCoreNode::inputSlotNames() const {
  return {"In", "Height"};
}
std::vector<std::string> MultiWarpCoreNode::outputSlotNames() const {
  return {"Out"};
}

nlohmann::json MultiWarpCoreNode::saveParams() const {
  return {{"size", m_size},
          {"intensity", m_intensity},
          {"quality", m_quality},
          {"mode", m_mode}};
}

void MultiWarpCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("size"))
    m_size = j["size"];
  if (j.contains("intensity"))
    m_intensity = j["intensity"];
  if (j.contains("quality"))
    m_quality = j["quality"];
  if (j.contains("mode"))
    m_mode = j["mode"];
}

void MultiWarpCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                std::vector<GenTexture>& outputs) {
  auto get = [&](size_t i) -> GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  GenTexture* in = mmEnsure(get(0));
  GenTexture* hm = get(1);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  if (hm)
    MMMultiWarp(outputs[0], *in, *hm, m_size, m_intensity, m_quality, m_mode);
  else
    outputs[0] = *in;
}

// ============================================================
// SlopeBlurCoreNode
// ============================================================

std::vector<std::string> SlopeBlurCoreNode::inputSlotNames() const {
  return {"In", "Height"};
}
std::vector<std::string> SlopeBlurCoreNode::outputSlotNames() const {
  return {"Out"};
}

nlohmann::json SlopeBlurCoreNode::saveParams() const {
  return {{"size", m_size}, {"sigma", m_sigma}};
}

void SlopeBlurCoreNode::loadParams(const nlohmann::json& j) {
  if (j.contains("size"))
    m_size = j["size"];
  if (j.contains("sigma"))
    m_sigma = j["sigma"];
}

void SlopeBlurCoreNode::execute(const std::vector<GenTexture*>& inputs,
                                std::vector<GenTexture>& outputs) {
  auto get = [&](size_t i) -> GenTexture* {
    return (i < inputs.size() && inputs[i] && inputs[i]->Data) ? inputs[i]
                                                               : nullptr;
  };
  GenTexture* in = mmEnsure(get(0));
  GenTexture* hm = get(1);
  outputs.resize(1);
  outputs[0].Init(in->XRes, in->YRes);
  if (hm)
    MMSlopeBlur(outputs[0], *in, *hm, m_size, m_sigma);
  else
    outputs[0] = *in;
}
