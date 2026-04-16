#include "CExport.h"
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
  int src = findInputSource(connections, nodeId, slotName);
  if (src >= 0)
    return "&" + var(src);
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
    std::string in = (src >= 0) ? var(src) : v;
    if (src >= 0) {
      ss << "    GenTexture " << v << ";\n";
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    }
    ss << "    " << v << ".Blur(" << in << ", " << pf(p, "sizex", 0.01f) << ", "
       << pf(p, "sizey", 0.01f) << ", " << p.value("order", 2) << ", "
       << p.value("mode", 0) << ");\n";
  }

  else if (type == "BlurKernel") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    BlurKernel(" << v << ", " << in << ", "
       << pf(p, "radiusX", 0.01f) << ", " << pf(p, "radiusY", 0.01f) << ", "
       << p.value("kernelType", 2) << ", " << p.value("wrapMode", 0) << ");\n";
  }

  else if (type == "HSCB") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    HSCB(" << v << ", " << in << ", " << pf(p, "hue", 0.0f) << ", "
       << pf(p, "sat", 1.0f) << ", " << pf(p, "contrast", 1.0f) << ", "
       << pf(p, "brightness", 1.0f) << ");\n";
  }

  else if (type == "Derive") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    " << v << ".Derive(" << in << ", " << p.value("op", 0) << ", "
       << pf(p, "strength", 1.0f) << ");\n";
  }

  else if (type == "ColorMatrix") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
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
      ss << "    " << v << ".Init(" << var(src1) << ".XRes, " << var(src1)
         << ".YRes);\n";
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
        ss << "      li[" << i << "].Tex = &" << var(s) << "; li[" << i
           << "].Weight = " << fmtf(weights[i].get<float>()) << "; li[" << i
           << "].UShift = " << fmtf(uShift[i].get<float>()) << "; li[" << i
           << "].VShift = " << fmtf(vShift[i].get<float>()) << "; li[" << i
           << "].FilterMode = " << fMode[i].get<int>() << ";\n";
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
      ss << "    " << v << ".Init(" << var(bgSrc) << ".XRes, " << var(bgSrc)
         << ".YRes);\n";
    else
      ss << "    " << v << ".Init(256, 256);\n";
    std::string bg = (bgSrc >= 0) ? var(bgSrc) : v;
    std::string sn = (snSrc >= 0) ? var(snSrc) : v;
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
      ss << "    " << v << ".Init(" << var(s1) << ".XRes, " << var(s1)
         << ".YRes);\n";
    std::string i1 = (s1 >= 0) ? var(s1) : v;
    std::string i2 = (s2 >= 0) ? var(s2) : v;
    std::string mask = (sm >= 0) ? var(sm) : v;
    ss << "    " << v << ".Ternary(" << i1 << ", " << i2 << ", " << mask << ", "
       << p.value("op", 0) << ");\n";
  }

  else if (type == "Wavelet") {
    int src = findInputSource(conns, id, "In");
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << " = " << var(src) << ";\n";
    ss << "    Wavelet(" << v << ", " << p.value("mode", 0) << ", "
       << p.value("count", 1) << ");\n";
  }

  else if (type == "ColorBalance") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
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
      ss << "    *out = " << var(src) << ";\n";
  }

  // Skip Comment and Image nodes (not exportable as pure C)
  else if (type == "Comment" || type == "Image") {
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
      ss << "    if(" << var(bgSrc) << ".Data && " << var(bgSrc)
         << ".XRes==" << w << " && " << var(bgSrc) << ".YRes==" << h << ")\n";
      ss << "      memcpy(" << v << ".Data, " << var(bgSrc) << ".Data, " << w
         << "*" << h << "*sizeof(Pixel));\n";
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
  nlohmann::json j = project;  // mutable copy
  auto& nodes = j["nodes"];
  auto& conns = j["connections"];

  // Build id -> node JSON map
  std::map<int, nlohmann::json*> nodeMap;
  for (auto& n : nodes)
    nodeMap[n["id"].get<int>()] = &n;

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
