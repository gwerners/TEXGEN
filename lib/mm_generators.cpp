// mm_generators — CPU ports of Material Maker procedural generators.
// See mm_generators.h for licensing/attribution.
#include "mm_generators.h"

#include <cmath>

// ============================================================
// GLSL-compatible helpers (float precision, matching MM shaders)
// ============================================================

namespace {

struct Vec2 {
  sF32 x, y;
};

inline sF32 glslFract(sF32 v) { return v - floorf(v); }
inline sF32 glslMod(sF32 a, sF32 b) { return a - b * floorf(a / b); }
inline sF32 mixf(sF32 a, sF32 b, sF32 t) { return a + (b - a) * t; }
inline sF32 clamp01(sF32 v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline sF32 clampf(sF32 v, sF32 lo, sF32 hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// float rand(vec2 x) { return fract(cos(mod(dot(x, vec2(13.9898, 8.141)),
//                                            3.14)) * 43758.5); }
inline sF32 mmRand(sF32 x, sF32 y) {
  return glslFract(cosf(glslMod(x * 13.9898f + y * 8.141f, 3.14f)) * 43758.5f);
}

// vec2 rand2(vec2 x)
inline Vec2 mmRand2(sF32 x, sF32 y) {
  return {glslFract(cosf(glslMod(x * 13.9898f + y * 8.141f, 3.14f)) *
                    43758.5f),
          glslFract(cosf(glslMod(x * 3.4562f + y * 17.398f, 3.14f)) *
                    43758.5f)};
}

// vec3 rand3(vec2 x)
inline void mmRand3(sF32 x, sF32 y, sF32 out[3]) {
  out[0] =
      glslFract(cosf(glslMod(x * 13.9898f + y * 8.141f, 3.14f)) * 43758.5f);
  out[1] =
      glslFract(cosf(glslMod(x * 3.4562f + y * 17.398f, 3.14f)) * 43758.5f);
  out[2] =
      glslFract(cosf(glslMod(x * 13.254f + y * 5.867f, 3.14f)) * 43758.5f);
}

inline sU16 to16(sF32 v) { return (sU16)(clamp01(v) * 65535.0f + 0.5f); }

inline void gray16(Pixel &p, sF32 v) {
  sU16 g = to16(v);
  p.r = p.g = p.b = g;
  p.a = 65535;
}

// Grayscale value of a pixel in [0,1] (RGB average, MM rgba->f convention).
inline sF32 lumOf(const Pixel &p) {
  return ((sF32)p.r + (sF32)p.g + (sF32)p.b) / (3.0f * 65535.0f);
}

// Bilinear wrap sampling, normalized coords, channel results in [0,1].
inline void sampleBilinearWrap(const GenTexture &t, sF32 u, sF32 v,
                               sF32 out[4]) {
  u = glslFract(u);
  v = glslFract(v);
  sF32 fx = u * t.XRes - 0.5f;
  sF32 fy = v * t.YRes - 0.5f;
  sInt x0 = (sInt)floorf(fx);
  sInt y0 = (sInt)floorf(fy);
  sF32 tx = fx - x0;
  sF32 ty = fy - y0;
  sInt x1 = x0 + 1;
  sInt y1 = y0 + 1;
  auto wrap = [](sInt a, sInt n) {
    a %= n;
    return a < 0 ? a + n : a;
  };
  x0 = wrap(x0, t.XRes);
  x1 = wrap(x1, t.XRes);
  y0 = wrap(y0, t.YRes);
  y1 = wrap(y1, t.YRes);
  const Pixel &p00 = t.Data[y0 * t.XRes + x0];
  const Pixel &p10 = t.Data[y0 * t.XRes + x1];
  const Pixel &p01 = t.Data[y1 * t.XRes + x0];
  const Pixel &p11 = t.Data[y1 * t.XRes + x1];
  const sF32 s = 1.0f / 65535.0f;
  sF32 c00[4] = {p00.r * s, p00.g * s, p00.b * s, p00.a * s};
  sF32 c10[4] = {p10.r * s, p10.g * s, p10.b * s, p10.a * s};
  sF32 c01[4] = {p01.r * s, p01.g * s, p01.b * s, p01.a * s};
  sF32 c11[4] = {p11.r * s, p11.g * s, p11.b * s, p11.a * s};
  for (int i = 0; i < 4; i++)
    out[i] = mixf(mixf(c00[i], c10[i], tx), mixf(c01[i], c11[i], tx), ty);
}

inline sF32 sampleGrayBilinearWrap(const GenTexture &t, sF32 u, sF32 v) {
  sF32 c[4];
  sampleBilinearWrap(t, u, v, c);
  return (c[0] + c[1] + c[2]) / 3.0f;
}

inline void u32ToF4(sU32 col, sF32 out[4]) {
  out[0] = ((col >> 16) & 0xff) / 255.0f;
  out[1] = ((col >> 8) & 0xff) / 255.0f;
  out[2] = (col & 0xff) / 255.0f;
  out[3] = ((col >> 24) & 0xff) / 255.0f;
}

} // namespace

// ============================================================
// Voronoi (iq_voronoi from voronoi.mmg)
// ============================================================

void MMVoronoi(GenTexture *outColor, GenTexture *outF1, GenTexture *outEdge,
               sInt scaleX, sInt scaleY, sF32 stretchX, sF32 stretchY,
               sF32 intensity, sF32 randomness, sF32 seed,
               GenTexture *outFill) {
  GenTexture *ref = outColor ? outColor : (outF1 ? outF1 : outEdge);
  if (!ref || !ref->Data)
    return;
  const sInt w = ref->XRes, h = ref->YRes;
  const sF32 sizeX = (sF32)scaleX, sizeY = (sF32)scaleY;
  // MM: voronoi() receives vec2($stretch_y, $stretch_x)
  const sF32 stX = stretchY, stY = stretchX;
  Vec2 seedV = mmRand2(seed, 1.0f - seed);

  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 x = ((px + 0.5f) / w) * sizeX;
      sF32 y = ((py + 0.5f) / h) * sizeY;
      sF32 nx = floorf(x), ny = floorf(y);
      sF32 fx = x - nx, fy = y - ny;

      // pass 1: closest center
      sF32 md = 8.0f, mgx = 0, mgy = 0, mrx = 0, mry = 0, mcx = 0, mcy = 0;
      for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
          sF32 gx = (sF32)i, gy = (sF32)j;
          Vec2 o = mmRand2(seedV.x + glslMod(nx + gx + sizeX, sizeX),
                           seedV.y + glslMod(ny + gy + sizeY, sizeY));
          sF32 cx = gx + randomness * o.x;
          sF32 cy = gy + randomness * o.y;
          sF32 rx = cx - fx, ry = cy - fy;
          sF32 rrx = rx * stX, rry = ry * stY;
          sF32 d = rrx * rrx + rry * rry;
          if (d < md) {
            md = d;
            mcx = cx;
            mcy = cy;
            mrx = rx;
            mry = ry;
            mgx = gx;
            mgy = gy;
          }
        }

      // pass 2: distance to borders
      sF32 edge = 8.0f;
      for (int j = -2; j <= 2; j++)
        for (int i = -2; i <= 2; i++) {
          sF32 gx = mgx + (sF32)i, gy = mgy + (sF32)j;
          Vec2 o = mmRand2(seedV.x + glslMod(nx + gx + sizeX, sizeX),
                           seedV.y + glslMod(ny + gy + sizeY, sizeY));
          sF32 rx = gx + randomness * o.x - fx;
          sF32 ry = gy + randomness * o.y - fy;
          sF32 drx = (mrx - rx) * stX, dry = (mry - ry) * stY;
          if (drx * drx + dry * dry > 0.00001f) {
            sF32 dx = (rx - mrx) * stX, dy = (ry - mry) * stY;
            sF32 len = sqrtf(dx * dx + dy * dy);
            sF32 mx = 0.5f * (mrx + rx) * stX, my = 0.5f * (mry + ry) * stY;
            sF32 d = mx * (dx / len) + my * (dy / len);
            if (d < edge)
              edge = d;
          }
        }

      sF32 cellX = mcx + nx, cellY = mcy + ny;
      sInt idx = py * w + px;

      if (outF1 && outF1->Data) {
        sF32 dx = (x - cellX) * stX, dy = (y - cellY) * stY;
        gray16(outF1->Data[idx], intensity * sqrtf(dx * dx + dy * dy));
      }
      if (outEdge && outEdge->Data)
        gray16(outEdge->Data[idx], edge);
      if (outColor && outColor->Data) {
        sF32 rgb[3];
        mmRand3(glslFract(floorf(cellX) / sizeX),
                glslFract(floorf(cellY) / sizeY), rgb);
        Pixel &p = outColor->Data[idx];
        p.r = to16(rgb[0]);
        p.g = to16(rgb[1]);
        p.b = to16(rgb[2]);
        p.a = 65535;
      }
      if (outFill && outFill->Data) {
        // MM: round(vec4(fract((cell-1)/size), 2/size) * 4096) / 4096
        auto q = [](sF32 v) { return roundf(v * 4096.0f) / 4096.0f; };
        Pixel &p = outFill->Data[idx];
        p.r = to16(q(glslFract((cellX - 1.0f) / sizeX)));
        p.g = to16(q(glslFract((cellY - 1.0f) / sizeY)));
        p.b = to16(q(2.0f / sizeX));
        p.a = to16(q(2.0f / sizeY));
      }
    }
  }
}

// ============================================================
// FBM (fbm.mmg)
// ============================================================

namespace {

sF32 fbmValue(sF32 cx, sF32 cy, sF32 sizeX, sF32 sizeY, sF32 seed) {
  Vec2 sv = mmRand2(seed, 1.0f - seed);
  sF32 ox = floorf(cx) + sv.x + sizeX;
  sF32 oy = floorf(cy) + sv.y + sizeY;
  sF32 fx = glslFract(cx), fy = glslFract(cy);
  sF32 p00 = mmRand(glslMod(ox, sizeX), glslMod(oy, sizeY));
  sF32 p01 = mmRand(glslMod(ox, sizeX), glslMod(oy + 1.0f, sizeY));
  sF32 p10 = mmRand(glslMod(ox + 1.0f, sizeX), glslMod(oy, sizeY));
  sF32 p11 = mmRand(glslMod(ox + 1.0f, sizeX), glslMod(oy + 1.0f, sizeY));
  sF32 tx = fx * fx * (3.0f - 2.0f * fx);
  sF32 ty = fy * fy * (3.0f - 2.0f * fy);
  return mixf(mixf(p00, p10, tx), mixf(p01, p11, tx), ty);
}

sF32 fbmPerlin(sF32 cx, sF32 cy, sF32 sizeX, sF32 sizeY, sF32 seed) {
  Vec2 sv = mmRand2(seed, 1.0f - seed);
  sF32 ox = floorf(cx) + sv.x + sizeX;
  sF32 oy = floorf(cy) + sv.y + sizeY;
  sF32 fx = glslFract(cx), fy = glslFract(cy);
  const sF32 tau = 6.28318530718f;
  sF32 a00 = mmRand(glslMod(ox, sizeX), glslMod(oy, sizeY)) * tau;
  sF32 a01 = mmRand(glslMod(ox, sizeX), glslMod(oy + 1.0f, sizeY)) * tau;
  sF32 a10 = mmRand(glslMod(ox + 1.0f, sizeX), glslMod(oy, sizeY)) * tau;
  sF32 a11 =
      mmRand(glslMod(ox + 1.0f, sizeX), glslMod(oy + 1.0f, sizeY)) * tau;
  sF32 p00 = cosf(a00) * fx + sinf(a00) * fy;
  sF32 p01 = cosf(a01) * fx + sinf(a01) * (fy - 1.0f);
  sF32 p10 = cosf(a10) * (fx - 1.0f) + sinf(a10) * fy;
  sF32 p11 = cosf(a11) * (fx - 1.0f) + sinf(a11) * (fy - 1.0f);
  sF32 tx = fx * fx * (3.0f - 2.0f * fx);
  sF32 ty = fy * fy * (3.0f - 2.0f * fy);
  return 0.5f + mixf(mixf(p00, p10, tx), mixf(p01, p11, tx), ty);
}

// dist metrics: 0 euclidean, 1 manhattan, 2 chebyshev; f2f1: F2-F1 variant
sF32 fbmCellular(sF32 cx, sF32 cy, sF32 sizeX, sF32 sizeY, sF32 seed,
                 int metric, bool f2f1, sF32 nodeScale) {
  Vec2 sv = mmRand2(seed, 1.0f - seed);
  sF32 ox = floorf(cx) + sv.x + sizeX;
  sF32 oy = floorf(cy) + sv.y + sizeY;
  sF32 fx = glslFract(cx), fy = glslFract(cy);
  sF32 min1 = 2.0f, min2 = 2.0f;
  for (sF32 x = -1.0f; x <= 1.0f; x++)
    for (sF32 y = -1.0f; y <= 1.0f; y++) {
      Vec2 n = mmRand2(glslMod(ox + x, sizeX), glslMod(oy + y, sizeY));
      sF32 nx = n.x * nodeScale + x;
      sF32 ny = n.y * nodeScale + y;
      sF32 dx = fx - nx, dy = fy - ny;
      sF32 dist;
      if (metric == 0)
        dist = sqrtf(dx * dx + dy * dy);
      else if (metric == 1)
        dist = fabsf(dx) + fabsf(dy);
      else
        dist = fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy);
      if (min1 > dist) {
        min2 = min1;
        min1 = dist;
      } else if (min2 > dist) {
        min2 = dist;
      }
    }
  return f2f1 ? (min2 - min1) : min1;
}

sF32 fbmNoise(sInt mode, sF32 cx, sF32 cy, sF32 sizeX, sF32 sizeY,
              sF32 seed) {
  switch (mode) {
  case MMFbmValue:
    return fbmValue(cx, cy, sizeX, sizeY, seed);
  case MMFbmPerlin:
    return fbmPerlin(cx, cy, sizeX, sizeY, seed);
  case MMFbmPerlinAbs:
    return fabsf(2.0f * fbmPerlin(cx, cy, sizeX, sizeY, seed) - 1.0f);
  case MMFbmCellular:
    return fbmCellular(cx, cy, sizeX, sizeY, seed, 0, false, 1.0f);
  case MMFbmCellular2:
    return fbmCellular(cx, cy, sizeX, sizeY, seed, 0, true, 1.0f);
  case MMFbmCellular3:
    return fbmCellular(cx, cy, sizeX, sizeY, seed, 1, false, 0.5f);
  case MMFbmCellular4:
    return fbmCellular(cx, cy, sizeX, sizeY, seed, 1, true, 0.5f);
  case MMFbmCellular5:
    return fbmCellular(cx, cy, sizeX, sizeY, seed, 2, false, 1.0f);
  case MMFbmCellular6:
    return fbmCellular(cx, cy, sizeX, sizeY, seed, 2, true, 1.0f);
  default:
    return 0.0f;
  }
}

} // namespace

void MMFbm(GenTexture &out, sInt mode, sInt scaleX, sInt scaleY, sInt folds,
           sInt octaves, sF32 persistence, sF32 seed) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      sF32 sizeX = (sF32)scaleX, sizeY = (sF32)scaleY;
      sF32 norm = 0.0f, value = 0.0f, scale = 1.0f;
      for (sInt i = 0; i < octaves; i++) {
        sF32 noise = fbmNoise(mode, u * sizeX, v * sizeY, sizeX, sizeY, seed);
        for (sInt f = 0; f < folds; f++)
          noise = fabsf(2.0f * noise - 1.0f);
        value += noise * scale;
        norm += scale;
        sizeX *= 2.0f;
        sizeY *= 2.0f;
        scale *= persistence;
      }
      gray16(out.Data[py * w + px], value / norm);
    }
  }
}

// ============================================================
// Blend (blend.mmg)
// ============================================================

namespace {

inline sF32 blendOverlayF(sF32 c1, sF32 c2) {
  return (c1 < 0.5f) ? (2.0f * c1 * c2)
                     : (1.0f - 2.0f * (1.0f - c1) * (1.0f - c2));
}
inline sF32 blendSoftLightF(sF32 c1, sF32 c2) {
  return (c2 < 0.5f) ? (2.0f * c1 * c2 + c1 * c1 * (1.0f - 2.0f * c2))
                     : 2.0f * c1 * (1.0f - c2) + sqrtf(c1) * (2.0f * c2 - 1.0f);
}
inline sF32 blendBurnF(sF32 c1, sF32 c2) {
  return (c1 == 0.0f) ? c1
                      : (1.0f - (1.0f - c2) / c1 > 0.0f
                             ? 1.0f - (1.0f - c2) / c1
                             : 0.0f);
}
inline sF32 blendDodgeF(sF32 c1, sF32 c2) {
  return (c1 == 1.0f) ? c1 : (c2 / (1.0f - c1) < 1.0f ? c2 / (1.0f - c1) : 1.0f);
}

// c1 = top layer, c2 = bottom layer, op = blend amount
void blendRGB(sInt mode, sF32 u, sF32 v, const sF32 c1[3], const sF32 c2[3],
              sF32 op, sF32 out[3]) {
  switch (mode) {
  case MMBlendNormal:
    for (int i = 0; i < 3; i++)
      out[i] = op * c1[i] + (1.0f - op) * c2[i];
    break;
  case MMBlendDissolve: {
    bool top = mmRand(u, v) < op;
    for (int i = 0; i < 3; i++)
      out[i] = top ? c1[i] : c2[i];
    break;
  }
  case MMBlendMultiply:
    for (int i = 0; i < 3; i++)
      out[i] = op * c1[i] * c2[i] + (1.0f - op) * c2[i];
    break;
  case MMBlendScreen:
    for (int i = 0; i < 3; i++)
      out[i] = op * (1.0f - (1.0f - c1[i]) * (1.0f - c2[i])) +
               (1.0f - op) * c2[i];
    break;
  case MMBlendOverlay:
    for (int i = 0; i < 3; i++)
      out[i] = op * blendOverlayF(c1[i], c2[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendHardLight:
    for (int i = 0; i < 3; i++)
      out[i] = op * 0.5f * (c1[i] * c2[i] + blendOverlayF(c1[i], c2[i])) +
               (1.0f - op) * c2[i];
    break;
  case MMBlendSoftLight:
    for (int i = 0; i < 3; i++)
      out[i] = op * blendSoftLightF(c1[i], c2[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendBurn:
    for (int i = 0; i < 3; i++)
      out[i] = op * blendBurnF(c1[i], c2[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendDodge:
    for (int i = 0; i < 3; i++)
      out[i] = op * blendDodgeF(c1[i], c2[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendLighten:
    for (int i = 0; i < 3; i++)
      out[i] = op * (c1[i] > c2[i] ? c1[i] : c2[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendDarken:
    for (int i = 0; i < 3; i++)
      out[i] = op * (c1[i] < c2[i] ? c1[i] : c2[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendDifference:
    for (int i = 0; i < 3; i++)
      out[i] = op * clamp01(c2[i] - c1[i]) + (1.0f - op) * c2[i];
    break;
  case MMBlendAdditive:
    for (int i = 0; i < 3; i++)
      out[i] = c2[i] + c1[i] * op;
    break;
  case MMBlendAddSub:
    for (int i = 0; i < 3; i++)
      out[i] = c2[i] + (c1[i] - 0.5f) * 2.0f * op;
    break;
  case MMBlendLinearLight:
    // linear light = burn/dodge by contrast: c2 + 2*c1 - 1
    for (int i = 0; i < 3; i++)
      out[i] = op * clamp01(c2[i] + 2.0f * c1[i] - 1.0f) + (1.0f - op) * c2[i];
    break;
  default:
    for (int i = 0; i < 3; i++)
      out[i] = c2[i];
    break;
  }
}

} // namespace

void MMBlend(GenTexture &out, const GenTexture &a, const GenTexture &b,
             const GenTexture *mask, sInt mode, sF32 opacity) {
  if (!out.Data || !a.Data || !b.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 s = 1.0f / 65535.0f;
  const bool sameA = (a.XRes == w && a.YRes == h);
  const bool sameB = (b.XRes == w && b.YRes == h);
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      sF32 c1[4], c2[4];
      if (sameA) {
        const Pixel &p = a.Data[py * w + px];
        c1[0] = p.r * s;
        c1[1] = p.g * s;
        c1[2] = p.b * s;
        c1[3] = p.a * s;
      } else {
        sampleBilinearWrap(a, u, v, c1);
      }
      if (sameB) {
        const Pixel &p = b.Data[py * w + px];
        c2[0] = p.r * s;
        c2[1] = p.g * s;
        c2[2] = p.b * s;
        c2[3] = p.a * s;
      } else {
        sampleBilinearWrap(b, u, v, c2);
      }
      sF32 amount = opacity;
      if (mask && mask->Data)
        amount *= sampleGrayBilinearWrap(*mask, u, v);
      sF32 op = amount * c1[3];
      sF32 rgb[3];
      blendRGB(mode, u, v, c1, c2, op, rgb);
      Pixel &o = out.Data[py * w + px];
      o.r = to16(rgb[0]);
      o.g = to16(rgb[1]);
      o.b = to16(rgb[2]);
      o.a = to16(c2[3] + op < 1.0f ? c2[3] + op : 1.0f);
    }
  }
}

// ============================================================
// Warp (warp.mmg)
// ============================================================

void MMWarp(GenTexture &out, const GenTexture &in, const GenTexture &height,
            const GenTexture *strength, sF32 amount, sF32 epsilon,
            sInt mode) {
  if (!out.Data || !in.Data || !height.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 eps = epsilon > 0.0001f ? epsilon : 0.0001f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      // slope = finite differences of the height map
      sF32 sx = sampleGrayBilinearWrap(height, u + eps, v) -
                sampleGrayBilinearWrap(height, u - eps, v);
      sF32 sy = sampleGrayBilinearWrap(height, u, v + eps) -
                sampleGrayBilinearWrap(height, u, v - eps);
      sF32 str = 1.0f;
      if (strength && strength->Data)
        str = sampleGrayBilinearWrap(*strength, u, v);
      if (mode == 1) // distance to top
        str *= 1.0f - sampleGrayBilinearWrap(height, u, v);
      sF32 c[4];
      sampleBilinearWrap(in, u + amount * str * sx, v + amount * str * sy, c);
      Pixel &o = out.Data[py * w + px];
      o.r = to16(c[0]);
      o.g = to16(c[1]);
      o.b = to16(c[2]);
      o.a = to16(c[3]);
    }
  }
}

// ============================================================
// Colorize (colorize.mmg — multi-stop gradient map)
// ============================================================

void MMColorize(GenTexture &out, const GenTexture &in,
                const MMGradientStop *stops, sInt nStops) {
  if (!out.Data || !in.Data || nStops < 1)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const bool same = (in.XRes == w && in.YRes == h);
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 t;
      if (same) {
        t = lumOf(in.Data[py * w + px]);
      } else {
        t = sampleGrayBilinearWrap(in, (px + 0.5f) / w, (py + 0.5f) / h);
      }
      sF32 r, g, b, a;
      if (t <= stops[0].pos || nStops == 1) {
        r = stops[0].r;
        g = stops[0].g;
        b = stops[0].b;
        a = stops[0].a;
      } else if (t >= stops[nStops - 1].pos) {
        r = stops[nStops - 1].r;
        g = stops[nStops - 1].g;
        b = stops[nStops - 1].b;
        a = stops[nStops - 1].a;
      } else {
        sInt i = 0;
        while (i < nStops - 2 && t > stops[i + 1].pos)
          i++;
        sF32 span = stops[i + 1].pos - stops[i].pos;
        sF32 f = span > 0.0f ? (t - stops[i].pos) / span : 0.0f;
        r = mixf(stops[i].r, stops[i + 1].r, f);
        g = mixf(stops[i].g, stops[i + 1].g, f);
        b = mixf(stops[i].b, stops[i + 1].b, f);
        a = mixf(stops[i].a, stops[i + 1].a, f);
      }
      Pixel &o = out.Data[py * w + px];
      o.r = to16(r);
      o.g = to16(g);
      o.b = to16(b);
      o.a = to16(a);
    }
  }
}

// ============================================================
// Bricks v2 (bricks.mmg tessellations)
// ============================================================

namespace {

struct Rect4 {
  sF32 minx, miny, maxx, maxy;
};

// oldbricks_rb — running bond
Rect4 bricksRB(sF32 u, sF32 v, sF32 cx, sF32 cy, sF32 repeat, sF32 offset) {
  cx *= repeat;
  cy *= repeat;
  sF32 xoff = offset * (glslFract(v * cy * 0.5f) >= 0.5f ? 1.0f : 0.0f);
  sF32 bminx = floorf(u * cx - xoff);
  sF32 bminy = floorf(v * cy);
  bminx += xoff;
  bminx /= cx;
  bminy /= cy;
  return {bminx, bminy, bminx + 1.0f / cx, bminy + 1.0f / cy};
}

// oldbricks_rb2 — running bond with alternating row density
Rect4 bricksRB2(sF32 u, sF32 v, sF32 cx, sF32 cy, sF32 repeat, sF32 offset) {
  cx *= repeat;
  cy *= repeat;
  sF32 alt = glslFract(v * cy * 0.5f) >= 0.5f ? 1.0f : 0.0f;
  sF32 xoff = offset * alt;
  sF32 ccx = cx * (1.0f + alt);
  sF32 bminx = floorf(u * ccx - xoff);
  sF32 bminy = floorf(v * cy);
  bminx += xoff;
  bminx /= ccx;
  bminy /= cy;
  return {bminx, bminy, bminx + 1.0f / ccx, bminy + 1.0f / cy};
}

// oldbricks_hb — herringbone
Rect4 bricksHB(sF32 u, sF32 v, sF32 cx, sF32 cy, sF32 repeat, sF32) {
  sF32 pc = cx + cy;
  sF32 c = pc * repeat;
  sF32 cornerX = floorf(u * c), cornerY = floorf(v * c);
  sF32 cdiff = glslMod(cornerX - cornerY, pc);
  if (cdiff < cx) {
    sF32 bx = (cornerX - cdiff) / c, by = cornerY / c;
    return {bx, by, bx + cx / c, by + 1.0f / c};
  }
  sF32 bx = cornerX / c, by = (cornerY - (pc - cdiff - 1.0f)) / c;
  return {bx, by, bx + 1.0f / c, by + cy / c};
}

// oldbricks_bw — basket weave
Rect4 bricksBW(sF32 u, sF32 v, sF32 cx, sF32 cy, sF32 repeat, sF32) {
  sF32 c2x = 2.0f * cx * repeat, c2y = 2.0f * cy * repeat;
  sF32 corner1x = floorf(u * c2x), corner1y = floorf(v * c2y);
  sF32 corner2x = cx * floorf(repeat * 2.0f * u);
  sF32 corner2y = cy * floorf(repeat * 2.0f * v);
  sF32 cdiff = glslMod(floorf(repeat * 2.0f * u) + floorf(repeat * 2.0f * v),
                       2.0f);
  sF32 cornerX, cornerY, sizeX, sizeY;
  if (cdiff == 0.0f) {
    cornerX = corner1x;
    cornerY = corner2y;
    sizeX = 1.0f;
    sizeY = cy;
  } else {
    cornerX = corner2x;
    cornerY = corner1y;
    sizeX = cx;
    sizeY = 1.0f;
  }
  return {cornerX / c2x, cornerY / c2y, (cornerX + sizeX) / c2x,
          (cornerY + sizeY) / c2y};
}

// oldbricks_sb — spanish bond
Rect4 bricksSB(sF32 u, sF32 v, sF32 cx, sF32 cy, sF32 repeat, sF32) {
  sF32 ccx = (cx + 1.0f) * repeat, ccy = (cy + 1.0f) * repeat;
  sF32 corner1x = floorf(u * ccx), corner1y = floorf(v * ccy);
  sF32 corner2x = (cx + 1.0f) * floorf(repeat * u);
  sF32 corner2y = (cy + 1.0f) * floorf(repeat * v);
  sF32 rx = corner1x - corner2x, ry = corner1y - corner2y;
  sF32 cornerX, cornerY, sizeX, sizeY;
  if (rx == 0.0f && ry < cy) {
    cornerX = corner2x;
    cornerY = corner2y;
    sizeX = 1.0f;
    sizeY = cy;
  } else if (ry == 0.0f) {
    cornerX = corner2x + 1.0f;
    cornerY = corner2y;
    sizeX = cx;
    sizeY = 1.0f;
  } else if (rx == cx) {
    cornerX = corner2x + cx;
    cornerY = corner2y + 1.0f;
    sizeX = 1.0f;
    sizeY = cy;
  } else if (ry == cy) {
    cornerX = corner2x;
    cornerY = corner2y + cy;
    sizeX = cx;
    sizeY = 1.0f;
  } else {
    cornerX = corner2x + 1.0f;
    cornerY = corner2y + 1.0f;
    sizeX = cx - 1.0f;
    sizeY = cy - 1.0f;
  }
  return {cornerX / ccx, cornerY / ccy, (cornerX + sizeX) / ccx,
          (cornerY + sizeY) / ccy};
}

// oldbrick — rounded-rect SDF inside the brick cell, clamped by bevel
sF32 brickMask(sF32 u, sF32 v, const Rect4 &b, sF32 mortar, sF32 roundR,
               sF32 bevel) {
  sF32 sizeX = b.maxx - b.minx, sizeY = b.maxy - b.miny;
  sF32 minSize = sizeX < sizeY ? sizeX : sizeY;
  mortar *= minSize;
  bevel *= minSize;
  roundR *= minSize;
  sF32 cx = 0.5f * (b.minx + b.maxx), cy = 0.5f * (b.miny + b.maxy);
  sF32 dx = fabsf(u - cx) - 0.5f * sizeX + roundR + mortar;
  sF32 dy = fabsf(v - cy) - 0.5f * sizeY + roundR + mortar;
  sF32 mx = dx > 0.0f ? dx : 0.0f, my = dy > 0.0f ? dy : 0.0f;
  sF32 inner = dx > dy ? dx : dy;
  sF32 d = sqrtf(mx * mx + my * my) + (inner < 0.0f ? inner : 0.0f) - roundR;
  return clamp01(-d / (bevel > 1e-6f ? bevel : 1e-6f));
}

} // namespace

void MMBricks(GenTexture &out, sInt pattern, sInt countX, sInt countY,
              sInt repeat, sF32 offset, sF32 mortar, sF32 roundRadius,
              sF32 bevel, sU32 col0, sU32 col1, sU32 colMortar,
              sF32 colorBalance, sF32 seed) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  sF32 c0[4], c1[4], cm[4];
  u32ToF4(col0, c0);
  u32ToF4(col1, c1);
  u32ToF4(colMortar, cm);
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      Rect4 b;
      switch (pattern) {
      case MMBricksRunningBond2:
        b = bricksRB2(u, v, (sF32)countX, (sF32)countY, (sF32)repeat, offset);
        break;
      case MMBricksHerringbone:
        b = bricksHB(u, v, (sF32)countX, (sF32)countY, (sF32)repeat, offset);
        break;
      case MMBricksBasketWeave:
        b = bricksBW(u, v, (sF32)countX, (sF32)countY, (sF32)repeat, offset);
        break;
      case MMBricksSpanishBond:
        b = bricksSB(u, v, (sF32)countX, (sF32)countY, (sF32)repeat, offset);
        break;
      case MMBricksRunningBond:
      default:
        b = bricksRB(u, v, (sF32)countX, (sF32)countY, (sF32)repeat, offset);
        break;
      }
      sF32 mask = brickMask(u, v, b, mortar, roundRadius, bevel);
      // per-brick random color between col0 and col1
      sF32 ccx = 0.5f * (b.minx + b.maxx), ccy = 0.5f * (b.miny + b.maxy);
      sF32 rnd = mmRand(glslFract(ccx + seed), glslFract(ccy + seed));
      sF32 t = clamp01(rnd * 2.0f * colorBalance);
      Pixel &o = out.Data[py * w + px];
      sF32 rgba[4];
      for (int i = 0; i < 4; i++) {
        sF32 brickC = mixf(c0[i], c1[i], t);
        rgba[i] = mixf(cm[i], brickC, mask);
      }
      o.r = to16(rgba[0]);
      o.g = to16(rgba[1]);
      o.b = to16(rgba[2]);
      o.a = to16(rgba[3]);
    }
  }
}

// ============================================================
// Shape (shape.mmg)
// ============================================================

namespace {

sF32 shapeValue(sInt shape, sF32 u, sF32 v, sF32 sides, sF32 size,
                sF32 edge) {
  sF32 x = 2.0f * u - 1.0f;
  sF32 y = 2.0f * v - 1.0f;
  const sF32 tau = 6.28318530718f;
  switch (shape) {
  case MMShapeCircle: {
    sF32 e = edge > 1e-8f ? edge : 1e-8f;
    return clamp01((1.0f - sqrtf(x * x + y * y) / size) / e);
  }
  case MMShapePolygon: {
    sF32 e = edge > 1e-8f ? edge : 1e-8f;
    sF32 angle = atan2f(x, y) + 3.14159265359f;
    sF32 slice = tau / sides;
    return clamp01((1.0f - (cosf(floorf(0.5f + angle / slice) * slice -
                                 angle) *
                            sqrtf(x * x + y * y)) /
                               size) /
                   e);
  }
  case MMShapeStar: {
    sF32 e = edge > 1e-8f ? edge : 1e-8f;
    sF32 angle = atan2f(x, y);
    sF32 slice = tau / sides;
    sF32 t = angle * sides / tau;
    sF32 stepv = glslFract(t) <= 0.5f ? 1.0f : 0.0f;
    return clamp01((1.0f - (cosf(floorf(t - 0.5f + 2.0f * stepv) * slice -
                                 angle) *
                            sqrtf(x * x + y * y)) /
                               size) /
                   e);
  }
  case MMShapeCurvedStar: {
    sF32 e = edge > 1e-8f ? edge : 1e-8f;
    sF32 angle = 2.0f * (atan2f(x, y) + 3.14159265359f);
    sF32 slice = tau / sides;
    return clamp01((1.0f - cosf(floorf(0.5f + 0.5f * angle / slice) * 2.0f *
                                    slice -
                                angle) *
                               sqrtf(x * x + y * y) / size) /
                   e);
  }
  case MMShapeRays: {
    sF32 e = 0.5f * (edge > 1e-8f ? edge : 1e-8f) * size;
    sF32 slice = tau / sides;
    sF32 angle =
        glslMod(atan2f(x, y) + 3.14159265359f, slice) / slice;
    sF32 a = (size - angle) / e;
    sF32 b = angle / e;
    return clamp01(a < b ? a : b);
  }
  default:
    return 0.0f;
  }
}

} // namespace

void MMShape(GenTexture &out, sInt shape, sF32 sides, sF32 radius,
             sF32 edge, const GenTexture *radiusMap,
             const GenTexture *edgeMap) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      sF32 r = radius, e = edge;
      if (radiusMap && radiusMap->Data)
        r *= sampleGrayBilinearWrap(*radiusMap, u, v);
      if (edgeMap && edgeMap->Data)
        e *= sampleGrayBilinearWrap(*edgeMap, u, v);
      gray16(out.Data[py * w + px],
             shapeValue(shape, u, v, sides + 1e-5f, r, e));
    }
}

// ============================================================
// Pattern (pattern.mmg)
// ============================================================

namespace {

sF32 waveValue(sInt wave, sF32 x) {
  switch (wave) {
  case MMWaveSine:
    return 0.5f - 0.5f * cosf(3.14159265359f * 2.0f * x);
  case MMWaveTriangle: {
    sF32 f = glslFract(x);
    return 2.0f * f < 2.0f - 2.0f * f ? 2.0f * f : 2.0f - 2.0f * f;
  }
  case MMWaveSquare:
    return glslFract(x) < 0.5f ? 0.0f : 1.0f;
  case MMWaveSawtooth:
    return glslFract(x);
  case MMWaveConstant:
    return 1.0f;
  case MMWaveBounce: {
    sF32 f = 2.0f * (glslFract(x) - 0.5f);
    return sqrtf(1.0f - f * f);
  }
  default:
    return 0.0f;
  }
}

sF32 waveMix(sInt mode, sF32 x, sF32 y) {
  switch (mode) {
  case MMWaveMixMultiply:
    return x * y;
  case MMWaveMixAdd:
    return x + y < 1.0f ? x + y : 1.0f;
  case MMWaveMixMax:
    return x > y ? x : y;
  case MMWaveMixMin:
    return x < y ? x : y;
  case MMWaveMixXor: {
    sF32 a = x + y, b = 2.0f - x - y;
    return a < b ? a : b;
  }
  case MMWaveMixPow:
    return powf(x, y);
  default:
    return x;
  }
}

} // namespace

void MMPattern(GenTexture &out, sInt mixMode, sInt xWave, sF32 xScale,
               sInt yWave, sF32 yScale) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      gray16(out.Data[py * w + px],
             waveMix(mixMode, waveValue(xWave, xScale * u),
                     waveValue(yWave, yScale * v)));
    }
}

// Gradient generator (gradient.mmg).
void MMGradientRamp(GenTexture &out, const MMGradientStop *stops,
                    sInt nStops, sF32 repeat, sF32 rotateDeg, bool mirror) {
  if (!out.Data || nStops < 1)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 rot = rotateDeg * 0.01745329251f;
  const sF32 cr = cosf(rot), sr = sinf(rot);
  // MM normalizes so the ramp spans the rotated unit square exactly
  const sF32 norm =
      cosf(fabsf(glslMod(rotateDeg, 90.0f) - 45.0f) * 0.01745329251f) *
      1.41421356237f;

  auto evalStops = [&](sF32 t, sF32 c[4]) {
    if (t <= stops[0].pos || nStops == 1) {
      c[0] = stops[0].r; c[1] = stops[0].g;
      c[2] = stops[0].b; c[3] = stops[0].a;
      return;
    }
    if (t >= stops[nStops - 1].pos) {
      c[0] = stops[nStops - 1].r; c[1] = stops[nStops - 1].g;
      c[2] = stops[nStops - 1].b; c[3] = stops[nStops - 1].a;
      return;
    }
    sInt i = 0;
    while (i < nStops - 2 && t > stops[i + 1].pos)
      i++;
    const sF32 span = stops[i + 1].pos - stops[i].pos;
    const sF32 f = span > 0.0f ? (t - stops[i].pos) / span : 0.0f;
    c[0] = stops[i].r + (stops[i + 1].r - stops[i].r) * f;
    c[1] = stops[i].g + (stops[i + 1].g - stops[i].g) * f;
    c[2] = stops[i].b + (stops[i + 1].b - stops[i].b) * f;
    c[3] = stops[i].a + (stops[i + 1].a - stops[i].a) * f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const sF32 r = 0.5f + (cr * (u - 0.5f) + sr * (v - 0.5f)) / norm;
      sF32 t = glslFract(r * repeat);
      if (mirror)
        t = 2.0f * (0.5f - fabsf(t - 0.5f));
      sF32 c[4];
      evalStops(t, c);
      Pixel &p = out.Data[py * w + px];
      p.r = to16(c[0]);
      p.g = to16(c[1]);
      p.b = to16(c[2]);
      p.a = to16(c[3]);
    }
  }
}

// Dot noise (noise.mmg).
void MMDotNoise(GenTexture &out, sInt gridSize, sF32 density,
                const GenTexture *densityIn, sF32 seed, sInt mode) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (gridSize < 1)
    gridSize = 1;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const sF32 cu = floorf(u * gridSize) / gridSize;
      const sF32 cv = floorf(v * gridSize) / gridSize;
      sF32 d = density;
      if (densityIn && densityIn->Data)
        d = sampleGrayBilinearWrap(*densityIn, cu, cv);
      const sF32 r = mmRand(cu + seed, cv + seed);
      gray16(out.Data[py * w + px], mode == 1 ? r : (r < d ? 1.0f : 0.0f));
    }
  }
}

// One scratch layer (old_scratch in scratches.mmg).
static sF32 mmScratchLayer(sF32 u, sF32 v, sF32 sizeX, sF32 sizeY,
                           sF32 waviness, sF32 angle, sF32 randomness,
                           sF32 seedX, sF32 seedY) {
  const sF32 subdivide = floorf(1.0f / sizeX);
  const sF32 cut = sizeX * subdivide;
  u *= subdivide;
  v *= subdivide;
  Vec2 r1 = mmRand2(floorf(u) + seedX, floorf(v) + seedY);
  Vec2 r2 = mmRand2(r1.x, r1.y);
  u = glslFract(u);
  v = glslFract(v);
  const sF32 bx = 10.0f * (u < 1.0f - u ? u : 1.0f - u);
  const sF32 by = 10.0f * (v < 1.0f - v ? v : 1.0f - v);
  u = 2.0f * u - 1.0f;
  v = 2.0f * v - 1.0f;
  const sF32 a = 6.28318530718f * (angle + (r1.x - 0.5f) * randomness);
  const sF32 c = cosf(a), s = sinf(a);
  sF32 ru = c * u + s * v;
  sF32 rv = s * u - c * v;
  rv += 2.0f * r1.y - 1.0f;
  rv += 0.5f * waviness * cosf(2.0f * ru + 6.28318530718f * r2.y);
  ru /= cut;
  rv /= subdivide * sizeY;
  const sF32 border = bx < by ? bx : by;
  sF32 fall = 1.0f - 1000.0f * rv * rv;
  if (fall < 0.0f)
    fall = 0.0f;
  return border * (1.0f - ru * ru) * fall;
}

// Scratches generator (scratches.mmg).
void MMScratches(GenTexture &out, sInt layers, sF32 length, sF32 width,
                 sF32 waviness, sF32 angleDeg, sF32 randomness, sF32 seed) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (length < 1e-3f)
    length = 1e-3f;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 sx = seed, sy = 0.0f;
      sF32 val = 0.0f;
      for (sInt i = 0; i < layers; i++) {
        Vec2 s2 = mmRand2(sx, sy);
        sx = s2.x;
        sy = s2.y;
        const sF32 lv = mmScratchLayer(glslFract(u + sx), glslFract(v + sy),
                                       length, width, waviness,
                                       angleDeg / 360.0f, randomness, sx, sy);
        if (lv > val)
          val = lv;
      }
      gray16(out.Data[py * w + px], val > 1.0f ? 1.0f : val);
    }
  }
}

// Sphere heightmap (sphere.mmg).
void MMSphere(GenTexture &out, sF32 cx, sF32 cy, sF32 r, bool normalized) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (r < 1e-5f)
    r = 1e-5f;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = ((py + 0.5f) / h - cy) / r;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = ((px + 0.5f) / w - cx) / r;
      const sF32 q = 1.0f - (u * u + v * v);
      sF32 val = q > 0.0f ? 2.0f * r * sqrtf(q) : 0.0f;
      if (normalized)
        val /= 2.0f * r;
      gray16(out.Data[py * w + px], val);
    }
  }
}

// Anisotropic noise (noise_anisotropic.mmg).
void MMAnisotropicNoise(GenTexture &out, sF32 scaleX, sF32 scaleY, sF32 seed,
                        sF32 smoothness, sF32 interpolation) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (smoothness < 1e-4f)
    smoothness = 1e-4f;
  Vec2 s2 = mmRand2(seed, 1.0f - seed);
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const sF32 xyY = floorf(v * scaleY);
      const sF32 off = mmRand(s2.x + xyY, s2.y + xyY);
      const sF32 xo = floorf(u * scaleX + off);
      const sF32 yo = floorf(v * scaleY);
      const sF32 f0 = mmRand(s2.x + glslMod(xo, scaleX), s2.y + glslMod(yo, scaleY));
      const sF32 f1 =
          mmRand(s2.x + glslMod(xo + 1.0f, scaleX), s2.y + glslMod(yo, scaleY));
      sF32 mixer =
          clamp01((glslFract(u * scaleX + off) - 0.5f) / smoothness + 0.5f);
      const sF32 sm = mixer * mixer * (3.0f - 2.0f * mixer);
      const sF32 lin = f0 + (f1 - f0) * mixer;
      const sF32 smo = f0 + (f1 - f0) * sm;
      gray16(out.Data[py * w + px], lin + (smo - lin) * interpolation);
    }
  }
}
