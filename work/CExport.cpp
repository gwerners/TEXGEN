#include "CExport.h"
#include "Nodes.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

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

  if (type == "Color" || type == "Input") {
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
       << p.value("fadeoff", 0.5f) << "f, " << p.value("seed", 0) << ", "
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
       << ", " << p.value("amp", 0.0f) << "f, " << p.value("mode", 0)
       << "); }\n";
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
       << p.value("fugeX", 0.03f) << "f, " << p.value("fugeY", 0.06f) << "f, "
       << p.value("tileX", 4) << ", " << p.value("tileY", 8) << ", "
       << p.value("seed", 0) << ", " << p.value("heads", 2) << ", "
       << p.value("colorBalance", 0.5f) << "f);\n";
  }

  else if (type == "DirectionalGradient") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    DirectionalGradient(" << v << ", " << p.value("x1", 0.0f)
       << "f, " << p.value("y1", 0.0f) << "f, " << p.value("x2", 1.0f) << "f, "
       << p.value("y2", 1.0f) << "f, " << c1 << ", " << c2 << ");\n";
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
    ss << "    GlowEffect(" << v << ", " << p.value("cx", 0.5f) << "f, "
       << p.value("cy", 0.5f) << "f, " << p.value("scale", 1.0f) << "f, "
       << p.value("exponent", 2.0f) << "f, " << p.value("intensity", 1.0f)
       << "f, " << bg << ", " << gl << ");\n";
  }

  else if (type == "PerlinNoiseRG2") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    std::string c1 = colorHex(p, "c1r", "c1g", "c1b", "c1a");
    std::string c2 = colorHex(p, "c2r", "c2g", "c2b", "c2a");
    ss << "    GenTexture " << v << ";\n";
    ss << "    " << v << ".Init(" << w << ", " << h << ");\n";
    ss << "    PerlinNoiseRG2(" << v << ", " << p.value("octaves", 4) << ", "
       << p.value("persistence", 0.5f) << "f, " << p.value("freqScale", 2)
       << ", " << p.value("seed", 0) << ", " << p.value("contrast", 1.0f)
       << "f, " << c1 << ", " << c2 << ", " << p.value("startOctave", 0)
       << ");\n";
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
    ss << "    " << v << ".Blur(" << in << ", " << p.value("sizex", 0.01f)
       << "f, " << p.value("sizey", 0.01f) << "f, " << p.value("order", 2)
       << ", " << p.value("mode", 0) << ");\n";
  }

  else if (type == "BlurKernel") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    BlurKernel(" << v << ", " << in << ", "
       << p.value("radiusX", 0.01f) << "f, " << p.value("radiusY", 0.01f)
       << "f, " << p.value("kernelType", 2) << ", " << p.value("wrapMode", 0)
       << ");\n";
  }

  else if (type == "HSCB") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    HSCB(" << v << ", " << in << ", " << p.value("hue", 0.0f)
       << "f, " << p.value("sat", 1.0f) << "f, " << p.value("contrast", 1.0f)
       << "f, " << p.value("brightness", 1.0f) << "f);\n";
  }

  else if (type == "Derive") {
    int src = findInputSource(conns, id, "In");
    std::string in = (src >= 0) ? var(src) : v;
    ss << "    GenTexture " << v << ";\n";
    if (src >= 0)
      ss << "    " << v << ".Init(" << in << ".XRes, " << in << ".YRes);\n";
    ss << "    " << v << ".Derive(" << in << ", " << p.value("op", 0) << ", "
       << p.value("strength", 1.0f) << "f);\n";
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
      ss << val << "f";
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
           << "].Weight = " << weights[i].get<float>() << "f; li[" << i
           << "].UShift = " << uShift[i].get<float>() << "f; li[" << i
           << "].VShift = " << vShift[i].get<float>() << "f; li[" << i
           << "].FilterMode = " << fMode[i].get<int>() << ";\n";
        count = i + 1;
      }
    }
    ss << "      " << v << ".LinearCombine(cp, " << p.value("constWeight", 0.0f)
       << "f, li, " << count << "); }\n";
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
       << p.value("orgx", 0.0f) << "f, " << p.value("orgy", 0.0f) << "f, "
       << p.value("ux", 1.0f) << "f, " << p.value("uy", 0.0f) << "f, "
       << p.value("vx", 0.0f) << "f, " << p.value("vy", 1.0f) << "f, "
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
    ss << "    ColorBalance(" << v << ", " << in << ", " << sh[0].get<float>()
       << "f, " << sh[1].get<float>() << "f, " << sh[2].get<float>() << "f, "
       << md[0].get<float>() << "f, " << md[1].get<float>() << "f, "
       << md[2].get<float>() << "f, " << hl[0].get<float>() << "f, "
       << hl[1].get<float>() << "f, " << hl[2].get<float>() << "f);\n";
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
           type == "AggPolygon" || type == "AggText") {
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
      ss << "      path.move_to(" << p.value("x1", 0.2f) << "f*" << w << ", "
         << p.value("y1", 0.2f) << "f*" << h << ");\n";
      ss << "      path.line_to(" << p.value("x2", 0.8f) << "f*" << w << ", "
         << p.value("y2", 0.8f) << "f*" << h << ");\n";
      ss << "      agg::conv_stroke<agg::path_storage> stroke(path);\n";
      ss << "      stroke.width(" << p.value("thickness", 3.0f) << ");\n";
      ss << "      stroke.line_cap(agg::round_cap); "
            "stroke.line_join(agg::round_join);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl;\n";
      ss << "      AggRendererSolid solid(ren); ras.add_path(stroke);\n";
      std::string col = "{" + std::to_string(p.value("cr", 1.0f)) + "f," +
                        std::to_string(p.value("cg", 1.0f)) + "f," +
                        std::to_string(p.value("cb", 1.0f)) + "f," +
                        std::to_string(p.value("ca", 1.0f)) + "f}";
      ss << "      float col[] = " << col << ";\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    else if (type == "AggCircle") {
      ss << "      agg::ellipse ell(" << p.value("cx", 0.5f) << "f*" << w
         << ", " << p.value("cy", 0.5f) << "f*" << h << ", "
         << p.value("rx", 0.3f) << "f*" << w << ", " << p.value("ry", 0.3f)
         << "f*" << h << ", 100);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      if (p.value("filled", true)) {
        ss << "      ras.add_path(ell); float fc[] = {" << p.value("fr", 1.0f)
           << "f," << p.value("fg", 1.0f) << "f," << p.value("fb", 1.0f) << "f,"
           << p.value("fa", 1.0f) << "f};\n";
        ss << "      solid.color(agg_color(fc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
      if (p.value("stroked", false)) {
        ss << "      ras.reset(); agg::conv_stroke<agg::ellipse> "
              "stroke(ell);\n";
        ss << "      stroke.width(" << p.value("thickness", 3.0f) << ");\n";
        ss << "      ras.add_path(stroke); float sc[] = {"
           << p.value("sr", 1.0f) << "f," << p.value("sg", 1.0f) << "f,"
           << p.value("sb", 1.0f) << "f," << p.value("sa", 1.0f) << "f};\n";
        ss << "      solid.color(agg_color(sc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
    }

    else if (type == "AggRect") {
      ss << "      agg::rounded_rect rect(" << p.value("x1", 0.1f) << "f*" << w
         << ", " << p.value("y1", 0.1f) << "f*" << h << ", "
         << p.value("x2", 0.9f) << "f*" << w << ", " << p.value("y2", 0.9f)
         << "f*" << h << ", " << p.value("cornerRadius", 0.0f) << "f*"
         << std::min(w, h) << "*0.5f);\n";
      ss << "      rect.normalize_radius();\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      if (p.value("filled", true)) {
        ss << "      ras.add_path(rect); float fc[] = {" << p.value("fr", 1.0f)
           << "f," << p.value("fg", 1.0f) << "f," << p.value("fb", 1.0f) << "f,"
           << p.value("fa", 1.0f) << "f};\n";
        ss << "      solid.color(agg_color(fc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
      if (p.value("stroked", false)) {
        ss << "      ras.reset(); agg::conv_stroke<agg::rounded_rect> "
              "stroke(rect);\n";
        ss << "      stroke.width(" << p.value("thickness", 3.0f) << ");\n";
        ss << "      ras.add_path(stroke); float sc[] = {"
           << p.value("sr", 1.0f) << "f," << p.value("sg", 1.0f) << "f,"
           << p.value("sb", 1.0f) << "f," << p.value("sa", 1.0f) << "f};\n";
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
        ss << "      ras.add_path(poly); float fc[] = {" << p.value("fr", 1.0f)
           << "f," << p.value("fg", 1.0f) << "f," << p.value("fb", 1.0f) << "f,"
           << p.value("fa", 1.0f) << "f};\n";
        ss << "      solid.color(agg_color(fc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
      if (p.value("stroked", false)) {
        ss << "      ras.reset(); agg::conv_stroke<agg::path_storage> "
              "stroke(poly);\n";
        ss << "      stroke.width(" << p.value("thickness", 3.0f)
           << "); stroke.line_cap(agg::round_cap);\n";
        ss << "      ras.add_path(stroke); float sc[] = {"
           << p.value("sr", 1.0f) << "f," << p.value("sg", 1.0f) << "f,"
           << p.value("sb", 1.0f) << "f," << p.value("sa", 1.0f) << "f};\n";
        ss << "      solid.color(agg_color(sc)); agg::render_scanlines(ras, "
              "sl, solid);\n";
      }
    }

    else if (type == "AggText") {
      std::string text = p.value("text", "AGG");
      ss << "      agg::gsv_text text; text.size(" << p.value("height", 30.0f)
         << ");\n";
      ss << "      text.start_point(" << p.value("x", 0.1f) << "f*" << w << ", "
         << p.value("y", 0.5f) << "f*" << h << ");\n";
      ss << "      text.text(\"" << text << "\");\n";
      ss << "      agg::conv_stroke<agg::gsv_text> stroke(text);\n";
      ss << "      stroke.width(" << p.value("thickness", 1.5f)
         << "); stroke.line_cap(agg::round_cap);\n";
      ss << "      agg::rasterizer_scanline_aa<> ras; agg::scanline_u8 sl; "
            "AggRendererSolid solid(ren);\n";
      ss << "      ras.add_path(stroke); float col[] = {" << p.value("cr", 1.0f)
         << "f," << p.value("cg", 1.0f) << "f," << p.value("cb", 1.0f) << "f,"
         << p.value("ca", 1.0f) << "f};\n";
      ss << "      solid.color(agg_color(col)); agg::render_scanlines(ras, sl, "
            "solid);\n";
    }

    ss << "    }\n";
  }

  else {
    ss << "    // [unsupported node: " << type << " id " << id << "]\n";
  }
}

bool exportCHeader(NodeGraph* graph,
                   const std::string& name,
                   const std::string& outPath) {
  if (!graph)
    return false;

  // Save current graph state as JSON to walk the topology
  nlohmann::json j = graph->save();
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
