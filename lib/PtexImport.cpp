// PtexImport — Material Maker .ptex to TEXGEN graph conversion.
// See PtexImport.h. Mapping mirrors tools/ptex2texgen.py plus aliases for
// Material Maker 1.x-era community nodes (fbm2/3/4, blend2, normal_map2,
// transform2, tones_*, greyscale, blur variants, buffer/reroute).
#include "PtexImport.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>

using nlohmann::json;

namespace {

// ---- tolerant readers (community files carry strings like "$size") ----

float numOr(const json &p, const char *key, float def) {
  if (!p.contains(key))
    return def;
  const json &v = p[key];
  if (v.is_number())
    return v.get<float>();
  if (v.is_boolean())
    return v.get<bool>() ? 1.0f : 0.0f;
  if (v.is_string()) {
    const std::string &s = v.get_ref<const std::string &>();
    char *end = nullptr;
    float f = strtof(s.c_str(), &end);
    if (end && end != s.c_str())
      return f;
  }
  return def;
}

int intOr(const json &p, const char *key, int def) {
  return (int)(numOr(p, key, (float)def) + 0.5f);
}

bool boolOr(const json &p, const char *key, bool def) {
  if (!p.contains(key))
    return def;
  const json &v = p[key];
  if (v.is_boolean())
    return v.get<bool>();
  if (v.is_number())
    return v.get<float>() != 0.0f;
  return def;
}

std::string strOr(const json &p, const char *key, const std::string &def) {
  if (p.contains(key) && p[key].is_string())
    return p[key].get<std::string>();
  return def;
}

// ---- port maps (MM port index -> TEXGEN slot name; "" = unmapped) ----

const std::map<std::string, std::vector<std::string>> &portsIn() {
  static const std::map<std::string, std::vector<std::string>> m = {
      {"colorize", {"In"}},
      {"transform", {"In", "TX", "TY", "Rot", "SX", "SY"}},
      {"transform2", {"In", "TX", "TY", "Rot", "SX", "SY"}},
      {"shape", {"RadiusMap", "EdgeMap"}},
      {"blend", {"A", "B", "Mask"}},
      {"blend2", {"B", "A", "Mask"}}, // b, l0, a0 (extra layers dropped)
      {"warp", {"In", "Height", "Strength"}},
      {"normal_map", {"Height"}},
      {"normal_map2", {"Height"}},
      {"material",
       {"Albedo", "Metallic", "Roughness", "Emission", "Normal", "AO",
        "Depth"}},
      {"material_tesselated",
       {"Albedo", "Metallic", "Roughness", "Emission", "Normal", "AO",
        "Depth"}},
      {"material_unlit", {"Albedo"}},
      {"combine", {"R", "G", "B", "A"}},
      {"decompose", {"In"}},
      {"invert", {"In"}},
      {"greyscale", {"In"}},
      {"tones", {"In"}},
      {"tones_step", {"In"}},
      {"tones_map", {"In"}},
      {"brightness_contrast", {"In"}},
      {"adjust_hsv", {"In"}},
      {"alter_hsv", {"In"}},
      {"gaussian_blur", {"In", "Sigma"}},
      {"fast_blur", {"In", "Sigma"}},
      {"make_tileable", {"In"}},
      {"emboss", {"In"}},
      {"quantize", {"In"}},
      {"mwf_mix",
       {"H1", "C1", "ORM1", "EM1", "NM1", "H2", "C2", "ORM2", "EM2", "NM2"}},
      {"mwf_mix_smooth",
       {"H1", "C1", "ORM1", "EM1", "NM1", "H2", "C2", "ORM2", "EM2", "NM2"}},
      {"mwf_output", {"Height", "Albedo", "ORM", "Emission", "Normal"}},
      {"math", {"A", "B"}},
      {"tiler", {"In", "Mask"}},
      {"multi_warp", {"In", "Height"}},
      {"slope_blur", {"In", "Height"}},
      {"scale", {"In"}},
      {"translate", {"In"}},
      {"splatter", {"In", "Mask"}},
      {"noise", {"Density"}},
      {"mirror", {"In"}},
      {"edge_detect", {"In"}},
      {"mwf_create_map", {"Height", "Offset"}},
      {"mwf_map", {"Map", "C", "ORM", "EM", "NM"}},
      {"fill", {"In"}},
      {"fill2", {"In"}},
      {"fill_to_uv", {"Fill"}},
      {"fill_to_uv2", {"Fill"}},
      {"fill_to_random_grey", {"Fill"}},
      {"fill_to_random_grey2", {"Fill"}},
      {"fill_to_random_color", {"Fill"}},
      {"fill_to_random_color2", {"Fill"}},
      {"fill_to_random_color3", {"Fill"}},
      {"fill_to_color", {"Fill", "Map"}},
      {"fill_to_color2", {"Fill", "Map"}},
      {"remap", {"In"}},
      {"tile2x2", {"In1", "In2", "In3", "In4"}},
      {"normal_map_convert", {"In"}},
      {"custom_uv", {"In", "Map"}},
      {"smooth_curvature", {"Height"}},
      {"smooth_curvature2", {"Height"}},
      {"occlusion2", {"Height"}},
      {"hbao", {"Height"}},
      {"occlusion", {"Height"}},
      {"noise2", {"Density"}},
      {"edge_detect_2", {"In"}},
      {"smooth_minmax", {"A", "B"}},
      {"weave", {"WidthMap"}},
      {"weave2", {"WidthMap"}},
      {"fill_to_gradient", {"Fill"}},
      {"fill_to_gradient2", {"Fill"}},
      {"fill_to_size2", {"Fill"}},
      {"radial_gradient", {}},
      {"circular_gradient", {}},
      {"rotate", {"In"}},
      {"tones_range", {"In"}},
      {"math_v3", {"A", "B"}},
      {"noise_anisotropic", {}},
      {"height_to_offset", {"Height"}},
      {"bevel", {"In"}},
      {"dilate", {"Mask", "Source"}},
      {"normal_blend", {"Foreground", "Background", "Mask"}},
      {"directional_blur", {"In", "Amount"}},
      {"tiler_advanced",
       {"In", "Mask", "Color1", "Color2", "TrX", "TrY", "Rot", "ScX",
        "ScY"}},
      {"bricks3", {"", "", ""}}, // mortar/bevel/round maps unsupported
  };
  return m;
}

const std::map<std::string, std::vector<std::string>> &portsOut() {
  static const std::map<std::string, std::vector<std::string>> m = {
      // MM voronoi: 0=Nodes(F1), 1=Borders(Edge), 2=Random color, 3=Fill
      {"voronoi", {"F1", "Edge", "Color", "Fill"}},
      {"voronoi2", {"F1", "Edge", ""}},
      {"decompose", {"R", "G", "B", "A"}},
      {"mwf_mix", {"H", "C", "ORM", "EM", "NM"}},
      {"mwf_mix_smooth", {"H", "C", "ORM", "EM", "NM"}},
      {"mwf_output",
       {"Albedo", "Metallic", "Roughness", "Emission", "Normal", "Occlusion",
        "Depth"}},
      {"tiler", {"Out", "Color", ""}},
      {"splatter", {"Out", "Color", ""}},
      {"mwf_map", {"H", "C", "ORM", "EM", "NM"}},
      {"tiler_advanced", {"Out", "Color1", "Color2", "UV"}},
      {"height_to_offset", {"X", "Y"}},
      {"weave2", {"Out", "Horizontal", "Vertical"}},
  };
  return m;
}

// ---- per-type parameter conversion ----

json stopsFromGradient(const json &grad) {
  json stops = json::array();
  if (grad.contains("points") && grad["points"].is_array()) {
    for (auto &pt : grad["points"]) {
      stops.push_back({numOr(pt, "pos", 0.0f), numOr(pt, "r", 0.0f),
                       numOr(pt, "g", 0.0f), numOr(pt, "b", 0.0f),
                       numOr(pt, "a", 1.0f)});
    }
  }
  if (stops.empty())
    stops = {{0, 0, 0, 0, 1}, {1, 1, 1, 1, 1}};
  std::vector<json> v = stops.get<std::vector<json>>();
  std::sort(v.begin(), v.end(), [](const json &a, const json &b) {
    return a[0].get<float>() < b[0].get<float>();
  });
  return json(v);
}

json grayStops(float pos0, float g0, float pos1, float g1) {
  return json::array({json::array({pos0, g0, g0, g0, 1.0f}),
                      json::array({pos1, g1, g1, g1, 1.0f})});
}

// MM fbm noise variant (string form) -> FBM mode
int fbmModeFromString(const std::string &s) {
  static const std::map<std::string, int> m = {
      {"value", 0},     {"perlin", 1},    {"perlinabs", 2},
      {"cellular", 3},  {"cellular2", 4}, {"cellular3", 5},
      {"cellular4", 6}, {"cellular5", 7}, {"cellular6", 8}};
  auto it = m.find(s);
  return it != m.end() ? it->second : 1;
}

// fbm2/3/4 noise enum: Value, Perlin, Simplex, Cellular..Cellular6,
// Voronoise. Simplex/Voronoise approximated by Perlin/Cellular.
int fbmModeFromEnum(int e) {
  if (e <= 1)
    return e;
  if (e == 2)
    return 1; // simplex ~ perlin
  if (e >= 3 && e <= 8)
    return e;
  return 3; // voronoise ~ cellular
}

// Returns true and fills (typeName, out) if the MM type maps to a node.
bool convertParams(const std::string &type, const json &p,
                   const std::string &baseName, std::string &typeName,
                   json &out) {
  auto size3 = [] { return json{{"widthIdx", 3}, {"heightIdx", 3}}; };

  if (type == "perlin_color") {
    // color perlin approximated by grayscale FBM (variation preserved)
    typeName = "FBM";
    out = size3();
    out["mode"] = 0;
    out["scaleX"] = intOr(p, "scale_x", 4);
    out["scaleY"] = intOr(p, "scale_y", 4);
    out["folds"] = 0;
    out["octaves"] = intOr(p, "iterations", 3);
    out["persistence"] = numOr(p, "persistence", 0.5f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "perlin") {
    typeName = "FBM";
    out = size3();
    out["mode"] = 0;
    out["scaleX"] = intOr(p, "scale_x", 4);
    out["scaleY"] = intOr(p, "scale_y", 4);
    out["folds"] = 0;
    out["octaves"] = intOr(p, "iterations", 3);
    out["persistence"] = numOr(p, "persistence", 0.5f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "fbm" || type == "fbm2" || type == "fbm3" || type == "fbm4") {
    typeName = "FBM";
    out = size3();
    if (type == "fbm") {
      out["mode"] = fbmModeFromString(strOr(p, "noise", "perlin"));
    } else {
      out["mode"] = fbmModeFromEnum(intOr(p, "noise", 1));
    }
    out["scaleX"] = intOr(p, "scale_x", 4);
    out["scaleY"] = intOr(p, "scale_y", 4);
    out["folds"] = intOr(p, "folds", 0);
    out["octaves"] = intOr(p, "iterations", 3);
    out["persistence"] = numOr(p, "persistence", 0.5f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "voronoi" || type == "voronoi2") {
    typeName = "Voronoi";
    out = size3();
    out["scaleX"] = intOr(p, "scale_x", 4);
    out["scaleY"] = intOr(p, "scale_y", 4);
    out["stretchX"] = numOr(p, "stretch_x", 1.0f);
    out["stretchY"] = numOr(p, "stretch_y", 1.0f);
    out["intensity"] = numOr(p, "intensity", 0.75f);
    out["randomness"] = numOr(p, "randomness", 1.0f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "colorize") {
    typeName = "Colorize";
    out = {{"stops", stopsFromGradient(p.value("gradient", json::object()))}};
    return true;
  }
  if (type == "blend" || type == "blend2") {
    // blend2 layer params are 1-indexed (blend_type1/amount1); multi-
    // layer instances are pre-expanded into single-layer chains by
    // expandLayeredBlends()
    typeName = "Blend";
    int mode = type == "blend"
                   ? intOr(p, "blend_type", 0)
                   : intOr(p, "blend_type1", intOr(p, "blend_type0", 0));
    if (mode > 14)
      mode = 0; // Vivid/Pin/... not ported — fall back to Normal
    float amount = type == "blend"
                       ? numOr(p, "amount", 1.0f)
                       : numOr(p, "amount1", numOr(p, "amount0", 0.5f));
    out = {{"mode", mode}, {"opacity", amount}};
    return true;
  }
  if (type == "warp") {
    typeName = "Warp";
    out = {{"amount", numOr(p, "amount", 0.1f)},
           {"epsilon", numOr(p, "eps", 0.005f)},
           {"mode", intOr(p, "mode", 0)}};
    return true;
  }
  if (type == "normal_map" || type == "normal_map2") {
    typeName = "NormalMap";
    float amount = type == "normal_map"
                       ? numOr(p, "param1", 1.0f)
                       : numOr(p, "amount", numOr(p, "strength", 1.0f));
    // MM's 'default' format is its own -Z convention; engines expect
    // OpenGL (+Z), so Default maps to OpenGL here.
    int fmt = intOr(p, "param2", 0);
    if (fmt == 0)
      fmt = 1;
    out = {{"amount", amount}, {"format", fmt}};
    return true;
  }
  if (type == "uniform") {
    typeName = "Color";
    json col = p.value("color", json::object());
    out = size3();
    out["r"] = numOr(col, "r", 0.0f);
    out["g"] = numOr(col, "g", 0.0f);
    out["b"] = numOr(col, "b", 0.0f);
    out["a"] = numOr(col, "a", 1.0f);
    return true;
  }
  if (type == "uniform_greyscale") {
    typeName = "Color";
    float v = numOr(p, "color", 0.5f);
    out = size3();
    out["r"] = v;
    out["g"] = v;
    out["b"] = v;
    out["a"] = 1.0f;
    return true;
  }
  if (type == "bricks" || type == "bricks2") {
    typeName = "BricksMM";
    static const std::map<std::string, int> patterns = {
        {"rb", 0}, {"rb2", 1}, {"hb", 2}, {"bw", 3}, {"sb", 4}};
    std::string pat = strOr(p, "pattern", "rb");
    auto it = patterns.find(pat);
    out = size3();
    out["pattern"] = it != patterns.end() ? it->second : 0;
    out["countX"] = intOr(p, "columns", 4);
    out["countY"] = intOr(p, "rows", 8);
    out["repeat"] = intOr(p, "repeat", 1);
    out["offset"] = numOr(p, "row_offset", 0.5f);
    out["mortar"] = numOr(p, "mortar", 0.1f);
    out["round"] = numOr(p, "round", 0.1f);
    out["bevel"] = numOr(p, "bevel", 0.2f);
    out["colorBalance"] = 0.5f;
    out["seed"] = 0.0f;
    return true;
  }
  if (type == "transform" || type == "transform2") {
    typeName = "Transform2D";
    bool repeat = type == "transform" ? boolOr(p, "repeat", false)
                                      : (intOr(p, "mode", 0) == 1);
    out = {{"tx", numOr(p, "translate_x", 0.0f)},
           {"ty", numOr(p, "translate_y", 0.0f)},
           {"rot", numOr(p, "rotate", 0.0f)},
           {"scaleX", numOr(p, "scale_x", 1.0f)},
           {"scaleY", numOr(p, "scale_y", 1.0f)},
           {"repeat", repeat}};
    return true;
  }
  if (type == "shape") {
    typeName = "Shape";
    out = size3();
    out["shape"] = intOr(p, "shape", 0);
    out["sides"] = numOr(p, "sides", 3.0f);
    out["radius"] = numOr(p, "radius", 1.0f);
    out["edge"] = numOr(p, "edge", 0.2f);
    return true;
  }
  if (type == "pattern") {
    typeName = "Pattern";
    out = size3();
    out["mix"] = intOr(p, "mix", 0);
    out["xWave"] = intOr(p, "x_wave", 0);
    out["xScale"] = numOr(p, "x_scale", 4.0f);
    out["yWave"] = intOr(p, "y_wave", 0);
    out["yScale"] = numOr(p, "y_scale", 4.0f);
    return true;
  }
  if (type == "combine") {
    typeName = "Combine";
    out = json::object();
    return true;
  }
  if (type == "decompose") {
    typeName = "Decompose";
    out = json::object();
    return true;
  }
  if (type == "invert") {
    typeName = "Invert";
    out = json::object();
    return true;
  }
  if (type == "greyscale") {
    typeName = "Colorize";
    out = {{"stops", grayStops(0.0f, 0.0f, 1.0f, 1.0f)}};
    return true;
  }
  if (type == "tones") {
    typeName = "Levels";
    auto col = [&](const char* key, float defR, float defG, float defB,
                   float defA) {
      json c = p.value(key, json::object());
      return json{numOr(c, "r", defR), numOr(c, "g", defG),
                  numOr(c, "b", defB), numOr(c, "a", defA)};
    };
    out = {{"inMin", col("in_min", 0.0f, 0.0f, 0.0f, 0.0f)},
           {"inMid", col("in_mid", 0.5f, 0.5f, 0.5f, 0.5f)},
           {"inMax", col("in_max", 1.0f, 1.0f, 1.0f, 1.0f)},
           {"outMin", col("out_min", 0.0f, 0.0f, 0.0f, 0.0f)},
           {"outMax", col("out_max", 1.0f, 1.0f, 1.0f, 1.0f)}};
    return true;
  }
  if (type == "tones_step") {
    // clamp((x - value) / width + 0.5): the ramp spans width around
    // value (width divides — small width means a sharp step)
    typeName = "Colorize";
    float value = numOr(p, "value", 0.5f);
    float width = numOr(p, "width", 1.0f);
    if (width < 1e-4f)
      width = 1e-4f;
    float lo = value - 0.5f * width, hi = value + 0.5f * width;
    bool invert = boolOr(p, "invert", false);
    out = {{"stops", invert ? grayStops(lo, 1.0f, hi, 0.0f)
                            : grayStops(lo, 0.0f, hi, 1.0f)}};
    return true;
  }
  if (type == "tones_map") {
    typeName = "Colorize";
    out = {{"stops", grayStops(numOr(p, "in_min", 0.0f),
                               numOr(p, "out_min", 0.0f),
                               numOr(p, "in_max", 1.0f),
                               numOr(p, "out_max", 1.0f))}};
    return true;
  }
  if (type == "brightness_contrast") {
    typeName = "HSCB";
    out = {{"hue", 0.0f},
           {"sat", 1.0f},
           {"contrast", numOr(p, "contrast", 1.0f)},
           {"brightness", 1.0f + numOr(p, "brightness", 0.0f)}};
    return true;
  }
  if (type == "adjust_hsv" || type == "alter_hsv") {
    typeName = "HSCB";
    out = {{"hue", numOr(p, "hue", 0.0f)},
           {"sat", numOr(p, "saturation", 1.0f)},
           {"contrast", 1.0f},
           {"brightness", numOr(p, "value", 1.0f)}};
    return true;
  }
  if (type == "gaussian_blur" || type == "fast_blur") {
    // approximation: sigma in texels -> normalized blur size
    typeName = "Blur";
    float sigma = numOr(p, "sigma", numOr(p, "param1", 8.0f));
    float size = sigma / 256.0f;
    out = {{"sizex", size}, {"sizey", size}, {"order", 2}, {"mode", 3}};
    return true;
  }
  if (type == "make_tileable") {
    typeName = "MakeTileable";
    out = {{"width", numOr(p, "w", 0.1f)}};
    return true;
  }
  if (type == "emboss") {
    typeName = "Emboss";
    out = {{"angle", numOr(p, "angle", 0.0f)},
           {"amount", numOr(p, "amount", 1.0f)},
           {"width", intOr(p, "width", 1)}};
    return true;
  }
  if (type == "quantize") {
    typeName = "Quantize";
    out = {{"steps", intOr(p, "steps", 4)}};
    return true;
  }
  if (type == "material" || type == "material_tesselated" ||
      type == "material_unlit") {
    typeName = "Material";
    out = {{"baseName", baseName}};
    return true;
  }
  if (type == "math" || type == "math_v3") {
    // math_v3 shares the first 20 ops (its vector ops 20+ fall back to add)
    typeName = "MathOp";
    int op = intOr(p, "op", 0);
    if (op > 36 || (type == "math_v3" && op > 19))
      op = 0;
    out = {{"op", op},
           {"def1", numOr(p, "default_in1", numOr(p, "d_in1_x", 0.0f))},
           {"def2", numOr(p, "default_in2", numOr(p, "d_in2_x", 0.0f))},
           {"clamp", boolOr(p, "clamp", false)}};
    return true;
  }
  if (type == "noise_white") {
    typeName = "DotNoise";
    int gridExp = intOr(p, "size", 11);
    out = size3();
    out["grid"] = 1 << (gridExp < 1 ? 1 : (gridExp > 12 ? 12 : gridExp));
    out["density"] = 0.5f;
    out["seed"] = numOr(p, "seed", 0.0f);
    out["mode"] = 1;
    return true;
  }
  if (type == "clouds_noise") {
    // 60-node MM macro with inner custom shaders; approximated by a
    // perlin FBM with matching scale
    typeName = "FBM";
    out = size3();
    out["mode"] = 1;
    out["scaleX"] = intOr(p, "n_scale", 4);
    out["scaleY"] = intOr(p, "n_scale", 4);
    out["folds"] = 0;
    out["octaves"] = 5;
    out["persistence"] = 0.5f;
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "bricks3") {
    typeName = "BricksMM";
    out = size3();
    out["pattern"] = intOr(p, "pattern", 2);
    out["countX"] = intOr(p, "columns", 3);
    out["countY"] = intOr(p, "rows", 6);
    out["repeat"] = intOr(p, "repeat", 1);
    out["offset"] = numOr(p, "row_offset", 0.5f);
    out["mortar"] = numOr(p, "mortar", 0.1f);
    out["round"] = numOr(p, "round", 0.0f);
    out["bevel"] = numOr(p, "bevel", 0.1f);
    out["colorBalance"] = 0.5f;
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "gradient") {
    typeName = "GradientMM";
    out = {{"stops", stopsFromGradient(p.value("gradient", json::object()))},
           {"repeat", numOr(p, "repeat", 1.0f)},
           {"rotate", numOr(p, "rotate", 0.0f)},
           {"mirror", boolOr(p, "mirror", false)},
           {"widthIdx", 3},
           {"heightIdx", 3}};
    return true;
  }
  if (type == "tiler") {
    typeName = "Tiler";
    int tsIdx = intOr(p, "inputs", 0); // enum index -> tileset count
    out = {{"tx", numOr(p, "tx", 4.0f)},
           {"ty", numOr(p, "ty", 4.0f)},
           {"overlap", intOr(p, "overlap", 1)},
           {"inputs", tsIdx == 2 ? 4 : (tsIdx == 1 ? 2 : 1)},
           {"scaleX", numOr(p, "scale_x", 1.0f)},
           {"scaleY", numOr(p, "scale_y", 1.0f)},
           {"fixedOffset", numOr(p, "fixed_offset", 0.5f)},
           {"offset", numOr(p, "offset", 0.5f)},
           {"rotate", numOr(p, "rotate", 0.0f)},
           {"scale", numOr(p, "scale", 0.0f)},
           {"value", numOr(p, "value", 0.5f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "multi_warp") {
    // graph macro: param0=size, param1=intensity, param2=quality,
    // param3=mode (see multi_warp.mmg gen_parameters)
    typeName = "MultiWarp";
    out = {{"size", numOr(p, "param0", 9.0f)},
           {"intensity", numOr(p, "param1", 0.5f)},
           {"quality", numOr(p, "param2", 50.0f)},
           {"mode", intOr(p, "param3", 2)}};
    return true;
  }
  if (type == "height_to_offset") {
    typeName = "HeightToOffset";
    out = {{"target", numOr(p, "target", 0.5f)}};
    return true;
  }
  if (type == "noise_anisotropic") {
    typeName = "AnisotropicNoise";
    out = size3();
    out["scaleX"] = numOr(p, "scale_x", 4.0f);
    out["scaleY"] = numOr(p, "scale_y", 256.0f);
    out["smoothness"] = numOr(p, "smoothness", 1.0f);
    out["interpolation"] = numOr(p, "interpolation", 1.0f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "tiler_advanced") {
    typeName = "TilerAdvanced";
    int tsIdx = intOr(p, "inputs", 0);
    out = {{"tx", numOr(p, "tx", 4.0f)},
           {"ty", numOr(p, "ty", 4.0f)},
           {"overlap", intOr(p, "overlap", 1)},
           {"inputs", tsIdx == 2 ? 4 : (tsIdx == 1 ? 2 : 1)},
           {"translateX", numOr(p, "translate_x", 0.0f)},
           {"translateY", numOr(p, "translate_y", 0.0f)},
           {"rotate", numOr(p, "rotate", 0.0f)},
           {"scaleX", numOr(p, "scale_x", 1.0f)},
           {"scaleY", numOr(p, "scale_y", 1.0f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "bevel") {
    typeName = "Bevel";
    json curve = json::array();
    auto bj = p.value("bevel", json::object());
    if (bj.contains("points") && bj["points"].is_array())
      for (auto &pt : bj["points"])
        curve.push_back({numOr(pt, "x", 0.0f), numOr(pt, "y", 0.0f),
                         numOr(pt, "ls", 0.0f), numOr(pt, "rs", 0.0f)});
    out = {{"distance", numOr(p, "distance", 0.1f)}, {"curve", curve}};
    return true;
  }
  if (type == "dilate") {
    // dilate.mmg graph params: param0=size (buffer resolution, N/A),
    // param1=length, param2=fill, param3=distance function, param4=tile
    // (we are always toroidal)
    typeName = "Dilate";
    out = {{"length", numOr(p, "param1", 0.27f)},
           {"fill", numOr(p, "param2", 0.0f)},
           {"metric", intOr(p, "param3", 0)}};
    return true;
  }
  if (type == "normal_blend") {
    typeName = "NormalBlend";
    out = {{"amount", numOr(p, "amount", 0.5f)}};
    return true;
  }
  if (type == "color_noise") {
    typeName = "ColorNoise";
    int gridExp = intOr(p, "size", 8);
    out = size3();
    out["grid"] = 1 << (gridExp < 1 ? 1 : (gridExp > 12 ? 12 : gridExp));
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "directional_blur") {
    // directional_blur.mmg graph params: param0=size (exponent),
    // param1=sigma, param2=angle, param3=mode
    typeName = "DirectionalBlur";
    int szExp = intOr(p, "param0", 9);
    out = {{"size", (float)(1 << (szExp < 4 ? 4 : (szExp > 12 ? 12 : szExp)))},
           {"sigma", numOr(p, "param1", 0.5f)},
           {"angle", numOr(p, "param2", 0.0f)},
           {"mode", intOr(p, "param3", 0)}};
    return true;
  }
  if (type == "sphere") {
    typeName = "Sphere";
    out = size3();
    out["cx"] = numOr(p, "cx", 0.5f);
    out["cy"] = numOr(p, "cy", 0.5f);
    out["r"] = numOr(p, "r", 0.5f);
    out["normalized"] = boolOr(p, "normalized", false);
    return true;
  }
  if (type == "noise") {
    // MM 'size' params store the exponent (2^n texels)
    typeName = "DotNoise";
    int gridExp = intOr(p, "size", 8);
    out = size3();
    out["grid"] = 1 << (gridExp < 1 ? 1 : (gridExp > 12 ? 12 : gridExp));
    out["density"] = numOr(p, "density", 0.5f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "scratches") {
    typeName = "Scratches";
    out = size3();
    out["layers"] = intOr(p, "layers", 4);
    out["length"] = numOr(p, "length", 0.25f);
    out["width"] = numOr(p, "width", 0.5f);
    out["waviness"] = numOr(p, "waviness", 0.5f);
    out["angle"] = numOr(p, "angle", 0.0f);
    out["randomness"] = numOr(p, "randomness", 0.5f);
    out["seed"] = numOr(p, "seed", 0.0f);
    return true;
  }
  if (type == "mirror") {
    typeName = "Mirror";
    out = {{"direction", intOr(p, "direction", 0)},
           {"offset", numOr(p, "offset", 0.0f)},
           {"flipSides", boolOr(p, "flip_sides", false)}};
    return true;
  }
  if (type == "edge_detect") {
    typeName = "EdgeDetect";
    int sizeExp = intOr(p, "size", 9);
    out = {{"size", (float)(1 << (sizeExp < 1 ? 1 : (sizeExp > 12 ? 12
                                                                  : sizeExp)))},
           {"width", intOr(p, "width", 1)},
           {"threshold", numOr(p, "threshold", 0.5f)}};
    return true;
  }
  if (type == "mwf_create_map") {
    typeName = "CreateMap";
    out = {{"height", numOr(p, "height", 1.0f)},
           {"angle", numOr(p, "angle", 0.0f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "mwf_map") {
    typeName = "MatMap";
    out = json::object();
    return true;
  }
  if (type == "fill" || type == "fill2") {
    typeName = "Fill";
    out = json::object();
    return true;
  }
  if (type == "fill_to_uv" || type == "fill_to_uv2") {
    typeName = "FillToUV";
    out = {{"mode", intOr(p, "mode", 0)}, {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "fill_to_random_grey" || type == "fill_to_random_grey2") {
    typeName = "FillToRandomGray";
    out = {{"edgecolor", numOr(p, "edgecolor", 1.0f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "fill_to_random_color" || type == "fill_to_random_color2" ||
      type == "fill_to_random_color3") {
    typeName = "FillToRandomColor";
    json ec = p.value("edgecolor", json::object());
    out = {{"edge",
            {numOr(ec, "r", 1.0f), numOr(ec, "g", 1.0f), numOr(ec, "b", 1.0f)}},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "fill_to_color" || type == "fill_to_color2") {
    typeName = "FillToColor";
    json ec = p.value("edgecolor", json::object());
    out = {{"edge",
            {numOr(ec, "r", 1.0f), numOr(ec, "g", 1.0f), numOr(ec, "b", 1.0f),
             numOr(ec, "a", 1.0f)}}};
    return true;
  }
  if (type == "remap") {
    typeName = "Remap";
    out = {{"min", numOr(p, "min", 0.0f)},
           {"max", numOr(p, "max", 1.0f)},
           {"step", numOr(p, "step", 0.0f)}};
    return true;
  }
  if (type == "tile2x2") {
    typeName = "Tile2x2";
    out = json::object();
    return true;
  }
  if (type == "normal_map_convert") {
    typeName = "NormalConvert";
    out = {{"op", intOr(p, "op", 1)}};
    return true;
  }
  if (type == "custom_uv") {
    typeName = "CustomUV";
    int tsIdx = intOr(p, "inputs", 0);
    out = {{"inputs", tsIdx == 2 ? 4 : (tsIdx == 1 ? 2 : 1)},
           {"sx", numOr(p, "sx", 1.0f)},
           {"sy", numOr(p, "sy", 1.0f)},
           {"rotate", numOr(p, "rotate", 0.0f)},
           {"scale", numOr(p, "scale", 0.5f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "smooth_curvature" || type == "smooth_curvature2") {
    // graph macro: param1=quality, param2=strength, param3=radius
    typeName = "SmoothCurvature";
    out = {{"quality", numOr(p, "param1", 4.0f)},
           {"strength", numOr(p, "param2", 1.0f)},
           {"radius", numOr(p, "param3", 1.0f)}};
    return true;
  }
  if (type == "occlusion2" || type == "hbao" || type == "occlusion") {
    // all are shader-based AO from height; approximated by blur AO
    typeName = "AmbientOcclusion";
    out = {{"radius", 0.05f}, {"strength", numOr(p, "param2", 1.0f)}};
    return true;
  }
  if (type == "noise2") {
    // noise.mmg with a density input; same per-cell threshold dots
    typeName = "DotNoise";
    int gridExp = intOr(p, "size", 8);
    out = size3();
    out["grid"] = 1 << (gridExp < 1 ? 1 : (gridExp > 12 ? 12 : gridExp));
    out["density"] = numOr(p, "density", 0.5f);
    out["seed"] = numOr(p, "seed", 0.0f);
    out["mode"] = 0;
    return true;
  }
  if (type == "edge_detect_2") {
    typeName = "EdgeDetect2";
    int szExp = intOr(p, "size", 9);
    out = {{"size", (float)(1 << (szExp < 4 ? 4 : (szExp > 12 ? 12 : szExp)))}};
    return true;
  }
  if (type == "smooth_minmax") {
    typeName = "SmoothMinMax";
    out = {{"op", intOr(p, "op", 0)},
           {"k", numOr(p, "k", 0.0f)},
           {"def1", numOr(p, "default_in1", 0.0f)},
           {"def2", numOr(p, "default_in2", 0.0f)}};
    return true;
  }
  if (type == "weave") {
    typeName = "Weave";
    out = size3();
    out["columns"] = intOr(p, "columns", 4);
    out["rows"] = intOr(p, "rows", 4);
    out["width"] = numOr(p, "width", 0.8f);
    return true;
  }
  if (type == "weave2") {
    typeName = "Weave2";
    out = size3();
    out["columns"] = intOr(p, "columns", 4);
    out["rows"] = intOr(p, "rows", 4);
    out["widthX"] = numOr(p, "width_x", 0.8f);
    out["widthY"] = numOr(p, "width_y", 0.8f);
    out["stitch"] = numOr(p, "stitch", 1.0f);
    return true;
  }
  if (type == "fill_to_gradient" || type == "fill_to_gradient2") {
    typeName = "FillToGradient";
    out = {{"stops", stopsFromGradient(p.value("grad", json::object()))},
           {"mode", intOr(p, "mode", 0)},
           {"layers", intOr(p, "layers", 1)},
           {"rotate", numOr(p, "rotate", 0.0f)},
           {"rndRotate", numOr(p, "r_rotate", 0.0f)},
           {"rndOffset", numOr(p, "r_offset", 0.0f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "fill_to_size2") {
    typeName = "FillToSize";
    out = {{"formula", intOr(p, "formula", 0)}};
    return true;
  }
  if (type == "radial_gradient" || type == "circular_gradient") {
    typeName = "GradientMM";
    out = {{"stops", stopsFromGradient(p.value("gradient", json::object()))},
           {"repeat", numOr(p, "repeat", 1.0f)},
           {"rotate", 0.0f},
           {"mirror", boolOr(p, "mirror", false)},
           {"shape", type == "radial_gradient" ? 1 : 2},
           {"widthIdx", 3},
           {"heightIdx", 3}};
    return true;
  }
  if (type == "rotate") {
    // rotation around (0.5+cx, 0.5+cy); the center offset is dropped
    typeName = "Transform2D";
    out = {{"tx", 0.0f},
           {"ty", 0.0f},
           {"rot", numOr(p, "rotate", 0.0f)},
           {"scaleX", 1.0f},
           {"scaleY", 1.0f},
           {"repeat", true}};
    return true;
  }
  if (type == "tones_range") {
    // tent curve peaked at 'value': expressible as Colorize stops
    typeName = "Colorize";
    float value = numOr(p, "value", 0.5f);
    float width = numOr(p, "width", 0.25f);
    if (width < 1e-4f)
      width = 1e-4f;
    float contrast = numOr(p, "contrast", 0.5f);
    if (contrast > 0.999f)
      contrast = 0.999f;
    if (contrast < 0.0f)
      contrast = 0.0f;
    bool invert = boolOr(p, "invert", false);
    const float lo = invert ? 1.0f : 0.0f;
    const float hi = invert ? 0.0f : 1.0f;
    json stops = json::array();
    stops.push_back({value - 0.5f * width, lo, lo, lo, 1.0f});
    stops.push_back({value - 0.5f * width * contrast, hi, hi, hi, 1.0f});
    stops.push_back({value + 0.5f * width * contrast, hi, hi, hi, 1.0f});
    stops.push_back({value + 0.5f * width, lo, lo, lo, 1.0f});
    out = {{"stops", stops}};
    return true;
  }
  if (type == "slope_blur") {
    // graph macro: param0=size, param1=sigma
    typeName = "SlopeBlur";
    out = {{"size", numOr(p, "param0", 9.0f)},
           {"sigma", numOr(p, "param1", 0.5f)}};
    return true;
  }
  if (type == "scale") {
    // scale around (0.5+cx, 0.5+cy); the center offset is dropped
    typeName = "Transform2D";
    out = {{"tx", 0.0f},
           {"ty", 0.0f},
           {"rot", 0.0f},
           {"scaleX", numOr(p, "scale_x", 1.0f)},
           {"scaleY", numOr(p, "scale_y", 1.0f)},
           {"repeat", true}};
    return true;
  }
  if (type == "translate") {
    typeName = "Transform2D";
    out = {{"tx", numOr(p, "translate_x", 0.0f)},
           {"ty", numOr(p, "translate_y", 0.0f)},
           {"rot", 0.0f},
           {"scaleX", 1.0f},
           {"scaleY", 1.0f},
           {"repeat", true}};
    return true;
  }
  if (type == "splatter") {
    // approximated by the tiler with a grid of ~count instances
    typeName = "Tiler";
    int count = intOr(p, "count", 10);
    int grid = 1;
    while (grid * grid < count)
      grid++;
    int tsIdx = intOr(p, "inputs", 0);
    out = {{"tx", (float)grid},
           {"ty", (float)grid},
           {"overlap", 2},
           {"inputs", tsIdx == 2 ? 4 : (tsIdx == 1 ? 2 : 1)},
           {"scaleX", numOr(p, "scale_x", 1.0f)},
           {"scaleY", numOr(p, "scale_y", 1.0f)},
           {"fixedOffset", 0.0f},
           {"offset", 1.0f},
           {"rotate", numOr(p, "rotate", 0.0f)},
           {"scale", numOr(p, "scale", 0.0f)},
           {"value", numOr(p, "value", 0.5f)},
           {"seed", numOr(p, "seed", 0.0f)}};
    return true;
  }
  if (type == "mwf_mix" || type == "mwf_mix_smooth") {
    typeName = "LayerMix";
    out = {{"mode", type == "mwf_mix" ? 0 : 1},
           {"width", numOr(p, "width", 0.05f)}};
    return true;
  }
  if (type == "mwf_output") {
    // param0 = material-normal weight, param2 = occlusion strength
    typeName = "WorkflowOutput";
    out = {{"matNormal", numOr(p, "param0", 1.0f)},
           {"occlusion", numOr(p, "param2", 1.0f)}};
    return true;
  }
  return false;
}

// Types that never affect the output and are dropped silently.
bool isIgnorable(const std::string &type) {
  static const std::set<std::string> s = {"remote", "debug", "export",
                                          "ios"};
  return s.count(type) > 0;
}

// Types replaced by a plain wire (single input port 0 -> all consumers).
// buffer/reroute are lossless plumbing; the tone/sharpen family defaults
// close to identity and is approximated by a wire.
bool isPassthrough(const std::string &type) {
  // variations_* re-evaluate their upstream with a shifted seed, which
  // a baked-texture pipeline cannot do — every output = the input.
  static const std::set<std::string> s = {
      "buffer",     "reroute", "supersample",
      "auto_tones", "tonality", "sharpen", "denoiser",
      "variations_greyscale", "variations_color", "optional"};
  return s.count(type) > 0;
}

// Remove passthrough nodes, rewiring their consumers to their source.
// Also collapses MM switch nodes: output port o resolves to the input
// feeding port (source * outputs + o).
void collapsePassthroughs(json &nodes, json &conns) {
  std::set<std::string> pass;
  std::map<std::string, std::pair<int, int>> switches; // -> source, outputs
  // 'optional' passes its input through, or its default color when
  // unconnected; 'tile2x2_variations' minus the (impossible) variation
  // re-seeding is the same input in all four quadrants.
  {
    std::set<std::string> fed;
    for (auto &c : conns)
      fed.insert(c.value("to", std::string()));
    json extra = json::array();
    for (auto &n : nodes) {
      const std::string t = n.value("type", std::string());
      if (t == "optional" && !fed.count(n.value("name", std::string()))) {
        json p = n.value("parameters", json::object());
        n["type"] = "uniform";
        n["parameters"] = {{"color", p.value("d", json::object())}};
      } else if (t == "tile2x2_variations") {
        n["type"] = "tile2x2";
        const std::string nm = n.value("name", std::string());
        for (auto &c : conns)
          if (c.value("to", std::string()) == nm &&
              c.value("to_port", 0) == 0)
            for (int port = 1; port <= 3; port++) {
              json c2 = c;
              c2["to_port"] = port;
              extra.push_back(c2);
            }
      }
    }
    for (auto &c : extra)
      conns.push_back(c);
  }
  json keptNodes = json::array();
  for (auto &n : nodes) {
    std::string t = n.value("type", std::string());
    std::string nm = n.value("name", std::string());
    if (isPassthrough(t)) {
      pass.insert(nm);
    } else if (t == "switch") {
      pass.insert(nm);
      json p = n.value("parameters", json::object());
      int outs = intOr(p, "outputs", 1);
      switches[nm] = {intOr(p, "source", 0), outs < 1 ? 1 : outs};
    } else {
      keptNodes.push_back(n);
    }
  }
  if (pass.empty())
    return;
  // source feeding each passthrough's input ports
  std::map<std::pair<std::string, int>, std::pair<std::string, int>> src;
  for (auto &c : conns) {
    std::string to = c.value("to", std::string());
    if (pass.count(to))
      src[{to, c.value("to_port", 0)}] = {c.value("from", std::string()),
                                          c.value("from_port", 0)};
  }
  auto resolve = [&](std::string from,
                     int fromPort) -> std::pair<std::string, int> {
    int guard = 0;
    while (pass.count(from) && guard++ < 64) {
      int wanted = 0;
      auto sw = switches.find(from);
      if (sw != switches.end())
        wanted = sw->second.first * sw->second.second + fromPort;
      auto it = src.find({from, wanted});
      if (it == src.end())
        return {std::string(), 0};
      from = it->second.first;
      fromPort = it->second.second;
    }
    return {from, fromPort};
  };
  json keptConns = json::array();
  for (auto &c : conns) {
    std::string to = c.value("to", std::string());
    if (pass.count(to))
      continue; // feeding a passthrough — replaced by resolution
    auto r = resolve(c.value("from", std::string()), c.value("from_port", 0));
    if (r.first.empty())
      continue;
    json cj = c;
    cj["from"] = r.first;
    cj["from_port"] = r.second;
    keptConns.push_back(cj);
  }
  nodes = keptNodes;
  conns = keptConns;
}

// blend2 stacks any number of layers over a background (layer i sits
// at port 2i-1, its opacity mask at port 2i, params blend_type<i> /
// amount<i>). Expand each multi-layer instance into a chain of
// single-layer blend2 nodes so the 1:1 node mapping applies.
void expandLayeredBlends(json &nodes, json &conns) {
  std::set<std::string> blend2s;
  for (auto &n : nodes)
    if (n.value("type", std::string()) == "blend2")
      blend2s.insert(n.value("name", std::string()));
  if (blend2s.empty())
    return;

  std::map<std::string, int> maxLayer;
  for (auto &c : conns) {
    std::string to = c.value("to", std::string());
    if (!blend2s.count(to))
      continue;
    int tp = c.value("to_port", 0);
    int layer = (tp + 1) / 2; // ports 1,2 -> layer 1; 3,4 -> 2; ...
    if (layer > maxLayer[to])
      maxLayer[to] = layer;
  }

  json newNodes = json::array();
  for (auto &n : nodes) {
    newNodes.push_back(n);
    std::string nm = n.value("name", std::string());
    if (!blend2s.count(nm) || maxLayer[nm] <= 1)
      continue;
    json params = n.value("parameters", json::object());
    for (int i = 2; i <= maxLayer[nm]; i++) {
      json syn;
      syn["name"] = nm + "__layer" + std::to_string(i);
      syn["type"] = "blend2";
      syn["node_position"] = n.value("node_position", json::object());
      json sp = json::object();
      std::string bt = "blend_type" + std::to_string(i);
      std::string am = "amount" + std::to_string(i);
      if (params.contains(bt))
        sp["blend_type1"] = params[bt];
      if (params.contains(am))
        sp["amount1"] = params[am];
      syn["parameters"] = sp;
      newNodes.push_back(syn);
    }
  }

  json newConns = json::array();
  for (auto &c : conns) {
    json cj = c;
    std::string to = cj.value("to", std::string());
    std::string from = cj.value("from", std::string());
    if (blend2s.count(to) && maxLayer[to] > 1) {
      int tp = cj.value("to_port", 0);
      int layer = (tp + 1) / 2;
      if (layer >= 2) {
        cj["to"] = to + "__layer" + std::to_string(layer);
        cj["to_port"] = (tp % 2 == 1) ? 1 : 2; // layer input / mask
      }
    }
    // consumers of the blend2 read the end of the chain
    if (blend2s.count(from) && maxLayer[from] > 1)
      cj["from"] = from + "__layer" + std::to_string(maxLayer[from]);
    newConns.push_back(cj);
  }
  for (auto &kv : maxLayer) {
    if (kv.second <= 1)
      continue;
    for (int i = 2; i <= kv.second; i++) {
      newConns.push_back(
          {{"from", i == 2 ? kv.first
                           : kv.first + "__layer" + std::to_string(i - 1)},
           {"from_port", 0},
           {"to", kv.first + "__layer" + std::to_string(i)},
           {"to_port", 0}});
    }
  }
  nodes = newNodes;
  conns = newConns;
}

// ---- MM library graph macros (template expansion) ----
// Macros from MM's node library whose inner nodes convert cleanly are
// expanded in place into inline "graph" nodes; the normal Subgraph
// path then converts them (recursively, so nested macros work too).
// Templates are the verbatim .mmg bodies from
// addons/material_maker/nodes/<name>.mmg, minified.
const std::map<std::string, const char *> &graphMacroTemplates() {
  static const std::map<std::string, const char *> m = {
      {"crystal",
       R"mmg({"connections":[{"from":"voronoi","from_port":0,"to":"math_2","to_port":0},{"from":"voronoi_2","from_port":0,"to":"math_3","to_port":0},{"from":"math","from_port":0,"to":"math_5","to_port":1},{"from":"math_4","from_port":0,"to":"math_5","to_port":0},{"from":"math_5","from_port":0,"to":"math_6","to_port":0},{"from":"math_4","from_port":0,"to":"math_6","to_port":1},{"from":"math_2","from_port":0,"to":"math","to_port":0},{"from":"math_2","from_port":0,"to":"math_4","to_port":0},{"from":"math_3","from_port":0,"to":"math_4","to_port":1},{"from":"math_3","from_port":0,"to":"math","to_port":1},{"from":"math_6","from_port":0,"to":"math_7","to_port":0},{"from":"math_7","from_port":0,"to":"gen_outputs","to_port":0}],"label":"Crystal","name":"crystal","nodes":[{"name":"math","node_position":{"x":-1260,"y":200},"parameters":{"clamp":false,"default_in1":0,"default_in2":0,"op":13},"seed_int":0,"type":"math"},{"name":"math_2","node_position":{"x":-1640,"y":200},"parameters":{"clamp":false,"default_in1":0,"default_in2":0,"op":19},"seed_int":0,"type":"math"},{"name":"voronoi","node_position":{"x":-2000,"y":200},"parameters":{"intensity":1,"randomness":1,"scale_x":16,"scale_y":16,"stretch_x":0.85,"stretch_y":0.85},"seed_int":0,"type":"voronoi"},{"name":"voronoi_2","node_position":{"x":-2000,"y":520},"parameters":{"intensity":1,"randomness":1,"scale_x":16,"scale_y":16,"stretch_x":0.85,"stretch_y":0.85},"seed_int":1998774700,"type":"voronoi"},{"name":"math_3","node_position":{"x":-1640,"y":520},"parameters":{"clamp":false,"default_in1":0,"default_in2":0,"op":19},"seed_int":0,"type":"math"},{"name":"math_4","node_position":{"x":-1260,"y":520},"parameters":{"clamp":false,"default_in1":0,"default_in2":0,"op":14},"seed_int":0,"type":"math"},{"name":"math_5","node_position":{"x":-920,"y":360},"parameters":{"clamp":false,"default_in1":0,"default_in2":0,"op":1},"seed_int":0,"type":"math"},{"name":"math_6","node_position":{"x":-660,"y":360},"parameters":{"clamp":false,"default_in1":0,"default_in2":0,"op":3},"seed_int":0,"type":"math"},{"name":"gen_inputs","node_position":{"x":-2200,"y":400},"parameters":{},"ports":[],"seed_int":0,"type":"ios"},{"name":"gen_outputs","node_position":{"x":-120,"y":360},"parameters":{},"ports":[{"group_size":0,"name":"out","shortdesc":"Output","type":"f"}],"seed_int":0,"type":"ios"},{"name":"gen_parameters","node_position":{"x":-1560,"y":-60},"parameters":{"param0":16,"param1":16},"seed_int":0,"type":"remote","widgets":[{"label":"Scale X","linked_widgets":[{"node":"voronoi","widget":"scale_x"},{"node":"voronoi_2","widget":"scale_x"}],"name":"param0","type":"linked_control"},{"label":"Scale Y","linked_widgets":[{"node":"voronoi","widget":"scale_y"},{"node":"voronoi_2","widget":"scale_y"}],"name":"param1","type":"linked_control"}]},{"name":"math_7","node_position":{"x":-400,"y":360},"parameters":{"clamp":false,"default_in1":0,"default_in2":1.45,"op":2},"seed_int":0,"type":"math"}],"parameters":{"param0":16,"param1":16},"type":"graph"})mmg"},
  };
  return m;
}

void expandGraphMacros(json &nodes) {
  for (auto &n : nodes) {
    const std::string type = n.value("type", std::string());
    auto it = graphMacroTemplates().find(type);
    if (it == graphMacroTemplates().end())
      continue;
    json tmpl = json::parse(it->second, nullptr, false);
    if (tmpl.is_discarded())
      continue;
    // widget values: template defaults overridden by the instance
    json pvals = tmpl.value("parameters", json::object());
    const json ip = n.value("parameters", json::object());
    for (auto it2 = ip.begin(); it2 != ip.end(); ++it2)
      pvals[it2.key()] = it2.value();
    json tnodes = tmpl.value("nodes", json::array());
    // push linked widget values into the inner nodes
    for (auto &gp : tnodes) {
      if (gp.value("type", std::string()) != "remote")
        continue;
      const json widgets = gp.value("widgets", json::array());
      for (auto &w : widgets) {
        const std::string pname = w.value("name", std::string());
        if (!pvals.contains(pname))
          continue;
        const json lws = w.value("linked_widgets", json::array());
        for (auto &lw : lws) {
          const std::string tgt = lw.value("node", std::string());
          const std::string widget = lw.value("widget", std::string());
          for (auto &tn : tnodes)
            if (tn.value("name", std::string()) == tgt)
              tn["parameters"][widget] = pvals[pname];
        }
      }
    }
    // the instance seed offsets every unlocked inner node's own seed
    double iseed = 0.0;
    if (n.contains("seed") && n["seed"].is_number())
      iseed = n["seed"].get<double>();
    else if (n.contains("seed_int") && n["seed_int"].is_number())
      iseed = n["seed_int"].get<double>();
    if (iseed != 0.0)
      for (auto &tn : tnodes) {
        const std::string tt = tn.value("type", std::string());
        if (tt == "ios" || tt == "remote" || tn.value("seed_locked", false))
          continue;
        double s = 0.0;
        if (tn.contains("seed") && tn["seed"].is_number())
          s = tn["seed"].get<double>();
        else if (tn.contains("seed_int") && tn["seed_int"].is_number())
          s = tn["seed_int"].get<double>();
        tn["seed"] = s + iseed;
      }
    n["type"] = "graph";
    n["nodes"] = tnodes;
    n["connections"] = tmpl.value("connections", json::array());
    if (!n.contains("label"))
      n["label"] = tmpl.value("label", type);
  }
}

struct GraphResult {
  json nodes = json::array();
  json conns = json::array();
  std::map<std::string, std::pair<int, std::string>> byName; // -> id, type
  // albedo (or fallback) source for the preview Output
  int previewId = -1;
  std::string previewSlot;
  int previewPrio = -1;
};

json graphToSubgraph(const json &n, const std::string &baseName,
                     std::vector<std::string> *skipped,
                     std::vector<std::string> &inPortNames,
                     std::vector<std::string> &outPortNames);

GraphResult convertGraph(json mmNodes, json mmConns,
                         const std::string &baseName,
                         std::vector<std::string> *skipped) {
  GraphResult res;
  expandGraphMacros(mmNodes);
  collapsePassthroughs(mmNodes, mmConns);
  expandLayeredBlends(mmNodes, mmConns);

  auto &byName = res.byName;
  std::map<std::string,
           std::pair<std::vector<std::string>, std::vector<std::string>>>
      dynPorts; // graph nodes: in/out port names
  int nextId = 0;

  for (auto &n : mmNodes) {
    std::string type = n.value("type", std::string());
    std::string name = n.value("name", std::string());
    json pos = n.value("node_position", json::object());
    json params = n.value("parameters", json::object());
    // MM stores the instance seed at node level, not in parameters
    if (!params.contains("seed")) {
      if (n.contains("seed"))
        params["seed"] = n["seed"];
      else if (n.contains("seed_int"))
        params["seed"] = n["seed_int"];
    }

    std::string typeName;
    json p;
    if (isIgnorable(type))
      continue;
    if (type == "comment") {
      json col = n.value("color", json::object());
      typeName = "Comment";
      p = {{"text", n.value("text", std::string())},
           {"color",
            {numOr(col, "r", 0.3f), numOr(col, "g", 0.3f),
             numOr(col, "b", 0.3f)}}};
    } else if (type == "graph") {
      std::vector<std::string> ins, outs;
      p = graphToSubgraph(n, baseName, skipped, ins, outs);
      typeName = "Subgraph";
      dynPorts[name] = {ins, outs};
    } else if (!convertParams(type, params, baseName, typeName, p)) {
      if (skipped)
        skipped->push_back(type);
      continue;
    }

    int id = nextId++;
    byName[name] = {id, type};
    res.nodes.push_back({{"id", id},
                         {"typeName", typeName},
                         {"posX", numOr(pos, "x", 0.0f)},
                         {"posY", numOr(pos, "y", 0.0f)},
                         {"params", p}});
  }

  for (auto &c : mmConns) {
    auto srcIt = byName.find(c.value("from", std::string()));
    auto dstIt = byName.find(c.value("to", std::string()));
    if (srcIt == byName.end() || dstIt == byName.end())
      continue;
    auto [fromId, fromType] = srcIt->second;
    auto [toId, toType] = dstIt->second;
    int fp = c.value("from_port", 0);
    int tp = c.value("to_port", 0);

    std::vector<std::string> outs = {"Out"};
    if (fromType == "graph")
      outs = dynPorts[c.value("from", std::string())].second;
    else if (portsOut().count(fromType))
      outs = portsOut().at(fromType);

    std::vector<std::string> ins;
    if (toType == "graph")
      ins = dynPorts[c.value("to", std::string())].first;
    else if (portsIn().count(toType))
      ins = portsIn().at(toType);

    if (fp >= (int)outs.size() || outs[fp].empty() ||
        tp >= (int)ins.size() || ins[tp].empty())
      continue; // unmapped port
    res.conns.push_back({{"fromId", fromId},
                         {"fromSlot", outs[fp]},
                         {"toId", toId},
                         {"toSlot", ins[tp]}});
    if (toType == "material" || toType == "material_tesselated" ||
        toType == "material_unlit") {
      static const std::map<std::string, int> prio = {
          {"Albedo", 3}, {"Height", 2}, {"Normal", 1}};
      auto it = prio.find(ins[tp]);
      int pr = it != prio.end() ? it->second : 0;
      if (pr > res.previewPrio) {
        res.previewPrio = pr;
        res.previewId = fromId;
        res.previewSlot = outs[fp];
      }
    }
  }

  // Prune recently-added filter types that lost every incoming
  // connection to unsupported sources: a converted filter with no
  // input feeds black downstream, while a missing node lets consumers
  // degrade gracefully (e.g. a Blend without mask). Restricted to
  // these types to keep the long-standing behavior of the others;
  // nodes with no connections in the original MM graph are kept —
  // they legitimately run on their defaults (e.g. math).
  {
    static const std::set<std::string> prunable = {
        "scale",        "translate",
        "tiler",        "splatter",
        "math",         "slope_blur",
        "multi_warp",   "fill",
        "fill2",        "fill_to_uv",
        "fill_to_uv2",  "fill_to_random_grey",
        "fill_to_random_grey2", "fill_to_random_color",
        "fill_to_random_color2", "fill_to_random_color3",
        "fill_to_color", "fill_to_color2",
        "remap", "tile2x2", "normal_map_convert", "custom_uv",
        "smooth_curvature", "smooth_curvature2", "occlusion2", "hbao",
        "rotate", "tones_range", "math_v3", "tiler_advanced",
        "height_to_offset", "bevel", "dilate", "normal_blend",
        "directional_blur", "occlusion", "edge_detect_2",
        "fill_to_gradient", "fill_to_gradient2", "fill_to_size2"};
    std::set<std::string> hadInput;
    for (auto &c : mmConns)
      hadInput.insert(c.value("to", std::string()));
    std::set<int> hadInputIds;
    for (auto &kv : byName)
      if (hadInput.count(kv.first) && prunable.count(kv.second.second))
        hadInputIds.insert(kv.second.first);

    for (;;) {
      std::set<int> hasIncoming;
      for (auto &c : res.conns)
        hasIncoming.insert(c["toId"].get<int>());
      std::set<int> pruned;
      json keptNodes = json::array();
      for (auto &n : res.nodes) {
        int id = n["id"].get<int>();
        if (hadInputIds.count(id) && !hasIncoming.count(id))
          pruned.insert(id);
        else
          keptNodes.push_back(n);
      }
      if (pruned.empty())
        break;
      res.nodes = keptNodes;
      json keptConns = json::array();
      for (auto &c : res.conns)
        if (!pruned.count(c["fromId"].get<int>()) &&
            !pruned.count(c["toId"].get<int>()))
          keptConns.push_back(c);
      res.conns = keptConns;
      if (pruned.count(res.previewId))
        res.previewId = -1;
    }
  }
  return res;
}

json graphToSubgraph(const json &n, const std::string &baseName,
                     std::vector<std::string> *skipped,
                     std::vector<std::string> &inPortNames,
                     std::vector<std::string> &outPortNames) {
  json innerNodes = json::array();
  json genIn, genOut;
  for (auto &x : n.value("nodes", json::array())) {
    std::string nm = x.value("name", std::string());
    if (nm == "gen_inputs")
      genIn = x;
    else if (nm == "gen_outputs")
      genOut = x;
    else if (x.value("type", std::string()) != "ios")
      innerNodes.push_back(x);
  }
  int k = 0;
  for (auto &pt : genIn.value("ports", json::array()))
    inPortNames.push_back(pt.value("name", "in" + std::to_string(k++)));
  k = 0;
  for (auto &pt : genOut.value("ports", json::array()))
    outPortNames.push_back(pt.value("name", "out" + std::to_string(k++)));

  json innerConns = json::array();
  json boundary = json::array();
  for (auto &c : n.value("connections", json::array())) {
    std::string from = c.value("from", std::string());
    std::string to = c.value("to", std::string());
    if (from == "gen_inputs" || to == "gen_outputs")
      boundary.push_back(c);
    else
      innerConns.push_back(c);
  }

  GraphResult inner = convertGraph(innerNodes, innerConns, baseName, skipped);
  std::map<std::string, int> nameToId;
  {
    // byName may still reference nodes pruned by convertGraph
    std::set<int> kept;
    for (auto &nj : inner.nodes)
      kept.insert(nj["id"].get<int>());
    for (auto &kv : inner.byName)
      if (kept.count(kv.second.first))
        nameToId[kv.first] = kv.second.first;
  }

  json inputs = json::array();
  for (auto &pn : inPortNames)
    inputs.push_back({{"name", pn}, {"targets", json::array()}});
  json outputs = json::array();
  for (auto &pn : outPortNames)
    outputs.push_back({{"name", pn}, {"id", -1}, {"slot", "Out"}});

  for (auto &c : boundary) {
    if (c.value("from", std::string()) == "gen_inputs") {
      int kp = c.value("from_port", 0);
      auto dst = nameToId.find(c.value("to", std::string()));
      if (kp >= (int)inputs.size() || dst == nameToId.end())
        continue;
      std::string toType;
      for (auto &x : innerNodes)
        if (x.value("name", std::string()) == c.value("to", std::string()))
          toType = x.value("type", std::string());
      std::vector<std::string> ins;
      if (portsIn().count(toType))
        ins = portsIn().at(toType);
      int tp = c.value("to_port", 0);
      if (tp < (int)ins.size() && !ins[tp].empty())
        inputs[kp]["targets"].push_back({dst->second, ins[tp]});
    } else if (c.value("to", std::string()) == "gen_outputs") {
      int kp = c.value("to_port", 0);
      auto src = nameToId.find(c.value("from", std::string()));
      if (kp >= (int)outputs.size() || src == nameToId.end())
        continue;
      std::string fromType;
      for (auto &x : innerNodes)
        if (x.value("name", std::string()) == c.value("from", std::string()))
          fromType = x.value("type", std::string());
      std::vector<std::string> outs = {"Out"};
      if (portsOut().count(fromType))
        outs = portsOut().at(fromType);
      int fp = c.value("from_port", 0);
      if (fp < (int)outs.size() && !outs[fp].empty()) {
        outputs[kp]["id"] = src->second;
        outputs[kp]["slot"] = outs[fp];
      }
    }
  }

  return {{"title", n.value("label", std::string("Subgraph"))},
          {"graph", {{"nodes", inner.nodes}, {"connections", inner.conns}}},
          {"inputs", inputs},
          {"outputs", outputs}};
}

} // namespace

json ptexToTexgen(const json &ptex, const std::string &baseName,
                  std::vector<std::string> *skippedTypes) {
  GraphResult res =
      convertGraph(ptex.value("nodes", json::array()),
                   ptex.value("connections", json::array()), baseName,
                   skippedTypes);

  // Prefer the Material node's lit preview over the raw albedo source
  for (auto &n : res.nodes) {
    if (n["typeName"] == "Material") {
      res.previewId = n["id"].get<int>();
      res.previewSlot = "Preview";
      break;
    }
  }

  if (res.previewId >= 0) {
    int outId = 0;
    for (auto &n : res.nodes)
      outId = std::max(outId, n["id"].get<int>() + 1);
    res.nodes.push_back(
        {{"id", outId},
         {"typeName", "Output"},
         {"posX", 1200.0f},
         {"posY", 0.0f},
         {"params", {{"filename", baseName + "_preview.png"}}}});
    res.conns.push_back({{"fromId", res.previewId},
                         {"fromSlot", res.previewSlot},
                         {"toId", outId},
                         {"toSlot", "In"}});
  }

  return {{"nodes", res.nodes}, {"connections", res.conns}};
}
