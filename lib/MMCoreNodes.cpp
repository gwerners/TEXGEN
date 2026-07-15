#include "MMCoreNodes.h"

#include <cstring>

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
