// mm_sdf — 2D signed distance field ports. See mm_sdf.h for attribution.
#include "mm_sdf.h"

#include <cmath>

namespace {

inline sF32 clampf(sF32 v, sF32 lo, sF32 hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}
inline sF32 mixf(sF32 a, sF32 b, sF32 t) { return a + (b - a) * t; }
inline sF32 glslMod(sF32 a, sF32 b) { return a - b * floorf(a / b); }

inline sU16 to16(sF32 v) {
  return (sU16)(clampf(v, 0.0f, 1.0f) * 65535.0f + 0.5f);
}
inline void gray16(Pixel &p, sF32 v) {
  sU16 g = to16(v);
  p.r = p.g = p.b = g;
  p.a = 65535;
}

// Bilinear wrap sampling of the grayscale (red) channel in [0,1].
inline sF32 sampleGray(const GenTexture &t, sF32 u, sF32 v) {
  u = u - floorf(u);
  v = v - floorf(v);
  sF32 fx = u * t.XRes - 0.5f;
  sF32 fy = v * t.YRes - 0.5f;
  sInt x0 = (sInt)floorf(fx);
  sInt y0 = (sInt)floorf(fy);
  sF32 tx = fx - x0;
  sF32 ty = fy - y0;
  auto wrap = [](sInt a, sInt n) {
    a %= n;
    return a < 0 ? a + n : a;
  };
  sInt x1 = wrap(x0 + 1, t.XRes);
  sInt y1 = wrap(y0 + 1, t.YRes);
  x0 = wrap(x0, t.XRes);
  y0 = wrap(y0, t.YRes);
  const sF32 s = 1.0f / 65535.0f;
  sF32 p00 = t.Data[y0 * t.XRes + x0].r * s;
  sF32 p10 = t.Data[y0 * t.XRes + x1].r * s;
  sF32 p01 = t.Data[y1 * t.XRes + x0].r * s;
  sF32 p11 = t.Data[y1 * t.XRes + x1].r * s;
  return mixf(mixf(p00, p10, tx), mixf(p01, p11, tx), ty);
}

inline sF32 sdfSample(const GenTexture &t, sF32 u, sF32 v) {
  return mmSdfDecode(sampleGray(t, u, v));
}

// vec2 sdf2d_rotate(vec2 uv, float a) — rotation around (0.5, 0.5)
inline void sdfRotate(sF32 &u, sF32 &v, sF32 a) {
  sF32 c = cosf(a), s = sinf(a);
  sF32 x = u - 0.5f, y = v - 0.5f;
  u = x * c + y * s + 0.5f;
  v = -x * s + y * c + 0.5f;
}

// ---- shape distance functions (from sd*.mmg / IQ) ----

sF32 sdBox(sF32 px, sF32 py, sF32 sx, sF32 sy) {
  sF32 dx = fabsf(px) - sx, dy = fabsf(py) - sy;
  sF32 mx = dx > 0 ? dx : 0, my = dy > 0 ? dy : 0;
  sF32 inner = dx > dy ? dx : dy;
  return sqrtf(mx * mx + my * my) + (inner < 0 ? inner : 0);
}

sF32 sdLineDist(sF32 px, sF32 py, sF32 ax, sF32 ay, sF32 bx, sF32 by) {
  sF32 pax = px - ax, pay = py - ay;
  sF32 bax = bx - ax, bay = by - ay;
  sF32 dot = pax * bax + pay * bay;
  sF32 len2 = bax * bax + bay * bay;
  sF32 h = len2 > 0 ? clampf(dot / len2, 0.0f, 1.0f) : 0.0f;
  sF32 dx = pax - bax * h, dy = pay - bay * h;
  return sqrtf(dx * dx + dy * dy);
}

// The MIT License, Copyright (c) 2019 Inigo Quilez
// https://www.shadertoy.com/view/3tSGDy
sF32 sdStar(sF32 px, sF32 py, sF32 r, sInt n, sF32 m) {
  sF32 an = 3.141593f / (sF32)n;
  sF32 en = 3.141593f / m;
  sF32 acsx = cosf(an), acsy = sinf(an);
  sF32 ecsx = cosf(en), ecsy = sinf(en);
  sF32 bn = glslMod(atan2f(px, py), 2.0f * an) - an;
  sF32 len = sqrtf(px * px + py * py);
  px = len * cosf(bn);
  py = len * fabsf(sinf(bn));
  px -= r * acsx;
  py -= r * acsy;
  sF32 h = clampf(-(px * ecsx + py * ecsy), 0.0f, r * acsy / ecsy);
  px += ecsx * h;
  py += ecsy * h;
  return sqrtf(px * px + py * py) * (px < 0.0f ? -1.0f : 1.0f);
}

// vec2 circle_repeat_transform_2d(vec2 p, float count)
inline void circleRepeat(sF32 &px, sF32 &py, sF32 count) {
  sF32 r = 6.28318530718f / count;
  sF32 pa = atan2f(px, py);
  sF32 a = glslMod(pa + 0.5f * r, r) - 0.5f * r;
  sF32 c = cosf(a - pa), s = sinf(a - pa);
  sF32 x = px, y = py;
  px = x * c + y * s;
  py = -x * s + y * c;
}

sF32 sdNgon(sF32 px, sF32 py, sF32 r, sF32 n) {
  circleRepeat(px, py, n);
  sF32 dx = fabsf(px) - r * tanf(3.14159265359f / n);
  sF32 dy = fabsf(py) - r;
  if (py < r)
    return py - r;
  sF32 mx = dx > 0 ? dx : 0, my = dy > 0 ? dy : 0;
  sF32 inner = dx > dy ? dx : dy;
  return sqrtf(mx * mx + my * my) + (inner < 0 ? inner : 0);
}

sF32 sdRhombus(sF32 px, sF32 py, sF32 bx, sF32 by) {
  sF32 qx = fabsf(px), qy = fabsf(py);
  auto ndot = [](sF32 ax, sF32 ay, sF32 bx2, sF32 by2) {
    return ax * bx2 - ay * by2;
  };
  sF32 h = clampf((-2.0f * ndot(qx, qy, bx, by) + ndot(bx, by, bx, by)) /
                      (bx * bx + by * by),
                  -1.0f, 1.0f);
  sF32 dx = qx - 0.5f * bx * (1.0f - h);
  sF32 dy = qy - 0.5f * by * (1.0f + h);
  sF32 d = sqrtf(dx * dx + dy * dy);
  sF32 sgn = qx * by + qy * bx - bx * by;
  return d * (sgn < 0.0f ? -1.0f : 1.0f);
}

// ---- smooth boolean ops (IQ polynomial smooth min) ----

sF32 sdSmoothUnion(sF32 d1, sF32 d2, sF32 k) {
  sF32 h = clampf(0.5f + 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
  return mixf(d2, d1, h) - k * h * (1.0f - h);
}
sF32 sdSmoothSubtraction(sF32 d1, sF32 d2, sF32 k) {
  sF32 h = clampf(0.5f - 0.5f * (d2 + d1) / k, 0.0f, 1.0f);
  return mixf(d2, -d1, h) + k * h * (1.0f - h);
}
sF32 sdSmoothIntersection(sF32 d1, sF32 d2, sF32 k) {
  sF32 h = clampf(0.5f - 0.5f * (d2 - d1) / k, 0.0f, 1.0f);
  return mixf(d2, d1, h) + k * h * (1.0f - h);
}

} // namespace

void MMSdfShape(GenTexture &out, const MMSdfShapeParams &p) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 rotRad = p.rot * 0.01745329251f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      sF32 d = 0.0f;
      switch (p.shape) {
      case MMSdfCircle: {
        sF32 dx = u - p.cx, dy = v - p.cy;
        d = sqrtf(dx * dx + dy * dy) - p.w;
        break;
      }
      case MMSdfBox:
        d = sdBox(u - p.cx, v - p.cy, p.w, p.h);
        break;
      case MMSdfLine:
        d = sdLineDist(u, v, p.ax, p.ay, p.bx, p.by) - p.w;
        break;
      case MMSdfStar: {
        // sdStar(rotate(uv-c, rot-90deg) - 0.5, r, n, n+(1-n)*ir)
        sF32 su = u - p.cx + 0.5f, sv = v - p.cy + 0.5f;
        sdfRotate(su, sv, rotRad - 1.57079632679f);
        sF32 m = (sF32)p.n + (1.0f - (sF32)p.n) * clampf(p.ir, 0.0f, 1.0f);
        d = sdStar(su - 0.5f, sv - 0.5f, p.w, p.n, m);
        break;
      }
      case MMSdfNgon: {
        sF32 su = u - p.cx + 0.5f, sv = v - p.cy + 0.5f;
        sdfRotate(su, sv, rotRad - 1.57079632679f);
        d = sdNgon(su - 0.5f, sv - 0.5f, p.w, (sF32)p.n);
        break;
      }
      case MMSdfRhombus:
        d = sdRhombus(u - p.cx, v - p.cy, p.w, p.h);
        break;
      default:
        d = 1.0f;
        break;
      }
      gray16(out.Data[py * w + px], mmSdfEncode(d));
    }
  }
}

void MMSdfOp(GenTexture &out, const GenTexture &a, const GenTexture &b,
             sInt op, sF32 k) {
  if (!out.Data || !a.Data || !b.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 kk = k > 1e-5f ? k : 1e-5f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      sF32 d1 = sdfSample(a, u, v);
      sF32 d2 = sdfSample(b, u, v);
      sF32 d;
      switch (op) {
      case MMSdfUnion:
        d = d1 < d2 ? d1 : d2;
        break;
      case MMSdfSubtraction:
        d = -d1 > d2 ? -d1 : d2;
        break;
      case MMSdfIntersection:
        d = d1 > d2 ? d1 : d2;
        break;
      case MMSdfSmoothUnion:
        d = sdSmoothUnion(d1, d2, kk);
        break;
      case MMSdfSmoothSubtraction:
        d = sdSmoothSubtraction(d1, d2, kk);
        break;
      case MMSdfSmoothIntersection:
        d = sdSmoothIntersection(d1, d2, kk);
        break;
      case MMSdfMorph:
        d = mixf(d1, d2, clampf(k, 0.0f, 1.0f));
        break;
      default:
        d = d1;
        break;
      }
      gray16(out.Data[py * w + px], mmSdfEncode(d));
    }
  }
}

void MMSdfTransform(GenTexture &out, const GenTexture &in, sF32 tx, sF32 ty,
                    sF32 rotDeg, sF32 scale, sF32 roundR, sF32 annularW,
                    sInt annularCount) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 s = scale > 1e-5f ? scale : 1e-5f;
  const sF32 rot = rotDeg * 0.01745329251f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      // translate, rotate around center, scale around center
      u -= tx;
      v -= ty;
      sdfRotate(u, v, rot);
      u = (u - 0.5f) / s + 0.5f;
      v = (v - 0.5f) / s + 0.5f;
      sF32 d = sdfSample(in, u, v) * s;
      d -= roundR;
      for (sInt i = 0; i < annularCount; i++)
        d = fabsf(d) - annularW;
      gray16(out.Data[py * w + px], mmSdfEncode(d));
    }
  }
}

void MMSdfShow(GenTexture &out, const GenTexture &in, sF32 base, sF32 bevel) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 bv = bevel > 0.00001f ? bevel : 0.00001f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 d = sdfSample(in, (px + 0.5f) / w, (py + 0.5f) / h);
      gray16(out.Data[py * w + px], clampf(base - d / bv, 0.0f, 1.0f));
    }
  }
}
