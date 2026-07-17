#include "CExport.h"
#include "CoreNodeRegistry.h"
#include "GraphEval.h"
#ifndef TEXGEN_NO_UI
#include "Nodes.h"
#endif

#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Format a float as a C literal (always has decimal point + 'f' suffix).
static std::string fmtf(float v) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.6gf", (double)v);
  std::string s(buf);
  size_t fpos = s.rfind('f');
  if (fpos != std::string::npos && fpos > 0) {
    bool hasDot = false;
    for (size_t i = 0; i < fpos; i++)
      if (s[i] == '.' || s[i] == 'e' || s[i] == 'E') {
        hasDot = true;
        break;
      }
    if (!hasDot)
      s.insert(fpos, ".0");
  }
  return s;
}

// Get a float from JSON params and format as C literal.
static std::string pf(const nlohmann::json& p, const char* key, float def) {
  return fmtf(p.value(key, def));
}

// Convert a float[4] color (from JSON) to a hex 0xAARRGGBB literal.
static std::string colorHex(float r, float g, float b, float a) {
  char buf[16];
  unsigned ar = (unsigned)(r * 255.0f + 0.5f);
  unsigned ag = (unsigned)(g * 255.0f + 0.5f);
  unsigned ab = (unsigned)(b * 255.0f + 0.5f);
  unsigned aa = (unsigned)(a * 255.0f + 0.5f);
  snprintf(buf, sizeof(buf), "0x%02x%02x%02x%02x", aa, ar, ag, ab);
  return buf;
}

static std::string colorHex(const nlohmann::json& j,
                            const char* r,
                            const char* g,
                            const char* b,
                            const char* a) {
  return colorHex(j.value(r, 0.0f), j.value(g, 0.0f), j.value(b, 0.0f),
                  j.value(a, 1.0f));
}

static std::string colorHexArr(const nlohmann::json& arr) {
  if (!arr.is_array() || arr.size() < 4)
    return "0xff000000";
  return colorHex(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>(),
                  arr[3].get<float>());
}

static int sizeFromIdx(int idx) {
  return 32 << idx;
}

static std::string var(int id) {
  return "n" + std::to_string(id);
}

// Output slot names per node id (filled at export start from the registry).
// Multi-output nodes get one variable per slot: n<id>_<slot>.
static std::map<int, std::vector<std::string>> g_nodeOutputs;

static std::string outVar(int srcId, const std::string& fromSlot) {
  auto it = g_nodeOutputs.find(srcId);
  if (it != g_nodeOutputs.end() && it->second.size() > 1)
    return var(srcId) + "_" + fromSlot;
  return var(srcId);
}

// Variable name of the source output feeding (nodeId, slot), "" if none.
static std::string srcVar(const nlohmann::json& connections,
                          int nodeId,
                          const std::string& slotName) {
  for (auto& c : connections) {
    if (c["toId"].get<int>() == nodeId &&
        c["toSlot"].get<std::string>() == slotName) {
      return outVar(c["fromId"].get<int>(), c["fromSlot"].get<std::string>());
    }
  }
  return "";
}

// Find which node output connects to a given input slot.
// Returns the node id of the source, or -1 if not connected.
static int findInputSource(const nlohmann::json& connections,
                           int nodeId,
                           const std::string& slotName) {
  for (auto& c : connections) {
    if (c["toId"].get<int>() == nodeId &&
        c["toSlot"].get<std::string>() == slotName) {
      return c["fromId"].get<int>();
    }
  }
  return -1;
}

static std::string inputRef(const nlohmann::json& connections,
                            int nodeId,
                            const std::string& slotName) {
  std::string s = srcVar(connections, nodeId, slotName);
  if (!s.empty())
    return "&" + s;
  return "NULL";
}

// Emit code for one node. Returns the lines to append.
static void emitNode(std::ostringstream& ss,
                     const std::string& type,
                     int id,
                     const nlohmann::json& p,
                     const nlohmann::json& conns) {
  std::string v = var(id);

  if (type == "Color") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string col = colorHex(p, "r", "g", "b", "a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    { sU32 c = " << col << "; for(int i=0; i<" << v
       << ".NPixels; i++) " << v << ".Data[i].Init(c); }\n";
  }

  else if (type == "Noise") {
    int sz = sizeFromIdx(p.value("sizeIdx", 3));
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << sz << ", " << sz << ");\n";
    ss << "    { GenTexture g = LinearGradient(" << c1 << ", " << c2 << ");\n";
    ss << "      " << v << ".Noise(g, " << p.value("freqX", 2) << ", "
       << p.value("freqY", 2) << ", " << p.value("oct", 4) << ", "
       << pf(p, "fadeoff", 0.5f) << ", " << p.value("seed", 0) << ", "
       << p.value("mode", 0) << "); }\n";
  }

  else if (type == "Cells") {
    int sz = sizeFromIdx(p.value("sizeIdx", 3));
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << sz << ", " << sz << ");\n";
    ss << "    { GenTexture g = LinearGradient(" << c1 << ", " << c2 << ");\n";
    ss << "      CellCenter centers[256];\n";
    ss << "      srand(" << p.value("seed", 0) << ");\n";
    ss << "      for(int i=0; i<" << p.value("nCenters", 16)
       << "; i++) { centers[i].x=rand()/(float)RAND_MAX; "
          "centers[i].y=rand()/(float)RAND_MAX; "
          "centers[i].color.Init(rand()%256,rand()%256,rand()%256,255); }\n";
    ss << "      " << v << ".Cells(g, centers, " << p.value("nCenters", 16)
       << ", " << pf(p, "amp", 0.0f) << ", " << p.value("mode", 0) << "); }\n";
  }

  else if (type == "Crystal") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string cn, cf;
    if (p.contains("colorNear") && p["colorNear"].is_array())
      cn = colorHexArr(p["colorNear"]);
    else
      cn = "0xffffffff";
    if (p.contains("colorFar") && p["colorFar"].is_array())
      cf = colorHexArr(p["colorFar"]);
    else
      cf = "0xff000000";
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    Crystal(" << v << ", " << p.value("seed", 0) << ", "
       << p.value("count", 16) << ", " << cn << ", " << cf << ");\n";
  }

  else if (type == "Bricks") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string c0 = colorHex(p, "col0r", "col0g", "col0b", "col0a");
    std::string c1 = colorHex(p, "col1r", "col1g", "col1b", "col1a");
    std::string cf =
        colorHex(p, "colFuger", "colFugeg", "colFugeb", "colFugea");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    Bricks(" << v << ", " << c0 << ", " << c1 << ", " << cf << ", "
       << pf(p, "fugeX", 0.03f) << ", " << pf(p, "fugeY", 0.06f) << ", "
       << p.value("tileX", 4) << ", " << p.value("tileY", 8) << ", "
       << p.value("seed", 0) << ", " << p.value("heads", 2) << ", "
       << pf(p, "colorBalance", 0.5f) << ");\n";
  }

  else if (type == "DirectionalGradient") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    DirectionalGradient(" << v << ", " << pf(p, "x1", 0.0f) << ", "
       << pf(p, "y1", 0.0f) << ", " << pf(p, "x2", 1.0f) << ", "
       << pf(p, "y2", 1.0f) << ", " << c1 << ", " << c2 << ");\n";
  }

  else if (type == "GlowEffect") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string bg =
        colorHexArr(p.value("bgCol", nlohmann::json::array({0, 0, 0, 1})));
    std::string gl =
        colorHexArr(p.value("glowCol", nlohmann::json::array({1, 1, 1, 1})));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    GlowEffect(" << v << ", " << pf(p, "cx", 0.5f) << ", "
       << pf(p, "cy", 0.5f) << ", " << pf(p, "scale", 1.0f) << ", "
       << pf(p, "exponent", 2.0f) << ", " << pf(p, "intensity", 1.0f) << ", "
       << bg << ", " << gl << ");\n";
  }

  else if (type == "PerlinNoiseRG2") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    PerlinNoiseRG2(" << v << ", " << p.value("octaves", 4) << ", "
       << pf(p, "persistence", 0.5f) << ", " << p.value("freqScale", 2) << ", "
       << p.value("seed", 0) << ", " << pf(p, "contrast", 1.0f) << ", " << c1
       << ", " << c2 << ", " << p.value("startOctave", 0) << ");\n";
  }

  else if (type == "Gradient") {
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << " = LinearGradient(" << c1 << ", " << c2
       << ");\n";
  }

  else if (type == "Blur") {
    int src = findInputSource(conns, id, "In");
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0) {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    " << v << ".Blur(" << in << ", " << pf(p, "sizex", 0.01f)
         << ", " << pf(p, "sizey", 0.01f) << ", " << p.value("order", 2) << ", "
         << p.value("mode", 0) << ");\n";
    } else {
      ss << "    " << v << ".Init(256, 256);\n";
    }
  }

  else if (type == "BlurKernel") {
    int src = findInputSource(conns, id, "In");
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0) {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    BlurKernel(" << v << ", " << in << ", "
         << pf(p, "radiusX", 0.01f) << ", " << pf(p, "radiusY", 0.01f) << ", "
         << p.value("kernelType", 2) << ", " << p.value("wrapMode", 0)
         << ");\n";
    } else {
      ss << "    " << v << ".Init(256, 256);\n";
    }
  }

  else if (type == "HSCB") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? srcVar(conns, id, "In") : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    ss << "    HSCB(" << v << ", " << in << ", " << pf(p, "hue", 0.0f) << ", "
       << pf(p, "sat", 1.0f) << ", " << pf(p, "contrast", 1.0f) << ", "
       << pf(p, "brightness", 1.0f) << ");\n";
  }

  else if (type == "Derive") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? srcVar(conns, id, "In") : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    ss << "    " << v << ".Derive(" << in << ", " << p.value("op", 0) << ", "
       << pf(p, "strength", 1.0f) << ");\n";
  }

  else if (type == "ColorMatrix") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? srcVar(conns, id, "In") : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    ss << "    { Matrix44 m = {";
    for (int i = 0; i < 16; i++) {
      std::string key = "m" + std::to_string(i);
      // matrix stored as flat array in params as m0..m15 or matrix[0..15]
      float val = 0;
      if (p.contains(key))
        val = p[key].get<float>();
      else if (p.contains("matrix") && p["matrix"].is_array() &&
               i < (int)p["matrix"].size())
        val = p["matrix"][i].get<float>();
      ss << fmtf(val) << "";
      if (i < 15)
        ss << ", ";
    }
    ss << "};\n";
    ss << "      " << v << ".ColorMatrixTransform(" << in << ", m, "
       << (p.value("clampPremult", true) ? "sTRUE" : "sFALSE") << "); }\n";
  }

  else if (type == "LinearCombine") {
    ss << "    GenTexture " << v << ";\n";
    // Find first connected input for size
    int src1 = findInputSource(conns, id, "Image1");
    if (src1 >= 0)
      ss << "    " << v << ".Init(" << srcVar(conns, id, "Image1") << ".XRes, "
         << srcVar(conns, id, "Image1") << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";

    // Emit LinearCombine call
    ss << "    { Pixel cp; cp.Init("
       << colorHexArr(
              p.value("constColor", nlohmann::json::array({0, 0, 0, 1})))
       << ");\n";
    ss << "      LinearInput li[4];\n";
    auto weights = p.value("weights", nlohmann::json::array({1, 0, 0, 0}));
    auto uShift = p.value("uShift", nlohmann::json::array({0, 0, 0, 0}));
    auto vShift = p.value("vShift", nlohmann::json::array({0, 0, 0, 0}));
    auto fMode = p.value("filterMode", nlohmann::json::array({0, 0, 0, 0}));
    std::string slotNames[] = {"Image1", "Image2", "Image3", "Image4"};
    int count = 0;
    for (int i = 0; i < 4; i++) {
      int s = findInputSource(conns, id, slotNames[i]);
      if (s >= 0) {
        ss << "      li[" << i << "].Tex = &" << srcVar(conns, id, slotNames[i])
           << "; li[" << i << "].Weight = " << fmtf(weights[i].get<float>())
           << "; li[" << i << "].UShift = " << fmtf(uShift[i].get<float>())
           << "; li[" << i << "].VShift = " << fmtf(vShift[i].get<float>())
           << "; li[" << i << "].FilterMode = " << fMode[i].get<int>() << ";\n";
        count = i + 1;
      }
    }
    ss << "      " << v << ".LinearCombine(cp, " << pf(p, "constWeight", 0.0f)
       << ", li, " << count << "); }\n";
  }

  else if (type == "Paste") {
    int bgSrc = findInputSource(conns, id, "Background");
    int snSrc = findInputSource(conns, id, "Snippet");
    ss << "    GenTexture " << v << ";\n";
    if (bgSrc >= 0)
      ss << "    " << v << ".Init(" << srcVar(conns, id, "Background")
         << ".XRes, " << srcVar(conns, id, "Background") << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    std::string bg = (bgSrc >= 0) ? srcVar(conns, id, "Background") : v;
    std::string sn = (snSrc >= 0) ? srcVar(conns, id, "Snippet") : v;
    ss << "    " << v << ".Paste(" << bg << ", " << sn << ", "
       << pf(p, "orgx", 0.0f) << ", " << pf(p, "orgy", 0.0f) << ", "
       << pf(p, "ux", 1.0f) << ", " << pf(p, "uy", 0.0f) << ", "
       << pf(p, "vx", 0.0f) << ", " << pf(p, "vy", 1.0f) << ", "
       << p.value("op", 0) << ", " << p.value("mode", 0) << ");\n";
  }

  else if (type == "Ternary") {
    int s1 = findInputSource(conns, id, "Image1");
    int s2 = findInputSource(conns, id, "Image2");
    int sm = findInputSource(conns, id, "Mask");
    ss << "    GenTexture " << v << ";\n";
    if (s1 >= 0)
      ss << "    " << v << ".Init(" << srcVar(conns, id, "Image1") << ".XRes, "
         << srcVar(conns, id, "Image1") << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    std::string i1 = (s1 >= 0) ? srcVar(conns, id, "Image1") : v;
    std::string i2 = (s2 >= 0) ? srcVar(conns, id, "Image2") : v;
    std::string mask = (sm >= 0) ? srcVar(conns, id, "Mask") : v;
    ss << "    " << v << ".Ternary(" << i1 << ", " << i2 << ", " << mask << ", "
       << p.value("op", 0) << ");\n";
  }

  else if (type == "Wavelet") {
    int src = findInputSource(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << " = " << srcVar(conns, id, "In") << ";\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    ss << "    Wavelet(" << v << ", " << p.value("mode", 0) << ", "
       << p.value("count", 1) << ");\n";
  }

  else if (type == "ColorBalance") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? srcVar(conns, id, "In") : v;
    auto sh = p.value("shadow", nlohmann::json::array({0, 0, 0}));
    auto md = p.value("mid", nlohmann::json::array({0, 0, 0}));
    auto hl = p.value("highlight", nlohmann::json::array({0, 0, 0}));
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    ColorBalance(" << v << ", " << in << ", "
       << fmtf(sh[0].get<float>()) << ", " << fmtf(sh[1].get<float>()) << ", "
       << fmtf(sh[2].get<float>()) << ", " << fmtf(md[0].get<float>()) << ", "
       << fmtf(md[1].get<float>()) << ", " << fmtf(md[2].get<float>()) << ", "
       << fmtf(hl[0].get<float>()) << ", " << fmtf(hl[1].get<float>()) << ", "
       << fmtf(hl[2].get<float>()) << ");\n";
  }

  else if (type == "Output") {
    int src = findInputSource(conns, id, "In");
    if (src >= 0)
      ss << "    *out = " << srcVar(conns, id, "In") << ";\n";
  }

  // Skip Comment/Image (not exportable) and Remote (already baked into
  // target params by applyRemotes before emission)
  else if (type == "Comment" || type == "Image" || type == "Remote") {
    ss << "    // [" << type << " node " << id << " skipped]\n";
  }

  // AGG nodes — emit inline AGG code
  else if (type == "AggLine" || type == "AggCircle" || type == "AggRect" ||
           type == "AggPolygon" || type == "AggText" || type == "AggArc" ||
           type == "AggBezier" || type == "AggDashLine" ||
           type == "AggGradient") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    int bgSrc = findInputSource(conns, id, "Bg");

    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    if (bgSrc >= 0) {
      std::string bgv = srcVar(conns, id, "Bg");
      ss << "    if(" << bgv << ".Data && " << bgv << ".XRes==" << w << " && "
         << bgv << ".YRes==" << h << ")\n";
      ss << "      memcpy(" << v << ".Data, " << bgv << ".Data, " << w << "*"
         << h << "*sizeof(Pixel));\n";
    }
    ss << "    { agg::rendering_buffer rbuf = agg_rbuf_from(" << v << ");\n";
    ss << "      AggPixfmt pixf(rbuf); AggRendererBase ren(pixf);\n";

    if (type == "AggLine") {
      ss << "      agg::path_storage path;\n";
      ss << "      path.move_to(" << pf(p, "x1", 0.2f) << "*" << w << ", "
         << pf(p, "y1", 0.2f) << "*" << h << ");\n";
      ss << "      path.line_to(" << pf(p, "x2", 0.8f) << "*" << w << ", "
         << pf(p, "y2", 0.8f) << "*" << h << ");\n";
      ss << "      agg::conv_stroke<agg::path_storage> stroke(path);\n";
      ss << "      stroke.width(" << p.value("thickness", 3.0f) << ");\n";
      ss << "      stroke.line_cap(agg::round_cap); "
            "stroke.line_join(agg::round_join);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl;\n";
      ss << "      AggRendererSolid solid(ren); ras.add_path(stroke);\n";
      std::string col = "{" + pf(p, "cr", 1.0f) + "," + pf(p, "cg", 1.0f) +
                        "," + pf(p, "cb", 1.0f) + "," + pf(p, "ca", 1.0f) + "}";
      ss << "      float col[] = " << col << ";\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    else if (type == "AggCircle") {
      ss << "      agg::ellipse ell(" << pf(p, "cx", 0.5f) << "*" << w << ", "
         << "aggY(" << pf(p, "cy", 0.5f) << "," << h << "), "
         << pf(p, "rx", 0.3f) << "*" << w << ", " << pf(p, "ry", 0.3f) << "*"
         << h << ", 100);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      if (p.value("filled", true)) {
        ss << "      ras.add_path(ell); float fc[] = {" << pf(p, "fr", 1.0f)
           << "," << pf(p, "fg", 1.0f) << "," << pf(p, "fb", 1.0f) << ","
           << pf(p, "fa", 1.0f) << "};\n";
        ss << "      solid.color(agg_color(fc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
      if (p.value("stroked", false)) {
        ss << "      ras.reset(); agg::conv_stroke<agg::ellipse> "
              "stroke(ell);\n";
        ss << "      stroke.width(" << p.value("thickness", 3.0f) << ");\n";
        ss << "      ras.add_path(stroke); float sc[] = {" << pf(p, "sr", 1.0f)
           << "," << pf(p, "sg", 1.0f) << "," << pf(p, "sb", 1.0f) << ","
           << pf(p, "sa", 1.0f) << "};\n";
        ss << "      solid.color(agg_color(sc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
    }

    else if (type == "AggRect") {
      ss << "      agg::rounded_rect rect(" << pf(p, "x1", 0.1f) << "*" << w
         << ", " << "aggY(" << pf(p, "y1", 0.1f) << "," << h << "), "
         << pf(p, "x2", 0.9f) << "*" << w << ", " << pf(p, "y2", 0.9f) << "*"
         << h << ", " << pf(p, "cornerRadius", 0.0f) << "*" << std::min(w, h)
         << "*0.5f);\n";
      ss << "      rect.normalize_radius();\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      if (p.value("filled", true)) {
        ss << "      ras.add_path(rect); float fc[] = {" << pf(p, "fr", 1.0f)
           << "," << pf(p, "fg", 1.0f) << "," << pf(p, "fb", 1.0f) << ","
           << pf(p, "fa", 1.0f) << "};\n";
        ss << "      solid.color(agg_color(fc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
      if (p.value("stroked", false)) {
        ss << "      ras.reset(); agg::conv_stroke<agg::rounded_rect> "
              "stroke(rect);\n";
        ss << "      stroke.width(" << p.value("thickness", 3.0f) << ");\n";
        ss << "      ras.add_path(stroke); float sc[] = {" << pf(p, "sr", 1.0f)
           << "," << pf(p, "sg", 1.0f) << "," << pf(p, "sb", 1.0f) << ","
           << pf(p, "sa", 1.0f) << "};\n";
        ss << "      solid.color(agg_color(sc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
    }

    else if (type == "AggPolygon") {
      int sides = p.value("sides", 6);
      float innerR = p.value("innerRadius", 1.0f);
      bool isStar = innerR < 0.999f;
      int n = isStar ? sides * 2 : sides;
      ss << "      agg::path_storage poly;\n";
      ss << "      { double cx=" << p.value("cx", 0.5f) << "*" << w
         << ", cy=" << p.value("cy", 0.5f) << "*" << h << ";\n";
      ss << "        double r=" << p.value("radius", 0.4f) << "*"
         << std::min(w, h) << "*0.5, ri=r*" << innerR << ";\n";
      ss << "        double rot=" << p.value("rotation", 0.0f)
         << "*3.14159265/180.0;\n";
      ss << "        for(int i=0; i<" << n << "; i++) {\n";
      ss << "          double a=rot+(6.28318530*i)/" << n << "-1.5707963;\n";
      if (isStar)
        ss << "          double rd=(i&1)?ri:r;\n";
      else
        ss << "          double rd=r;\n";
      ss << "          if(i==0) poly.move_to(cx+cos(a)*rd, cy+sin(a)*rd);\n";
      ss << "          else poly.line_to(cx+cos(a)*rd, cy+sin(a)*rd); }\n";
      ss << "        poly.close_polygon(); }\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      if (p.value("filled", true)) {
        ss << "      ras.add_path(poly); float fc[] = {" << pf(p, "fr", 1.0f)
           << "," << pf(p, "fg", 1.0f) << "," << pf(p, "fb", 1.0f) << ","
           << pf(p, "fa", 1.0f) << "};\n";
        ss << "      solid.color(agg_color(fc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
      if (p.value("stroked", false)) {
        ss << "      ras.reset(); agg::conv_stroke<agg::path_storage> "
              "stroke(poly);\n";
        ss << "      stroke.width(" << p.value("thickness", 3.0f)
           << "); stroke.line_cap(agg::round_cap);\n";
        ss << "      ras.add_path(stroke); float sc[] = {" << pf(p, "sr", 1.0f)
           << "," << pf(p, "sg", 1.0f) << "," << pf(p, "sb", 1.0f) << ","
           << pf(p, "sa", 1.0f) << "};\n";
        ss << "      solid.color(agg_color(sc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
    }

    else if (type == "AggText") {
      std::string text = p.value("text", "AGG");
      ss << "      agg::gsv_text text; text.size(" << p.value("height", 30.0f)
         << ");\n";
      ss << "      text.start_point(" << pf(p, "x", 0.1f) << "*" << w << ", "
         << pf(p, "y", 0.5f) << "*" << h << ");\n";
      ss << "      text.text(\"" << text << "\");\n";
      ss << "      agg::conv_stroke<agg::gsv_text> stroke(text);\n";
      ss << "      stroke.width(" << p.value("thickness", 1.5f)
         << "); stroke.line_cap(agg::round_cap);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      ss << "      ras.add_path(stroke); float col[] = {" << pf(p, "cr", 1.0f)
         << "," << pf(p, "cg", 1.0f) << "," << pf(p, "cb", 1.0f) << ","
         << pf(p, "ca", 1.0f) << "};\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    else if (type == "AggArc") {
      ss << "      agg::arc a(" << pf(p, "cx", 0.5f) << "*" << w << ", aggY("
         << pf(p, "cy", 0.5f) << "," << h << "), " << pf(p, "rx", 0.3f) << "*"
         << w << ", " << pf(p, "ry", 0.3f) << "*" << h << ", "
         << pf(p, "angle1", 0.0f) << "*3.14159265/180.0, "
         << pf(p, "angle2", 270.0f) << "*3.14159265/180.0, true);\n";
      ss << "      agg::conv_stroke<agg::arc> stroke(a);\n";
      ss << "      stroke.width(" << pf(p, "thickness", 3.0f) << ");\n";
      ss << "      stroke.line_cap(agg::round_cap); "
            "stroke.line_join(agg::round_join);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      ss << "      ras.add_path(stroke); float col[] = {" << pf(p, "cr", 1.0f)
         << "," << pf(p, "cg", 1.0f) << "," << pf(p, "cb", 1.0f) << ","
         << pf(p, "ca", 1.0f) << "};\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    else if (type == "AggBezier") {
      ss << "      agg::curve4 curve(" << pf(p, "x1", 0.1f) << "*" << w
         << ", aggY(" << pf(p, "y1", 0.5f) << "," << h << "), "
         << pf(p, "cx1", 0.3f) << "*" << w << ", aggY(" << pf(p, "cy1", 0.1f)
         << "," << h << "), " << pf(p, "cx2", 0.7f) << "*" << w << ", aggY("
         << pf(p, "cy2", 0.9f) << "," << h << "), " << pf(p, "x2", 0.9f) << "*"
         << w << ", aggY(" << pf(p, "y2", 0.5f) << "," << h << "));\n";
      ss << "      agg::conv_stroke<agg::curve4> stroke(curve);\n";
      ss << "      stroke.width(" << pf(p, "thickness", 3.0f) << ");\n";
      ss << "      stroke.line_cap(agg::round_cap); "
            "stroke.line_join(agg::round_join);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      ss << "      ras.add_path(stroke); float col[] = {" << pf(p, "cr", 1.0f)
         << "," << pf(p, "cg", 1.0f) << "," << pf(p, "cb", 1.0f) << ","
         << pf(p, "ca", 1.0f) << "};\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    else if (type == "AggDashLine") {
      ss << "      agg::path_storage lp;\n";
      ss << "      lp.move_to(" << pf(p, "x1", 0.1f) << "*" << w << ", aggY("
         << pf(p, "y1", 0.5f) << "," << h << "));\n";
      ss << "      lp.line_to(" << pf(p, "x2", 0.9f) << "*" << w << ", aggY("
         << pf(p, "y2", 0.5f) << "," << h << "));\n";
      ss << "      agg::conv_dash<agg::path_storage> dash(lp);\n";
      ss << "      dash.add_dash(" << pf(p, "dashLen", 15.0f) << ", "
         << pf(p, "gapLen", 10.0f) << ");\n";
      ss << "      agg::conv_stroke<agg::conv_dash<agg::path_storage>> "
            "stroke(dash);\n";
      ss << "      stroke.width(" << pf(p, "thickness", 3.0f)
         << "); stroke.line_cap(agg::round_cap);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      ss << "      ras.add_path(stroke); float col[] = {" << pf(p, "cr", 1.0f)
         << "," << pf(p, "cg", 1.0f) << "," << pf(p, "cb", 1.0f) << ","
         << pf(p, "ca", 1.0f) << "};\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    else if (type == "AggGradient") {
      // Gradient fill — emit simplified per-pixel loop (avoids complex span
      // generator templates in header)
      int gtype = p.value("type", 0);
      ss << "      // Gradient fill (" << (gtype == 0 ? "linear" : "radial")
         << ")\n";
      ss << "      { float c1[] = {" << pf(p, "c1r", 0.0f) << ","
         << pf(p, "c1g", 0.0f) << "," << pf(p, "c1b", 0.0f) << ","
         << pf(p, "c1a", 1.0f) << "};\n";
      ss << "        float c2[] = {" << pf(p, "c2r", 1.0f) << ","
         << pf(p, "c2g", 1.0f) << "," << pf(p, "c2b", 1.0f) << ","
         << pf(p, "c2a", 1.0f) << "};\n";
      ss << "        agg::rgba16 ac1 = agg_color(c1), ac2 = agg_color(c2);\n";
      if (gtype == 0) {
        ss << "        double gx1=" << pf(p, "x1", 0.0f) << "*" << w
           << ", gy1=aggY(" << pf(p, "y1", 0.5f) << "," << h << ");\n";
        ss << "        double gx2=" << pf(p, "x2", 1.0f) << "*" << w
           << ", gy2=aggY(" << pf(p, "y2", 0.5f) << "," << h << ");\n";
        ss << "        double gdx=gx2-gx1, gdy=gy2-gy1, "
              "glen=sqrt(gdx*gdx+gdy*gdy);\n";
        ss << "        if(glen<1.0) glen=1.0;\n";
        ss << "        for(int y=0; y<" << h << "; y++) for(int x=0; x<" << w
           << "; x++) {\n";
        ss << "          double proj=((x-gx1)*gdx+(y-gy1)*gdy)/(glen*glen);\n";
        ss << "          if(proj<0) proj=0; if(proj>1) proj=1;\n";
        ss << "          agg::rgba16 c = ac1.gradient(ac2, proj);\n";
        ss << "          " << v << ".Data[y*" << w
           << "+x].Init((c.a>>8)<<24|(c.r>>8)<<16|(c.g>>8)<<8|(c.b>>8));\n";
        ss << "        }\n";
      } else {
        ss << "        double gcx=" << pf(p, "x1", 0.5f) << "*" << w
           << ", gcy=aggY(" << pf(p, "y1", 0.5f) << "," << h << ");\n";
        ss << "        double gex=" << pf(p, "x2", 0.9f) << "*" << w
           << ", gey=aggY(" << pf(p, "y2", 0.5f) << "," << h << ");\n";
        ss << "        double "
              "gradR=sqrt((gex-gcx)*(gex-gcx)+(gey-gcy)*(gey-gcy));\n";
        ss << "        if(gradR<1.0) gradR=1.0;\n";
        ss << "        for(int y=0; y<" << h << "; y++) for(int x=0; x<" << w
           << "; x++) {\n";
        ss << "          double "
              "d=sqrt((x-gcx)*(x-gcx)+(y-gcy)*(y-gcy))/gradR;\n";
        ss << "          if(d>1.0) d=1.0;\n";
        ss << "          agg::rgba16 c = ac1.gradient(ac2, d);\n";
        ss << "          " << v << ".Data[y*" << w
           << "+x].Init((c.a>>8)<<24|(c.r>>8)<<16|(c.g>>8)<<8|(c.b>>8));\n";
        ss << "        }\n";
      }
      ss << "      }\n";
    }

    ss << "    }\n";
  }

  else if (type == "Voronoi") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << "_Color, " << v << "_F1, " << v << "_Edge, "
       << v << "_Fill;\n";
    ss << "    " << v << "_Color.Init(" << w << ", " << h << "); " << v
       << "_F1.Init(" << w << ", " << h << "); " << v << "_Edge.Init(" << w
       << ", " << h << "); " << v << "_Fill.Init(" << w << ", " << h << ");\n";
    ss << "    MMVoronoi(&" << v << "_Color, &" << v << "_F1, &" << v
       << "_Edge, " << p.value("scaleX", 4) << ", " << p.value("scaleY", 4)
       << ", " << pf(p, "stretchX", 1.0f) << ", " << pf(p, "stretchY", 1.0f)
       << ", " << pf(p, "intensity", 0.75f) << ", " << pf(p, "randomness", 1.0f)
       << ", " << pf(p, "seed", 0.0f) << ", &" << v << "_Fill);\n";
  }

  else if (type == "FBM") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMFbm(" << v << ", " << p.value("mode", 1) << ", "
       << p.value("scaleX", 4) << ", " << p.value("scaleY", 4) << ", "
       << p.value("folds", 0) << ", " << p.value("octaves", 4) << ", "
       << pf(p, "persistence", 0.5f) << ", " << pf(p, "seed", 0.0f) << ");\n";
  }

  else if (type == "Blend") {
    std::string a = srcVar(conns, id, "A");
    std::string b = srcVar(conns, id, "B");
    if (a.empty() || b.empty()) {
      ss << "    // [Blend node " << id << " skipped: missing A/B input]\n";
      ss << "    GenTexture " << v << ";\n";
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    GenTexture " << v << ";\n";
      ss << "    " << v << ".Init(" << b << ".XRes, " << b << ".YRes);\n";
      ss << "    MMBlend(" << v << ", " << a << ", " << b << ", "
         << inputRef(conns, id, "Mask") << ", " << p.value("mode", 0) << ", "
         << pf(p, "opacity", 1.0f) << ");\n";
    }
  }

  else if (type == "Warp") {
    std::string in = srcVar(conns, id, "In");
    std::string hm = srcVar(conns, id, "Height");
    if (in.empty() || hm.empty()) {
      ss << "    // [Warp node " << id
         << " skipped: missing In/Height input]\n";
      ss << "    GenTexture " << v << ";\n";
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    GenTexture " << v << ";\n";
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMWarp(" << v << ", " << in << ", " << hm << ", "
         << inputRef(conns, id, "Strength") << ", " << pf(p, "amount", 0.1f)
         << ", " << pf(p, "epsilon", 0.005f) << ", " << p.value("mode", 0)
         << ");\n";
    }
  }

  else if (type == "Colorize") {
    std::string in = srcVar(conns, id, "In");
    auto stops = p.value("stops", nlohmann::json::array());
    ss << "    GenTexture " << v << ";\n";
    if (in.empty() || stops.empty()) {
      ss << "    // [Colorize node " << id << " skipped: missing input]\n";
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    { const MMGradientStop stops[] = {";
      for (size_t i = 0; i < stops.size(); i++) {
        auto& s = stops[i];
        ss << "{" << fmtf(s[0].get<float>()) << ", " << fmtf(s[1].get<float>())
           << ", " << fmtf(s[2].get<float>()) << ", " << fmtf(s[3].get<float>())
           << ", " << fmtf(s[4].get<float>()) << "}"
           << (i + 1 < stops.size() ? ", " : "");
      }
      ss << "};\n";
      ss << "      MMColorize(" << v << ", " << in << ", stops, "
         << stops.size() << "); }\n";
    }
  }

  else if (type == "BricksMM") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string c0 = colorHexArr(
        p.value("col0", nlohmann::json::array({0.55, 0.25, 0.15, 1.0})));
    std::string c1 = colorHexArr(
        p.value("col1", nlohmann::json::array({0.65, 0.35, 0.2, 1.0})));
    std::string cm = colorHexArr(
        p.value("colMortar", nlohmann::json::array({0.3, 0.3, 0.28, 1.0})));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMBricks(" << v << ", " << p.value("pattern", 0) << ", "
       << p.value("countX", 4) << ", " << p.value("countY", 8) << ", "
       << p.value("repeat", 1) << ", " << pf(p, "offset", 0.5f) << ", "
       << pf(p, "mortar", 0.1f) << ", " << pf(p, "round", 0.1f) << ", "
       << pf(p, "bevel", 0.2f) << ", " << c0 << ", " << c1 << ", " << cm << ", "
       << pf(p, "colorBalance", 0.5f) << ", " << pf(p, "seed", 0.0f) << ");\n";
  }

  else if (type == "Material") {
    static const char* channels[] = {
        "Albedo", "Normal", "Roughness", "Metallic", "Depth", "AO", "Emission"};
    std::string base = p.value("baseName", std::string("material"));
    for (auto& ch : channels) {
      std::string src = srcVar(conns, id, ch);
      if (src.empty())
        continue;
      std::string lower = ch;
      for (auto& c : lower)
        c = (char)tolower(c);
      ss << "    SaveImage(" << src << ", \"" << base << "_" << lower
         << ".png\");\n";
    }
    // Lit preview output
    static const char* previewIns[] = {
        "Albedo", "Normal", "Roughness", "Metallic", "AO", "Emission", "Depth"};
    std::string srcs[7], sizeRef;
    for (int i = 0; i < 7; i++) {
      srcs[i] = srcVar(conns, id, previewIns[i]);
      if (sizeRef.empty() && !srcs[i].empty())
        sizeRef = srcs[i];
    }
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init("
       << (sizeRef.empty() ? "256, 256"
                           : sizeRef + ".XRes, " + sizeRef + ".YRes")
       << ");\n";
    ss << "    MMShadePreview(" << v;
    for (int i = 0; i < 7; i++)
      ss << ", " << (srcs[i].empty() ? "nullptr" : "&" + srcs[i]);
    ss << ", " << pf(p, "lightAzimuth", 135.0f) << ", "
       << pf(p, "lightElevation", 45.0f) << ", "
       << pf(p, "lightIntensity", 1.0f) << ", " << pf(p, "ambient", 0.25f)
       << ");\n";
  }

  else if (type == "NormalMap") {
    std::string in = srcVar(conns, id, "Height");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMNormalMap(" << v << ", " << in << ", "
         << pf(p, "amount", 1.0f) << ", " << p.value("format", 1) << ");\n";
    }
  }

  else if (type == "LayerMix") {
    static const char* outSlots[5] = {"H", "C", "ORM", "EM", "NM"};
    static const char* inSlots[10] = {"H1", "C1", "ORM1", "EM1", "NM1",
                                      "H2", "C2", "ORM2", "EM2", "NM2"};
    std::string srcs[10], sizeRef;
    for (int i = 0; i < 10; i++) {
      srcs[i] = srcVar(conns, id, inSlots[i]);
      if (sizeRef.empty() && !srcs[i].empty())
        sizeRef = srcs[i];
    }
    std::string dims =
        sizeRef.empty() ? "256, 256" : sizeRef + ".XRes, " + sizeRef + ".YRes";
    ss << "    GenTexture";
    for (int i = 0; i < 5; i++)
      ss << (i ? "," : "") << " " << v << "_" << outSlots[i];
    ss << ";\n";
    for (int i = 0; i < 5; i++)
      ss << "    " << v << "_" << outSlots[i] << ".Init(" << dims << ");\n";
    ss << "    { GenTexture* outs[5] = {";
    for (int i = 0; i < 5; i++)
      ss << (i ? ", " : "") << "&" << v << "_" << outSlots[i];
    ss << "};\n";
    for (int half = 0; half < 2; half++) {
      ss << "      const GenTexture* l" << (half + 1) << "[5] = {";
      for (int i = 0; i < 5; i++) {
        const std::string& s = srcs[half * 5 + i];
        ss << (i ? ", " : "") << (s.empty() ? "nullptr" : "&" + s);
      }
      ss << "};\n";
    }
    ss << "      MMLayerMix(outs, l1, l2, " << p.value("mode", 0) << ", "
       << pf(p, "width", 0.05f) << "); }\n";
  }

  else if (type == "WorkflowOutput") {
    static const char* outSlots[7] = {"Albedo",   "Metallic", "Roughness",
                                      "Emission", "Normal",   "Occlusion",
                                      "Depth"};
    static const char* inSlots[5] = {"Height", "Albedo", "ORM", "Emission",
                                     "Normal"};
    std::string srcs[5], sizeRef;
    for (int i = 0; i < 5; i++) {
      srcs[i] = srcVar(conns, id, inSlots[i]);
      if (sizeRef.empty() && !srcs[i].empty())
        sizeRef = srcs[i];
    }
    std::string dims =
        sizeRef.empty() ? "256, 256" : sizeRef + ".XRes, " + sizeRef + ".YRes";
    ss << "    GenTexture";
    for (int i = 0; i < 7; i++)
      ss << (i ? "," : "") << " " << v << "_" << outSlots[i];
    ss << ";\n";
    for (int i = 0; i < 7; i++)
      ss << "    " << v << "_" << outSlots[i] << ".Init(" << dims << ");\n";
    ss << "    MMWorkflowOutput(";
    for (int i = 0; i < 7; i++)
      ss << (i ? ", " : "") << v << "_" << outSlots[i];
    for (int i = 0; i < 5; i++)
      ss << ", " << (srcs[i].empty() ? "nullptr" : "&" + srcs[i]);
    ss << ", " << pf(p, "matNormal", 1.0f) << ", " << pf(p, "occlusion", 1.0f)
       << ");\n";
  }

  else if (type == "MathOp") {
    std::string a = srcVar(conns, id, "A");
    std::string b = srcVar(conns, id, "B");
    std::string sz = !a.empty() ? a : b;
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init("
       << (sz.empty() ? "256, 256" : sz + ".XRes, " + sz + ".YRes") << ");\n";
    ss << "    MMMath(" << v << ", " << (a.empty() ? "nullptr" : "&" + a)
       << ", " << (b.empty() ? "nullptr" : "&" + b) << ", " << p.value("op", 0)
       << ", " << pf(p, "def1", 0.0f) << ", " << pf(p, "def2", 0.0f) << ", "
       << (p.value("clamp", false) ? "true" : "false") << ");\n";
  }

  else if (type == "GradientMM") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    auto stops = p.value("stops", nlohmann::json::array());
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    if (!stops.empty()) {
      ss << "    { const MMGradientStop stops[] = {";
      for (size_t i = 0; i < stops.size(); i++) {
        auto& s = stops[i];
        ss << "{" << fmtf(s[0].get<float>()) << ", " << fmtf(s[1].get<float>())
           << ", " << fmtf(s[2].get<float>()) << ", " << fmtf(s[3].get<float>())
           << ", " << fmtf(s[4].get<float>()) << "}"
           << (i + 1 < stops.size() ? ", " : "");
      }
      ss << "};\n";
      ss << "      MMGradientRamp(" << v << ", stops, " << stops.size() << ", "
         << pf(p, "repeat", 1.0f) << ", " << pf(p, "rotate", 0.0f) << ", "
         << (p.value("mirror", false) ? "true" : "false") << "); }\n";
    }
  }

  else if (type == "Tiler") {
    std::string in = srcVar(conns, id, "In");
    std::string mask = srcVar(conns, id, "Mask");
    ss << "    GenTexture " << v << "_Out, " << v << "_Color;\n";
    if (in.empty()) {
      ss << "    " << v << "_Out.Init(256, 256); " << v
         << "_Color.Init(256, 256);\n";
    } else {
      ss << "    " << v << "_Out.Init(" << in << ".XRes, " << in << ".YRes); "
         << v << "_Color.Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMTiler(" << v << "_Out, &" << v << "_Color, " << in << ", "
         << (mask.empty() ? "nullptr" : "&" + mask) << ", " << pf(p, "tx", 4.0f)
         << ", " << pf(p, "ty", 4.0f) << ", " << p.value("overlap", 1) << ", "
         << p.value("inputs", 1) << ", " << pf(p, "scaleX", 1.0f) << ", "
         << pf(p, "scaleY", 1.0f) << ", " << pf(p, "fixedOffset", 0.0f) << ", "
         << pf(p, "offset", 0.5f) << ", " << pf(p, "rotate", 0.0f) << ", "
         << pf(p, "scale", 0.0f) << ", " << pf(p, "value", 0.0f) << ", "
         << pf(p, "seed", 0.0f) << ");\n";
    }
  }

  else if (type == "MultiWarp") {
    std::string in = srcVar(conns, id, "In");
    std::string hm = srcVar(conns, id, "Height");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty() || hm.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMMultiWarp(" << v << ", " << in << ", " << hm << ", "
         << pf(p, "size", 9.0f) << ", " << pf(p, "intensity", 0.5f) << ", "
         << pf(p, "quality", 50.0f) << ", " << p.value("mode", 2) << ");\n";
    }
  }

  else if (type == "SlopeBlur") {
    std::string in = srcVar(conns, id, "In");
    std::string hm = srcVar(conns, id, "Height");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty() || hm.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMSlopeBlur(" << v << ", " << in << ", " << hm << ", "
         << pf(p, "size", 9.0f) << ", " << pf(p, "sigma", 0.5f) << ");\n";
    }
  }

  else if (type == "HeightToOffset") {
    std::string in = srcVar(conns, id, "Height");
    ss << "    GenTexture " << v << "_X, " << v << "_Y;\n";
    if (in.empty()) {
      ss << "    " << v << "_X.Init(256, 256); " << v << "_Y.Init(256, 256);\n";
    } else {
      ss << "    " << v << "_X.Init(" << in << ".XRes, " << in << ".YRes); "
         << v << "_Y.Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMHeightToOffset(" << v << "_X, " << v << "_Y, " << in << ", "
         << pf(p, "target", 0.5f) << ");\n";
    }
  }

  else if (type == "Bevel") {
    std::string in = srcVar(conns, id, "In");
    auto curve = p.value("curve", nlohmann::json::array());
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    { const MMCurvePoint curve[] = {";
      if (curve.empty())
        ss << "{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.0f}";
      else
        for (size_t i = 0; i < curve.size(); i++) {
          auto& c = curve[i];
          ss << "{" << fmtf(c[0].get<float>()) << ", "
             << fmtf(c[1].get<float>()) << ", " << fmtf(c[2].get<float>())
             << ", " << fmtf(c[3].get<float>()) << "}"
             << (i + 1 < curve.size() ? ", " : "");
        }
      ss << "};\n";
      ss << "      MMBevel(" << v << ", " << in << ", "
         << pf(p, "distance", 0.1f) << ", curve, "
         << (curve.empty() ? 2 : (int)curve.size()) << "); }\n";
    }
  }

  else if (type == "AnisotropicNoise") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMAnisotropicNoise(" << v << ", " << pf(p, "scaleX", 4.0f)
       << ", " << pf(p, "scaleY", 256.0f) << ", " << pf(p, "seed", 0.0f) << ", "
       << pf(p, "smoothness", 1.0f) << ", " << pf(p, "interpolation", 1.0f)
       << ");\n";
  }

  else if (type == "TilerAdvanced") {
    static const char* outSlots[4] = {"Out", "Color1", "Color2", "UV"};
    static const char* inSlots[9] = {"In",  "Mask", "Color1", "Color2", "TrX",
                                     "TrY", "Rot",  "ScX",    "ScY"};
    std::string srcs[9];
    for (int i = 0; i < 9; i++)
      srcs[i] = srcVar(conns, id, inSlots[i]);
    ss << "    GenTexture";
    for (int i = 0; i < 4; i++)
      ss << (i ? "," : "") << " " << v << "_" << outSlots[i];
    ss << ";\n";
    std::string dims =
        srcs[0].empty() ? "256, 256" : srcs[0] + ".XRes, " + srcs[0] + ".YRes";
    for (int i = 0; i < 4; i++)
      ss << "    " << v << "_" << outSlots[i] << ".Init(" << dims << ");\n";
    if (!srcs[0].empty()) {
      ss << "    MMTilerAdvanced(" << v << "_Out, &" << v << "_Color1, &" << v
         << "_Color2, &" << v << "_UV, " << srcs[0];
      // mask, trX, trY, rot, scX, scY, color1, color2
      static const int order[8] = {1, 4, 5, 6, 7, 8, 2, 3};
      for (int k = 0; k < 8; k++) {
        const std::string& s = srcs[order[k]];
        ss << ", " << (s.empty() ? "nullptr" : "&" + s);
      }
      ss << ", " << pf(p, "tx", 4.0f) << ", " << pf(p, "ty", 4.0f) << ", "
         << p.value("overlap", 1) << ", " << p.value("inputs", 1) << ", "
         << pf(p, "translateX", 0.0f) << ", " << pf(p, "translateY", 0.0f)
         << ", " << pf(p, "rotate", 0.0f) << ", " << pf(p, "scaleX", 1.0f)
         << ", " << pf(p, "scaleY", 1.0f) << ", " << pf(p, "seed", 0.0f)
         << ");\n";
    }
  }

  else if (type == "Sphere") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMSphere(" << v << ", " << pf(p, "cx", 0.5f) << ", "
       << pf(p, "cy", 0.5f) << ", " << pf(p, "r", 0.5f) << ", "
       << (p.value("normalized", false) ? "true" : "false") << ");\n";
  }

  else if (type == "DotNoise") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string dens = srcVar(conns, id, "Density");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMDotNoise(" << v << ", " << p.value("grid", 256) << ", "
       << pf(p, "density", 0.5f) << ", "
       << (dens.empty() ? "nullptr" : "&" + dens) << ", " << pf(p, "seed", 0.0f)
       << ");\n";
  }

  else if (type == "Scratches") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMScratches(" << v << ", " << p.value("layers", 4) << ", "
       << pf(p, "length", 0.25f) << ", " << pf(p, "width", 0.5f) << ", "
       << pf(p, "waviness", 0.5f) << ", " << pf(p, "angle", 0.0f) << ", "
       << pf(p, "randomness", 0.5f) << ", " << pf(p, "seed", 0.0f) << ");\n";
  }

  else if (type == "Mirror") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMMirror(" << v << ", " << in << ", "
         << p.value("direction", 0) << ", " << pf(p, "offset", 0.0f) << ", "
         << (p.value("flipSides", false) ? "true" : "false") << ");\n";
    }
  }

  else if (type == "EdgeDetect") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMEdgeDetect(" << v << ", " << in << ", "
         << pf(p, "size", 512.0f) << ", " << p.value("width", 1) << ", "
         << pf(p, "threshold", 0.5f) << ");\n";
    }
  }

  else if (type == "CreateMap") {
    std::string h = srcVar(conns, id, "Height");
    std::string o = srcVar(conns, id, "Offset");
    std::string sz = !h.empty() ? h : o;
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init("
       << (sz.empty() ? "256, 256" : sz + ".XRes, " + sz + ".YRes") << ");\n";
    ss << "    MMCreateMap(" << v << ", " << (h.empty() ? "nullptr" : "&" + h)
       << ", " << (o.empty() ? "nullptr" : "&" + o) << ", "
       << pf(p, "height", 1.0f) << ", " << pf(p, "angle", 0.0f) << ", "
       << pf(p, "seed", 0.0f) << ");\n";
  }

  else if (type == "MatMap") {
    static const char* outSlots[5] = {"H", "C", "ORM", "EM", "NM"};
    static const char* inSlots[4] = {"C", "ORM", "EM", "NM"};
    std::string map = srcVar(conns, id, "Map");
    std::string srcs[4];
    for (int i = 0; i < 4; i++)
      srcs[i] = srcVar(conns, id, inSlots[i]);
    ss << "    GenTexture";
    for (int i = 0; i < 5; i++)
      ss << (i ? "," : "") << " " << v << "_" << outSlots[i];
    ss << ";\n";
    std::string dims =
        map.empty() ? "256, 256" : map + ".XRes, " + map + ".YRes";
    for (int i = 0; i < 5; i++)
      ss << "    " << v << "_" << outSlots[i] << ".Init(" << dims << ");\n";
    if (!map.empty()) {
      ss << "    MMMatMap(";
      for (int i = 0; i < 5; i++)
        ss << (i ? ", " : "") << v << "_" << outSlots[i];
      ss << ", " << map;
      for (int i = 0; i < 4; i++)
        ss << ", " << (srcs[i].empty() ? "nullptr" : "&" + srcs[i]);
      ss << ");\n";
    }
  }

  else if (type == "Fill") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMFill(" << v << ", " << in << ");\n";
    }
  }

  else if (type == "FillToUV") {
    std::string in = srcVar(conns, id, "Fill");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMFillToUV(" << v << ", " << in << ", " << p.value("mode", 0)
         << ", " << pf(p, "seed", 0.0f) << ");\n";
    }
  }

  else if (type == "FillToRandomGray") {
    std::string in = srcVar(conns, id, "Fill");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMFillToRandomGray(" << v << ", " << in << ", "
         << pf(p, "edgecolor", 1.0f) << ", " << pf(p, "seed", 0.0f) << ");\n";
    }
  }

  else if (type == "FillToRandomColor") {
    std::string in = srcVar(conns, id, "Fill");
    auto edge = p.value("edge", nlohmann::json::array({1.0, 1.0, 1.0}));
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMFillToRandomColor(" << v << ", " << in << ", "
         << fmtf(edge[0].get<float>()) << ", " << fmtf(edge[1].get<float>())
         << ", " << fmtf(edge[2].get<float>()) << ", " << pf(p, "seed", 0.0f)
         << ");\n";
    }
  }

  else if (type == "FillToColor") {
    std::string in = srcVar(conns, id, "Fill");
    std::string map = srcVar(conns, id, "Map");
    auto edge = p.value("edge", nlohmann::json::array({1.0, 1.0, 1.0, 1.0}));
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMFillToColor(" << v << ", " << in << ", "
         << (map.empty() ? "nullptr" : "&" + map) << ", "
         << fmtf(edge[0].get<float>()) << ", " << fmtf(edge[1].get<float>())
         << ", " << fmtf(edge[2].get<float>()) << ", "
         << fmtf(edge[3].get<float>()) << ");\n";
    }
  }

  else if (type == "Levels") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      static const char* keys[5] = {"inMin", "inMid", "inMax", "outMin",
                                    "outMax"};
      static const float defs[5] = {0.0f, 0.5f, 1.0f, 0.0f, 1.0f};
      ss << "    { ";
      for (int k = 0; k < 5; k++) {
        auto arr = p.value(keys[k], nlohmann::json::array());
        ss << "const sF32 " << keys[k] << "[4] = {";
        for (int i = 0; i < 4; i++) {
          float val = (arr.is_array() && (int)arr.size() > i)
                          ? arr[i].get<float>()
                          : defs[k];
          ss << fmtf(val) << (i < 3 ? ", " : "");
        }
        ss << "}; ";
      }
      ss << "\n      MMLevels(" << v << ", " << in
         << ", inMin, inMid, inMax, outMin, outMax); }\n";
    }
  }

  else if (type == "Remap") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMRemap(" << v << ", " << in << ", " << pf(p, "min", 0.0f)
         << ", " << pf(p, "max", 1.0f) << ", " << pf(p, "step", 0.0f) << ");\n";
    }
  }

  else if (type == "Tile2x2") {
    std::string srcs[4], sizeRef;
    static const char* ins[4] = {"In1", "In2", "In3", "In4"};
    for (int i = 0; i < 4; i++) {
      srcs[i] = srcVar(conns, id, ins[i]);
      if (sizeRef.empty() && !srcs[i].empty())
        sizeRef = srcs[i];
    }
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init("
       << (sizeRef.empty() ? "256, 256"
                           : sizeRef + ".XRes, " + sizeRef + ".YRes")
       << ");\n";
    ss << "    MMTile2x2(" << v;
    for (int i = 0; i < 4; i++)
      ss << ", " << (srcs[i].empty() ? "nullptr" : "&" + srcs[i]);
    ss << ");\n";
  }

  else if (type == "NormalConvert") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMNormalConvert(" << v << ", " << in << ", "
         << p.value("op", 1) << ");\n";
    }
  }

  else if (type == "CustomUV") {
    std::string in = srcVar(conns, id, "In");
    std::string map = srcVar(conns, id, "Map");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty() || map.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << map << ".XRes, " << map << ".YRes);\n";
      ss << "    MMCustomUV(" << v << ", " << in << ", " << map << ", "
         << p.value("inputs", 1) << ", " << pf(p, "sx", 1.0f) << ", "
         << pf(p, "sy", 1.0f) << ", " << pf(p, "rotate", 0.0f) << ", "
         << pf(p, "scale", 0.5f) << ", " << pf(p, "seed", 0.0f) << ");\n";
    }
  }

  else if (type == "SmoothCurvature") {
    std::string in = srcVar(conns, id, "Height");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMSmoothCurvature(" << v << ", " << in << ", "
         << pf(p, "quality", 4.0f) << ", " << pf(p, "strength", 1.0f) << ", "
         << pf(p, "radius", 1.0f) << ");\n";
    }
  }

  else if (type == "AmbientOcclusion") {
    std::string in = srcVar(conns, id, "Height");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMAmbientOcclusion(" << v << ", " << in << ", "
         << pf(p, "radius", 0.05f) << ", " << pf(p, "strength", 1.0f) << ");\n";
    }
  }

  else if (type == "SdfShape") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    { MMSdfShapeParams sp;\n";
    ss << "      sp.shape = " << p.value("shape", 0)
       << "; sp.cx = " << pf(p, "cx", 0.5f) << "; sp.cy = " << pf(p, "cy", 0.5f)
       << ";\n";
    ss << "      sp.w = " << pf(p, "w", 0.3f) << "; sp.h = " << pf(p, "h", 0.2f)
       << "; sp.n = " << p.value("n", 5) << "; sp.ir = " << pf(p, "ir", 0.5f)
       << "; sp.rot = " << pf(p, "rot", 0.0f) << ";\n";
    ss << "      sp.ax = " << pf(p, "ax", 0.2f)
       << "; sp.ay = " << pf(p, "ay", 0.2f) << "; sp.bx = " << pf(p, "bx", 0.8f)
       << "; sp.by = " << pf(p, "by", 0.8f) << ";\n";
    ss << "      MMSdfShape(" << v << ", sp); }\n";
  }

  else if (type == "SdfOp") {
    std::string a = srcVar(conns, id, "A");
    std::string b = srcVar(conns, id, "B");
    ss << "    GenTexture " << v << ";\n";
    if (a.empty() || b.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << a << ".XRes, " << a << ".YRes);\n";
      ss << "    MMSdfOp(" << v << ", " << a << ", " << b << ", "
         << p.value("op", 0) << ", " << pf(p, "k", 0.1f) << ");\n";
    }
  }

  else if (type == "SdfTransform") {
    std::string in = srcVar(conns, id, "Sdf");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMSdfTransform(" << v << ", " << in << ", "
         << pf(p, "tx", 0.0f) << ", " << pf(p, "ty", 0.0f) << ", "
         << pf(p, "rot", 0.0f) << ", " << pf(p, "scale", 1.0f) << ", "
         << pf(p, "round", 0.0f) << ", " << pf(p, "annularW", 0.05f) << ", "
         << p.value("annularCount", 0) << ");\n";
    }
  }

  else if (type == "SdfShow") {
    std::string in = srcVar(conns, id, "Sdf");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMSdfShow(" << v << ", " << in << ", " << pf(p, "base", 0.0f)
         << ", " << pf(p, "bevel", 0.01f) << ");\n";
    }
  }

  else if (type == "MakeTileable") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMMakeTileable(" << v << ", " << in << ", "
         << pf(p, "width", 0.1f) << ");\n";
    }
  }

  else if (type == "Quantize") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMQuantize(" << v << ", " << in << ", " << p.value("steps", 4)
         << ");\n";
    }
  }

  else if (type == "Emboss") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMEmboss(" << v << ", " << in << ", " << pf(p, "angle", 0.0f)
         << ", " << pf(p, "amount", 1.0f) << ", " << p.value("width", 1)
         << ");\n";
    }
  }

  else if (type == "Transform2D") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMTransform(" << v << ", " << in << ", " << pf(p, "tx", 0.0f)
         << ", " << pf(p, "ty", 0.0f) << ", " << pf(p, "rot", 0.0f) << ", "
         << pf(p, "scaleX", 1.0f) << ", " << pf(p, "scaleY", 1.0f) << ", "
         << (p.value("repeat", true) ? "true" : "false") << ", "
         << inputRef(conns, id, "TX") << ", " << inputRef(conns, id, "TY")
         << ", " << inputRef(conns, id, "Rot") << ", "
         << inputRef(conns, id, "SX") << ", " << inputRef(conns, id, "SY")
         << ");\n";
    }
  }

  else if (type == "Shape") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMShape(" << v << ", " << p.value("shape", 0) << ", "
       << pf(p, "sides", 3.0f) << ", " << pf(p, "radius", 1.0f) << ", "
       << pf(p, "edge", 0.2f) << ", " << inputRef(conns, id, "RadiusMap")
       << ", " << inputRef(conns, id, "EdgeMap") << ");\n";
  }

  else if (type == "Pattern") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    MMPattern(" << v << ", " << p.value("mix", 0) << ", "
       << p.value("xWave", 0) << ", " << pf(p, "xScale", 4.0f) << ", "
       << p.value("yWave", 0) << ", " << pf(p, "yScale", 4.0f) << ");\n";
  }

  else if (type == "Combine") {
    std::string r = srcVar(conns, id, "R");
    std::string g = srcVar(conns, id, "G");
    std::string b = srcVar(conns, id, "B");
    std::string a = srcVar(conns, id, "A");
    std::string first;
    for (auto& s : {r, g, b, a})
      if (!s.empty()) {
        first = s;
        break;
      }
    ss << "    GenTexture " << v << ";\n";
    if (first.empty())
      ss << "    " << v << ".Init(256, 256);\n";
    else
      ss << "    " << v << ".Init(" << first << ".XRes, " << first
         << ".YRes);\n";
    auto ref = [](const std::string& s) {
      return s.empty() ? std::string("NULL") : "&" + s;
    };
    ss << "    MMCombine(" << v << ", " << ref(r) << ", " << ref(g) << ", "
       << ref(b) << ", " << ref(a) << ");\n";
  }

  else if (type == "Decompose") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << "_R, " << v << "_G, " << v << "_B, " << v
       << "_A;\n";
    if (in.empty()) {
      ss << "    " << v << "_R.Init(256, 256); " << v << "_G.Init(256, 256); "
         << v << "_B.Init(256, 256); " << v << "_A.Init(256, 256);\n";
    } else {
      ss << "    " << v << "_R.Init(" << in << ".XRes, " << in << ".YRes); "
         << v << "_G.Init(" << in << ".XRes, " << in << ".YRes); " << v
         << "_B.Init(" << in << ".XRes, " << in << ".YRes); " << v << "_A.Init("
         << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMDecompose(" << v << "_R, " << v << "_G, " << v << "_B, " << v
         << "_A, " << in << ");\n";
    }
  }

  else if (type == "Invert") {
    std::string in = srcVar(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (in.empty()) {
      ss << "    " << v << ".Init(256, 256);\n";
    } else {
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
      ss << "    MMInvert(" << v << ", " << in << ");\n";
    }
  }

  else {
    ss << "    // [unsupported node: " << type << " id " << id << "]\n";
  }
}

#ifndef TEXGEN_NO_UI
bool exportCHeader(NodeGraph* graph,
                   const std::string& name,
                   const std::string& outPath) {
  if (!graph)
    return false;
  nlohmann::json j = graph->save();
  return exportCHeaderFromJSON(j, name, outPath);
}
#endif

bool exportCHeaderFromJSON(const nlohmann::json& project,
                           const std::string& name,
                           const std::string& outPath) {
  // Inline subgraphs and bake Remote-driven parameter values, so the
  // emitter only ever sees flat graphs of concrete nodes.
  nlohmann::json j = applyRemotes(flattenSubgraphs(project));
  auto& nodes = j["nodes"];
  auto& conns = j["connections"];

  // Build id -> node JSON map
  std::map<int, nlohmann::json*> nodeMap;
  for (auto& n : nodes)
    nodeMap[n["id"].get<int>()] = &n;

  // Output slot names per node (multi-output nodes get per-slot variables)
  g_nodeOutputs.clear();
  for (auto& n : nodes) {
    auto core = getCoreNodeRegistry().create(n["typeName"].get<std::string>());
    if (core)
      g_nodeOutputs[n["id"].get<int>()] = core->outputSlotNames();
  }

  // Topological sort via Kahn's algorithm on the JSON
  std::map<int, int> inDegree;
  std::map<int, std::vector<int>> adj;
  for (auto& [id, _] : nodeMap) {
    inDegree[id] = 0;
    adj[id] = {};
  }
  for (auto& c : conns) {
    int from = c["fromId"].get<int>();
    int to = c["toId"].get<int>();
    adj[from].push_back(to);
    inDegree[to]++;
  }
  std::vector<int> sorted;
  std::vector<int> queue;
  for (auto& [id, deg] : inDegree)
    if (deg == 0)
      queue.push_back(id);
  while (!queue.empty()) {
    int cur = queue.back();
    queue.pop_back();
    sorted.push_back(cur);
    for (int next : adj[cur])
      if (--inDegree[next] == 0)
        queue.push_back(next);
  }
  // Append any remaining (disconnected) nodes
  for (auto& [id, _] : nodeMap) {
    bool found = false;
    for (int s : sorted)
      if (s == id) {
        found = true;
        break;
      }
    if (!found)
      sorted.push_back(id);
  }

  // Generate the header
  std::string guard = name + "_H";
  for (auto& c : guard)
    c = toupper(c);

  std::ostringstream ss;
  ss << "// Generated by TEXGEN - do not edit\n";
  ss << "// This generated file is placed in the public domain.\n";
  ss << "// No rights reserved. Use for any purpose without restriction.\n";
  ss << "#pragma once\n";
  ss << "#ifndef " << guard << "\n";
  ss << "#define " << guard << "\n\n";
  ss << "#include \"texgen.h\"\n";
  ss << "#include <cstring>\n";
  ss << "#include <cmath>\n\n";
  ss << "static inline void " << name << "_generate(GenTexture* out) {\n";

  for (int id : sorted) {
    auto it = nodeMap.find(id);
    if (it == nodeMap.end())
      continue;
    auto& n = *(it->second);
    std::string type = n["typeName"].get<std::string>();
    nlohmann::json params = n.value("params", nlohmann::json::object());
    ss << "\n    // " << type << " (node " << id << ")\n";
    emitNode(ss, type, id, params, conns);
  }

  ss << "}\n\n";
  ss << "#endif // " << guard << "\n";

  FILE* f = fopen(outPath.c_str(), "w");
  if (!f)
    return false;
  std::string code = ss.str();
  fwrite(code.c_str(), 1, code.size(), f);
  fclose(f);
  return true;
}
