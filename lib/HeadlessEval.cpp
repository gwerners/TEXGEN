#include "HeadlessEval.h"
#include "texgen.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <queue>
#include <vector>

static int sizeFromIdx(int idx) { return 32 << idx; }

static sU32 colorToU32(float r, float g, float b, float a) {
  sU32 ar = (sU32)(r * 255.0f + 0.5f);
  sU32 ag = (sU32)(g * 255.0f + 0.5f);
  sU32 ab = (sU32)(b * 255.0f + 0.5f);
  sU32 aa = (sU32)(a * 255.0f + 0.5f);
  return (aa << 24) | (ar << 16) | (ag << 8) | ab;
}

static sU32 colorFromParams(const nlohmann::json &p, const char *r,
                            const char *g, const char *b, const char *a) {
  return colorToU32(p.value(r, 0.0f), p.value(g, 0.0f), p.value(b, 0.0f),
                    p.value(a, 1.0f));
}

static sU32 colorFromArray(const nlohmann::json &arr) {
  if (!arr.is_array() || arr.size() < 4)
    return 0xff000000;
  return colorToU32(arr[0].get<float>(), arr[1].get<float>(),
                    arr[2].get<float>(), arr[3].get<float>());
}

// Find source node id for a given input slot
static int findSource(const nlohmann::json &conns, int nodeId,
                      const std::string &slot) {
  for (auto &c : conns) {
    if (c["toId"].get<int>() == nodeId &&
        c["toSlot"].get<std::string>() == slot)
      return c["fromId"].get<int>();
  }
  return -1;
}

static GenTexture *getInput(std::map<int, GenTexture> &cache,
                            const nlohmann::json &conns, int nodeId,
                            const std::string &slot) {
  int src = findSource(conns, nodeId, slot);
  if (src >= 0) {
    auto it = cache.find(src);
    if (it != cache.end() && it->second.Data)
      return &it->second;
  }
  return nullptr;
}

static void execNode(const std::string &type, int id,
                     const nlohmann::json &p, const nlohmann::json &conns,
                     std::map<int, GenTexture> &cache) {
  auto *bg = getInput(cache, conns, id, "Bg");
  auto *in = getInput(cache, conns, id, "In");

  if (type == "Color") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    sU32 col = colorFromParams(p, "r", "g", "b", "a");
    cache[id].Init(w, h);
    for (int i = 0; i < cache[id].NPixels; i++)
      cache[id].Data[i].Init(col);
  }

  else if (type == "Noise") {
    int sz = sizeFromIdx(p.value("sizeIdx", 3));
    sU32 c1 = colorFromParams(p, "c1r", "c1g", "c1b", "c1a");
    sU32 c2 = colorFromParams(p, "c2r", "c2g", "c2b", "c2a");
    GenTexture grad = LinearGradient(c1, c2);
    cache[id].Init(sz, sz);
    cache[id].Noise(grad, p.value("freqX", 2), p.value("freqY", 2),
                    p.value("oct", 4), p.value("fadeoff", 0.5f),
                    p.value("seed", 0), p.value("mode", 0));
  }

  else if (type == "Crystal") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    sU32 cn = p.contains("colorNear") ? colorFromArray(p["colorNear"])
                                      : 0xffffffff;
    sU32 cf =
        p.contains("colorFar") ? colorFromArray(p["colorFar"]) : 0xff000000;
    cache[id].Init(w, h);
    Crystal(cache[id], (sU32)p.value("seed", 0), p.value("count", 16), cn, cf);
  }

  else if (type == "Bricks") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    sU32 c0 = colorFromParams(p, "col0r", "col0g", "col0b", "col0a");
    sU32 c1 = colorFromParams(p, "col1r", "col1g", "col1b", "col1a");
    sU32 cf = colorFromParams(p, "colFuger", "colFugeg", "colFugeb", "colFugea");
    cache[id].Init(w, h);
    Bricks(cache[id], c0, c1, cf, p.value("fugeX", 0.03f),
           p.value("fugeY", 0.06f), p.value("tileX", 4), p.value("tileY", 8),
           p.value("seed", 0), p.value("heads", 2),
           p.value("colorBalance", 0.5f));
  }

  else if (type == "DirectionalGradient") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    sU32 c1 = colorFromParams(p, "c1r", "c1g", "c1b", "c1a");
    sU32 c2 = colorFromParams(p, "c2r", "c2g", "c2b", "c2a");
    cache[id].Init(w, h);
    DirectionalGradient(cache[id], p.value("x1", 0.0f), p.value("y1", 0.0f),
                        p.value("x2", 1.0f), p.value("y2", 1.0f), c1, c2);
  }

  else if (type == "GlowEffect") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    auto bgCol = p.value("bgCol", nlohmann::json::array({0, 0, 0, 1}));
    auto glCol = p.value("glowCol", nlohmann::json::array({1, 1, 1, 1}));
    cache[id].Init(w, h);
    GlowEffect(cache[id], p.value("cx", 0.5f), p.value("cy", 0.5f),
               p.value("scale", 1.0f), p.value("exponent", 2.0f),
               p.value("intensity", 1.0f), colorFromArray(bgCol),
               colorFromArray(glCol));
  }

  else if (type == "PerlinNoiseRG2") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    sU32 c1 = colorFromParams(p, "c1r", "c1g", "c1b", "c1a");
    sU32 c2 = colorFromParams(p, "c2r", "c2g", "c2b", "c2a");
    cache[id].Init(w, h);
    PerlinNoiseRG2(cache[id], p.value("octaves", 4),
                   p.value("persistence", 0.5f), p.value("freqScale", 2),
                   p.value("seed", 0), p.value("contrast", 1.0f), c1, c2,
                   p.value("startOctave", 0));
  }

  else if (type == "Gradient") {
    sU32 c1 = colorFromParams(p, "c1r", "c1g", "c1b", "c1a");
    sU32 c2 = colorFromParams(p, "c2r", "c2g", "c2b", "c2a");
    cache[id] = LinearGradient(c1, c2);
  }

  else if (type == "Blur") {
    if (in && in->Data) {
      cache[id].Init(in->XRes, in->YRes);
      cache[id].Blur(*in, p.value("sizex", 0.01f), p.value("sizey", 0.01f),
                      p.value("order", 2), p.value("mode", 0));
    }
  }

  else if (type == "BlurKernel") {
    if (in && in->Data) {
      cache[id].Init(in->XRes, in->YRes);
      BlurKernel(cache[id], *in, p.value("radiusX", 0.01f),
                 p.value("radiusY", 0.01f), p.value("kernelType", 2),
                 p.value("wrapMode", 0));
    }
  }

  else if (type == "HSCB") {
    if (in && in->Data) {
      cache[id].Init(in->XRes, in->YRes);
      HSCB(cache[id], *in, p.value("hue", 0.0f), p.value("sat", 1.0f),
           p.value("contrast", 1.0f), p.value("brightness", 1.0f));
    }
  }

  else if (type == "Derive") {
    if (in && in->Data) {
      cache[id].Init(in->XRes, in->YRes);
      cache[id].Derive(*in, (GenTexture::DeriveOp)p.value("op", 0), p.value("strength", 1.0f));
    }
  }

  else if (type == "Wavelet") {
    if (in && in->Data) {
      cache[id] = *in;
      Wavelet(cache[id], p.value("mode", 0), p.value("count", 1));
    }
  }

  else if (type == "ColorBalance") {
    if (in && in->Data) {
      auto sh = p.value("shadow", nlohmann::json::array({0, 0, 0}));
      auto md = p.value("mid", nlohmann::json::array({0, 0, 0}));
      auto hl = p.value("highlight", nlohmann::json::array({0, 0, 0}));
      cache[id].Init(in->XRes, in->YRes);
      ColorBalance(cache[id], *in, sh[0], sh[1], sh[2], md[0], md[1], md[2],
                   hl[0], hl[1], hl[2]);
    }
  }

  else if (type == "Paste") {
    auto *bgp = getInput(cache, conns, id, "Background");
    auto *snp = getInput(cache, conns, id, "Snippet");
    if (bgp && bgp->Data && snp && snp->Data) {
      cache[id].Init(bgp->XRes, bgp->YRes);
      cache[id].Paste(*bgp, *snp, p.value("orgx", 0.0f),
                       p.value("orgy", 0.0f), p.value("ux", 1.0f),
                       p.value("uy", 0.0f), p.value("vx", 0.0f),
                       p.value("vy", 1.0f), (GenTexture::CombineOp)p.value("op", 0),
                       p.value("mode", 0));
    }
  }

  else if (type == "Ternary") {
    auto *i1 = getInput(cache, conns, id, "Image1");
    auto *i2 = getInput(cache, conns, id, "Image2");
    auto *mask = getInput(cache, conns, id, "Mask");
    if (i1 && i1->Data) {
      cache[id].Init(i1->XRes, i1->YRes);
      GenTexture &i2r = (i2 && i2->Data) ? *i2 : *i1;
      GenTexture &mr = (mask && mask->Data) ? *mask : *i1;
      cache[id].Ternary(*i1, i2r, mr, (GenTexture::TernaryOp)p.value("op", 0));
    }
  }

  else if (type == "LinearCombine") {
    auto *i1 = getInput(cache, conns, id, "Image1");
    if (i1 && i1->Data) {
      cache[id].Init(i1->XRes, i1->YRes);
      auto cc = p.value("constColor", nlohmann::json::array({0, 0, 0, 1}));
      Pixel cp;
      cp.Init(colorFromArray(cc));
      auto weights = p.value("weights", nlohmann::json::array({1, 0, 0, 0}));
      auto uShift = p.value("uShift", nlohmann::json::array({0, 0, 0, 0}));
      auto vShift = p.value("vShift", nlohmann::json::array({0, 0, 0, 0}));
      auto fMode = p.value("filterMode", nlohmann::json::array({0, 0, 0, 0}));
      std::string slots[] = {"Image1", "Image2", "Image3", "Image4"};
      LinearInput li[4] = {};
      int count = 0;
      for (int i = 0; i < 4; i++) {
        auto *inp = getInput(cache, conns, id, slots[i]);
        if (inp && inp->Data) {
          li[i].Tex = inp;
          li[i].Weight = weights[i].get<float>();
          li[i].UShift = uShift[i].get<float>();
          li[i].VShift = vShift[i].get<float>();
          li[i].FilterMode = fMode[i].get<int>();
          count = i + 1;
        }
      }
      cache[id].LinearCombine(cp, p.value("constWeight", 0.0f), li, count);
    }
  }

  // AGG vector nodes
  else if (type == "AggLine" || type == "AggCircle" || type == "AggRect" ||
           type == "AggPolygon" || type == "AggText" || type == "AggArc" ||
           type == "AggBezier" || type == "AggDashLine" ||
           type == "AggGradient") {
    int w = sizeFromIdx(p.value("widthIdx", 3));
    int h = sizeFromIdx(p.value("heightIdx", 3));
    cache[id].Init(w, h);
    // Copy background for Over blend (mode 0)
    int blendMode = p.value("blendMode", 0);
    if (blendMode == 0 && bg && bg->Data && bg->XRes == w && bg->YRes == h)
      memcpy(cache[id].Data, bg->Data, w * h * sizeof(Pixel));
    else
      memset(cache[id].Data, 0, w * h * sizeof(Pixel));

    auto rbuf = agg_rbuf_from(cache[id]);
    AggPixfmt pixf(rbuf);
    AggRendererBase ren(pixf);

    auto renderSolid = [&](auto &path, const float col[4]) {
      agg::rasterizer_scanline_aa<> ras;
      agg::scanline_u8 sl;
      AggRendererSolid solid(ren);
      ras.add_path(path);
      solid.color(agg_color(col));
      agg::render_scanlines(ras, sl, solid);
    };

    auto renderStroke = [&](auto &path, const float col[4], double thick) {
      agg::conv_stroke<std::remove_reference_t<decltype(path)>> stroke(path);
      stroke.width(thick);
      stroke.line_cap(agg::round_cap);
      stroke.line_join(agg::round_join);
      agg::rasterizer_scanline_aa<> ras;
      agg::scanline_u8 sl;
      AggRendererSolid solid(ren);
      ras.add_path(stroke);
      solid.color(agg_color(col));
      agg::render_scanlines(ras, sl, solid);
    };

    if (type == "AggLine") {
      agg::path_storage path;
      path.move_to(p.value("x1", 0.2f) * w, aggY(p.value("y1", 0.2f), h));
      path.line_to(p.value("x2", 0.8f) * w, aggY(p.value("y2", 0.8f), h));
      float col[] = {p.value("cr", 1.0f), p.value("cg", 1.0f),
                     p.value("cb", 1.0f), p.value("ca", 1.0f)};
      renderStroke(path, col, p.value("thickness", 3.0f));
    }

    else if (type == "AggCircle") {
      agg::ellipse ell(p.value("cx", 0.5f) * w, aggY(p.value("cy", 0.5f), h),
                       p.value("rx", 0.3f) * w, p.value("ry", 0.3f) * h, 100);
      float fc[] = {p.value("fr", 1.0f), p.value("fg", 1.0f),
                    p.value("fb", 1.0f), p.value("fa", 1.0f)};
      float sc[] = {p.value("sr", 1.0f), p.value("sg", 1.0f),
                    p.value("sb", 1.0f), p.value("sa", 1.0f)};
      if (p.value("filled", true)) renderSolid(ell, fc);
      if (p.value("stroked", false)) renderStroke(ell, sc, p.value("thickness", 3.0f));
    }

    else if (type == "AggRect") {
      agg::rounded_rect rect(
          p.value("x1", 0.1f) * w, aggY(p.value("y1", 0.1f), h),
          p.value("x2", 0.9f) * w, aggY(p.value("y2", 0.9f), h),
          p.value("cornerRadius", 0.0f) * sMin(w, h) * 0.5);
      rect.normalize_radius();
      float fc[] = {p.value("fr", 1.0f), p.value("fg", 1.0f),
                    p.value("fb", 1.0f), p.value("fa", 1.0f)};
      float sc[] = {p.value("sr", 1.0f), p.value("sg", 1.0f),
                    p.value("sb", 1.0f), p.value("sa", 1.0f)};
      if (p.value("filled", true)) renderSolid(rect, fc);
      if (p.value("stroked", false)) renderStroke(rect, sc, p.value("thickness", 3.0f));
    }

    else if (type == "AggPolygon") {
      int sides = p.value("sides", 6);
      float innerR = p.value("innerRadius", 1.0f);
      bool isStar = innerR < 0.999f;
      int n = isStar ? sides * 2 : sides;
      double cx = p.value("cx", 0.5f) * w, cy = aggY(p.value("cy", 0.5f), h);
      double r = p.value("radius", 0.4f) * sMin(w, h) * 0.5;
      double ri = r * innerR;
      double rot = p.value("rotation", 0.0f) * sPI / 180.0;
      agg::path_storage poly;
      for (int i = 0; i < n; i++) {
        double a = rot + (2.0 * sPI * i) / n - sPI * 0.5;
        double rd = (isStar && (i & 1)) ? ri : r;
        if (i == 0) poly.move_to(cx + cos(a) * rd, cy + sin(a) * rd);
        else poly.line_to(cx + cos(a) * rd, cy + sin(a) * rd);
      }
      poly.close_polygon();
      float fc[] = {p.value("fr", 1.0f), p.value("fg", 1.0f),
                    p.value("fb", 1.0f), p.value("fa", 1.0f)};
      float sc[] = {p.value("sr", 1.0f), p.value("sg", 1.0f),
                    p.value("sb", 1.0f), p.value("sa", 1.0f)};
      if (p.value("filled", true)) renderSolid(poly, fc);
      if (p.value("stroked", false)) renderStroke(poly, sc, p.value("thickness", 3.0f));
    }

    else if (type == "AggText") {
      agg::gsv_text text;
      text.size(p.value("height", 30.0f));
      text.start_point(p.value("x", 0.1f) * w, aggY(p.value("y", 0.5f), h));
      std::string str = p.value("text", "AGG");
      text.text(str.c_str());
      float col[] = {p.value("cr", 1.0f), p.value("cg", 1.0f),
                     p.value("cb", 1.0f), p.value("ca", 1.0f)};
      renderStroke(text, col, p.value("thickness", 1.5f));
    }

    else if (type == "AggArc") {
      double a1 = p.value("angle1", 0.0f) * sPI / 180.0;
      double a2 = p.value("angle2", 270.0f) * sPI / 180.0;
      agg::arc a(p.value("cx", 0.5f) * w, aggY(p.value("cy", 0.5f), h),
                 p.value("rx", 0.3f) * w, p.value("ry", 0.3f) * h, a1, a2);
      float col[] = {p.value("cr", 1.0f), p.value("cg", 1.0f),
                     p.value("cb", 1.0f), p.value("ca", 1.0f)};
      renderStroke(a, col, p.value("thickness", 3.0f));
    }

    else if (type == "AggBezier") {
      agg::curve4 curve(
          p.value("x1", 0.1f) * w, aggY(p.value("y1", 0.5f), h),
          p.value("cx1", 0.3f) * w, aggY(p.value("cy1", 0.1f), h),
          p.value("cx2", 0.7f) * w, aggY(p.value("cy2", 0.9f), h),
          p.value("x2", 0.9f) * w, aggY(p.value("y2", 0.5f), h));
      float col[] = {p.value("cr", 1.0f), p.value("cg", 1.0f),
                     p.value("cb", 1.0f), p.value("ca", 1.0f)};
      renderStroke(curve, col, p.value("thickness", 3.0f));
    }

    else if (type == "AggDashLine") {
      agg::path_storage lp;
      lp.move_to(p.value("x1", 0.1f) * w, aggY(p.value("y1", 0.5f), h));
      lp.line_to(p.value("x2", 0.9f) * w, aggY(p.value("y2", 0.5f), h));
      agg::conv_dash<agg::path_storage> dash(lp);
      dash.add_dash(p.value("dashLen", 15.0f), p.value("gapLen", 10.0f));
      float col[] = {p.value("cr", 1.0f), p.value("cg", 1.0f),
                     p.value("cb", 1.0f), p.value("ca", 1.0f)};
      // conv_dash needs conv_stroke on top
      agg::conv_stroke<agg::conv_dash<agg::path_storage>> stroke(dash);
      stroke.width(p.value("thickness", 3.0f));
      stroke.line_cap(agg::round_cap);
      agg::rasterizer_scanline_aa<> ras;
      agg::scanline_u8 sl;
      AggRendererSolid solid(ren);
      ras.add_path(stroke);
      solid.color(agg_color(col));
      agg::render_scanlines(ras, sl, solid);
    }

    else if (type == "AggGradient") {
      // Simple per-pixel gradient (same as CExport)
      agg::rgba16 c1 = agg_color(
          (float[]){p.value("c1r", 0.0f), p.value("c1g", 0.0f),
                    p.value("c1b", 0.0f), p.value("c1a", 1.0f)});
      agg::rgba16 c2 = agg_color(
          (float[]){p.value("c2r", 1.0f), p.value("c2g", 1.0f),
                    p.value("c2b", 1.0f), p.value("c2a", 1.0f)});
      int gtype = p.value("type", 0);
      if (gtype == 0) {
        double gx1 = p.value("x1", 0.0f) * w, gy1 = aggY(p.value("y1", 0.5f), h);
        double gx2 = p.value("x2", 1.0f) * w, gy2 = aggY(p.value("y2", 0.5f), h);
        double dx = gx2 - gx1, dy = gy2 - gy1;
        double len = sqrt(dx * dx + dy * dy);
        if (len < 1.0) len = 1.0;
        for (int y = 0; y < h; y++)
          for (int x = 0; x < w; x++) {
            double proj = ((x - gx1) * dx + (y - gy1) * dy) / (len * len);
            if (proj < 0) proj = 0;
            if (proj > 1) proj = 1;
            agg::rgba16 c = c1.gradient(c2, proj);
            cache[id].Data[y * w + x].Init(
                (sU32)((c.a >> 8) << 24 | (c.r >> 8) << 16 | (c.g >> 8) << 8 | (c.b >> 8)));
          }
      } else {
        double gcx = p.value("x1", 0.5f) * w, gcy = aggY(p.value("y1", 0.5f), h);
        double gex = p.value("x2", 0.9f) * w, gey = aggY(p.value("y2", 0.5f), h);
        double gr = sqrt((gex - gcx) * (gex - gcx) + (gey - gcy) * (gey - gcy));
        if (gr < 1.0) gr = 1.0;
        for (int y = 0; y < h; y++)
          for (int x = 0; x < w; x++) {
            double d = sqrt((x - gcx) * (x - gcx) + (y - gcy) * (y - gcy)) / gr;
            if (d > 1.0) d = 1.0;
            agg::rgba16 c = c1.gradient(c2, d);
            cache[id].Data[y * w + x].Init(
                (sU32)((c.a >> 8) << 24 | (c.r >> 8) << 16 | (c.g >> 8) << 8 | (c.b >> 8)));
          }
      }
    }

    // Apply non-Over blend
    if (blendMode > 0 && bg && bg->Data && bg->SizeMatchesWith(cache[id])) {
      static const GenTexture::CombineOp ops[] = {
          GenTexture::CombineOver,    GenTexture::CombineAdd,
          GenTexture::CombineSub,     GenTexture::CombineMulC,
          GenTexture::CombineMin,     GenTexture::CombineMax,
          GenTexture::CombineOver,    GenTexture::CombineMultiply,
          GenTexture::CombineScreen,  GenTexture::CombineDarken,
          GenTexture::CombineLighten,
      };
      if (blendMode < (int)(sizeof(ops) / sizeof(ops[0]))) {
        GenTexture result;
        result.Init(w, h);
        result.Paste(*bg, cache[id], 0, 0, 1, 0, 0, 1, ops[blendMode], 0);
        cache[id] = result;
      }
    }
  }

  else if (type == "Output") {
    // Output just passes through — handled in headlessEvaluate
  }
}

bool headlessEvaluate(const nlohmann::json &project, GenTexture &output) {
  auto &nodes = project["nodes"];
  auto &conns = project["connections"];

  // Build id → params map
  std::map<int, const nlohmann::json *> nodeMap;
  for (auto &n : nodes)
    nodeMap[n["id"].get<int>()] = &n;

  // Topological sort (Kahn's)
  std::map<int, int> inDeg;
  std::map<int, std::vector<int>> adj;
  for (auto &[id, _] : nodeMap) {
    inDeg[id] = 0;
    adj[id] = {};
  }
  for (auto &c : conns) {
    int from = c["fromId"].get<int>();
    int to = c["toId"].get<int>();
    adj[from].push_back(to);
    inDeg[to]++;
  }
  std::vector<int> sorted;
  std::queue<int> q;
  for (auto &[id, d] : inDeg)
    if (d == 0) q.push(id);
  while (!q.empty()) {
    int cur = q.front();
    q.pop();
    sorted.push_back(cur);
    for (int next : adj[cur])
      if (--inDeg[next] == 0) q.push(next);
  }
  for (auto &[id, _] : nodeMap) {
    bool found = false;
    for (int s : sorted)
      if (s == id) { found = true; break; }
    if (!found) sorted.push_back(id);
  }

  // Execute
  std::map<int, GenTexture> cache;
  int outputSrc = -1;
  for (int id : sorted) {
    auto it = nodeMap.find(id);
    if (it == nodeMap.end()) continue;
    auto &n = *(it->second);
    std::string type = n["typeName"].get<std::string>();
    nlohmann::json params = n.value("params", nlohmann::json::object());
    execNode(type, id, params, conns, cache);

    if (type == "Output") {
      int src = findSource(conns, id, "In");
      if (src >= 0) outputSrc = src;
    }
  }

  if (outputSrc >= 0 && cache.count(outputSrc) && cache[outputSrc].Data) {
    output = cache[outputSrc];
    return true;
  }
  return false;
}
