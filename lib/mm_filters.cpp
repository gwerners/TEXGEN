// mm_filters — image filter ports. See mm_filters.h for attribution.
#include "mm_filters.h"

#include <cmath>
#include <vector>

namespace {

inline sF32 clamp01(sF32 v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline sF32 mixf(sF32 a, sF32 b, sF32 t) { return a + (b - a) * t; }
inline sU16 to16(sF32 v) { return (sU16)(clamp01(v) * 65535.0f + 0.5f); }

inline sInt wrapi(sInt a, sInt n) {
  a %= n;
  return a < 0 ? a + n : a;
}

// Grayscale of a pixel (RGB average) in [0,1], integer coords with wrap.
inline sF32 grayAt(const GenTexture &t, sInt x, sInt y) {
  const Pixel &p = t.Data[wrapi(y, t.YRes) * t.XRes + wrapi(x, t.XRes)];
  return ((sF32)p.r + (sF32)p.g + (sF32)p.b) / (3.0f * 65535.0f);
}

// Bilinear wrap RGBA sample in [0,1].
inline void sampleRGBA(const GenTexture &t, sF32 u, sF32 v, sF32 out[4]) {
  u = u - floorf(u);
  v = v - floorf(v);
  sF32 fx = u * t.XRes - 0.5f;
  sF32 fy = v * t.YRes - 0.5f;
  sInt x0 = (sInt)floorf(fx);
  sInt y0 = (sInt)floorf(fy);
  sF32 tx = fx - x0;
  sF32 ty = fy - y0;
  sInt x1 = wrapi(x0 + 1, t.XRes);
  sInt y1 = wrapi(y0 + 1, t.YRes);
  x0 = wrapi(x0, t.XRes);
  y0 = wrapi(y0, t.YRes);
  const sF32 s = 1.0f / 65535.0f;
  const Pixel &p00 = t.Data[y0 * t.XRes + x0];
  const Pixel &p10 = t.Data[y0 * t.XRes + x1];
  const Pixel &p01 = t.Data[y1 * t.XRes + x0];
  const Pixel &p11 = t.Data[y1 * t.XRes + x1];
  sF32 c00[4] = {p00.r * s, p00.g * s, p00.b * s, p00.a * s};
  sF32 c10[4] = {p10.r * s, p10.g * s, p10.b * s, p10.a * s};
  sF32 c01[4] = {p01.r * s, p01.g * s, p01.b * s, p01.a * s};
  sF32 c11[4] = {p11.r * s, p11.g * s, p11.b * s, p11.a * s};
  for (int i = 0; i < 4; i++)
    out[i] = mixf(mixf(c00[i], c10[i], tx), mixf(c01[i], c11[i], tx), ty);
}

} // namespace

// Sobel edge detect (normal_map.mmg inner shader) + process_normal_<format>.
void MMNormalMap(GenTexture &out, const GenTexture &height, sF32 amount,
                 sInt format) {
  if (!out.Data || !height.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  // MM: multiplier = amount * size / 128.0, e = 1/size (one texel)
  const sF32 mult = amount * (sF32)height.XRes / 128.0f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sInt hx = px * height.XRes / w;
      sInt hy = py * height.YRes / h;
      // Sobel: e.xy = (+1,-1), e.xx = (+1,+1), e.xz = (+1,0), e.zx = (0,+1)
      sF32 rvx = 0.0f, rvy = 0.0f;
      sF32 s;
      s = grayAt(height, hx + 1, hy - 1); rvx += s; rvy -= s;
      s = grayAt(height, hx - 1, hy + 1); rvx -= s; rvy += s;
      s = grayAt(height, hx + 1, hy + 1); rvx += s; rvy += s;
      s = grayAt(height, hx - 1, hy - 1); rvx -= s; rvy -= s;
      s = grayAt(height, hx + 1, hy);     rvx += 2.0f * s;
      s = grayAt(height, hx - 1, hy);     rvx -= 2.0f * s;
      s = grayAt(height, hx, hy + 1);     rvy += 2.0f * s;
      s = grayAt(height, hx, hy - 1);     rvy -= 2.0f * s;

      sF32 vx = rvx * mult, vy = rvy * mult, vz;
      switch (format) {
      case MMNormalOpenGL:
        vz = 1.0f;
        break;
      case MMNormalDirectX:
        vy = -vy;
        vz = 1.0f;
        break;
      case MMNormalDefault:
      default:
        vz = -1.0f;
        break;
      }
      sF32 len = sqrtf(vx * vx + vy * vy + vz * vz);
      if (len < 1e-6f)
        len = 1.0f;
      Pixel &o = out.Data[py * w + px];
      o.r = to16(0.5f * vx / len + 0.5f);
      o.g = to16(0.5f * vy / len + 0.5f);
      o.b = to16(0.5f * vz / len + 0.5f);
      o.a = 65535;
    }
  }
}

// make_tileable.mmg: blend with offset copies using radial masks.
void MMMakeTileable(GenTexture &out, const GenTexture &in, sF32 w) {
  if (!out.Data || !in.Data)
    return;
  const sInt ow = out.XRes, oh = out.YRes;
  const sF32 hw = 0.5f * (w > 1e-4f ? w : 1e-4f); // MM passes 0.5*$w
  for (sInt py = 0; py < oh; py++) {
    for (sInt px = 0; px < ow; px++) {
      sF32 u = (px + 0.5f) / ow;
      sF32 v = (py + 0.5f) / oh;
      sF32 a[4], b[4], c[4];
      sampleRGBA(in, u, v, a);
      sampleRGBA(in, u + 0.5f, v + 0.5f, b);
      sampleRGBA(in, u + 0.25f, v + 0.25f, c);
      sF32 du = u - 0.5f, dv = v - 0.5f;
      sF32 coefAB = sinf(1.57079632679f *
                         clamp01((sqrtf(du * du + dv * dv) - 0.5f + hw) / hw));
      auto dist = [&](sF32 cx, sF32 cy) {
        sF32 dx = u - cx, dy = v - cy;
        return sqrtf(dx * dx + dy * dy);
      };
      sF32 dmin1 = dist(0.0f, 0.5f) < dist(0.5f, 0.0f) ? dist(0.0f, 0.5f)
                                                       : dist(0.5f, 0.0f);
      sF32 dmin2 = dist(1.0f, 0.5f) < dist(0.5f, 1.0f) ? dist(1.0f, 0.5f)
                                                       : dist(0.5f, 1.0f);
      sF32 dmin = dmin1 < dmin2 ? dmin1 : dmin2;
      sF32 coefABC = sinf(1.57079632679f * clamp01((dmin - hw) / hw));
      Pixel &o = out.Data[py * ow + px];
      sF32 res[4];
      for (int i = 0; i < 4; i++)
        res[i] = mixf(c[i], mixf(a[i], b[i], coefAB), coefABC);
      o.r = to16(res[0]);
      o.g = to16(res[1]);
      o.b = to16(res[2]);
      o.a = to16(res[3]);
    }
  }
}

// quantize.mmg: floor(rgb*steps)/steps, alpha preserved.
void MMQuantize(GenTexture &out, const GenTexture &in, sInt steps) {
  if (!out.Data || !in.Data || steps < 1)
    return;
  const sInt n = out.NPixels;
  const sF32 st = (sF32)steps;
  const sF32 s = 1.0f / 65535.0f;
  for (sInt i = 0; i < n; i++) {
    const Pixel &p = in.Data[i];
    Pixel &o = out.Data[i];
    o.r = to16(floorf(p.r * s * st) / st);
    o.g = to16(floorf(p.g * s * st) / st);
    o.b = to16(floorf(p.b * s * st) / st);
    o.a = p.a;
  }
}

// emboss.mmg inner shader: directional gradient accumulation.
void MMEmboss(GenTexture &out, const GenTexture &in, sF32 angleDeg,
              sF32 amount, sInt width) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sInt pixels = width > 1 ? width : 1;
  const sF32 angle = angleDeg * 3.14159265359f / 180.0f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sInt ix = px * in.XRes / w;
      sInt iy = py * in.YRes / h;
      sF32 rv = 0.0f;
      for (sInt dx = -pixels; dx <= pixels; dx++) {
        for (sInt dy = -pixels; dy <= pixels; dy++) {
          if (dx == 0 && dy == 0)
            continue;
          sF32 len = sqrtf((sF32)(dx * dx + dy * dy));
          rv += grayAt(in, ix + dx, iy + dy) *
                cosf(atan2f((sF32)dy, (sF32)dx) - angle) / len;
        }
      }
      sF32 val = amount * rv / (sF32)pixels + 0.5f;
      sU16 g = to16(val);
      Pixel &o = out.Data[py * w + px];
      o.r = o.g = o.b = g;
      o.a = 65535;
    }
  }
}

// transform.mmg: translate, rotate around center, scale, repeat/clamp.
void MMTransform(GenTexture &out, const GenTexture &in, sF32 tx, sF32 ty,
                 sF32 rotDeg, sF32 scaleX, sF32 scaleY, bool repeat,
                 const GenTexture *mapTx, const GenTexture *mapTy,
                 const GenTexture *mapRot, const GenTexture *mapSx,
                 const GenTexture *mapSy) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const bool mapped = mapTx || mapTy || mapRot || mapSx || mapSy;
  // MM modulation: effective = param * (2*map(uv) - 1); null map = 1
  auto mod = [&](const GenTexture *m, sF32 u, sF32 v) -> sF32 {
    if (!m || !m->Data)
      return 1.0f;
    sF32 cc[4];
    sampleRGBA(*m, u, v, cc);
    return 2.0f * (cc[0] + cc[1] + cc[2]) / 3.0f - 1.0f;
  };
  const sF32 rot0 = rotDeg * 0.01745329251f;
  const sF32 c0 = cosf(rot0), s0 = sinf(rot0);
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      const sF32 uu = (px + 0.5f) / w;
      const sF32 vv = (py + 0.5f) / h;
      sF32 c = c0, s = s0;
      sF32 sx = scaleX, sy = scaleY;
      sF32 etx = tx, ety = ty;
      if (mapped) {
        etx = tx * mod(mapTx, uu, vv);
        ety = ty * mod(mapTy, uu, vv);
        const sF32 rot = rot0 * mod(mapRot, uu, vv);
        c = cosf(rot);
        s = sinf(rot);
        sx = scaleX * mod(mapSx, uu, vv);
        sy = scaleY * mod(mapSy, uu, vv);
      }
      if (fabsf(sx) < 1e-6f)
        sx = 1e-6f;
      if (fabsf(sy) < 1e-6f)
        sy = 1e-6f;
      sF32 u = uu - etx - 0.5f;
      sF32 v = vv - ety - 0.5f;
      sF32 ru = (c * u + s * v) / sx + 0.5f;
      sF32 rv = (-s * u + c * v) / sy + 0.5f;
      if (repeat) {
        ru = ru - floorf(ru);
        rv = rv - floorf(rv);
      } else {
        ru = clamp01(ru);
        rv = clamp01(rv);
      }
      sF32 col[4];
      sampleRGBA(in, ru, rv, col);
      Pixel &o = out.Data[py * w + px];
      o.r = to16(col[0]);
      o.g = to16(col[1]);
      o.b = to16(col[2]);
      o.a = to16(col[3]);
    }
  }
}

// combine.mmg: vec4(r, g, b, a) from grayscale inputs.
void MMCombine(GenTexture &out, const GenTexture *r, const GenTexture *g,
               const GenTexture *b, const GenTexture *a) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w;
      sF32 v = (py + 0.5f) / h;
      auto ch = [&](const GenTexture *t, sF32 def) {
        if (!t || !t->Data)
          return def;
        sF32 c[4];
        sampleRGBA(*t, u, v, c);
        return (c[0] + c[1] + c[2]) / 3.0f;
      };
      Pixel &o = out.Data[py * w + px];
      o.r = to16(ch(r, 0.0f));
      o.g = to16(ch(g, 0.0f));
      o.b = to16(ch(b, 0.0f));
      o.a = to16(ch(a, 1.0f));
    }
  }
}

// decompose.mmg: four grayscale outputs, one per channel.
void MMDecompose(GenTexture &outR, GenTexture &outG, GenTexture &outB,
                 GenTexture &outA, const GenTexture &in) {
  if (!in.Data)
    return;
  const sInt n = in.NPixels;
  auto fill = [&](GenTexture &dst, int channel) {
    if (!dst.Data || dst.NPixels != n)
      return;
    for (sInt i = 0; i < n; i++) {
      const Pixel &p = in.Data[i];
      sU16 val = channel == 0 ? p.r : channel == 1 ? p.g
                              : channel == 2       ? p.b
                                                   : p.a;
      Pixel &o = dst.Data[i];
      o.r = o.g = o.b = val;
      o.a = 65535;
    }
  };
  fill(outR, 0);
  fill(outG, 1);
  fill(outB, 2);
  fill(outA, 3);
}

// invert.mmg: 1 - rgb, alpha preserved.
void MMInvert(GenTexture &out, const GenTexture &in) {
  if (!out.Data || !in.Data || out.NPixels != in.NPixels)
    return;
  for (sInt i = 0; i < out.NPixels; i++) {
    const Pixel &p = in.Data[i];
    Pixel &o = out.Data[i];
    o.r = 65535 - p.r;
    o.g = 65535 - p.g;
    o.b = 65535 - p.b;
    o.a = p.a;
  }
}

// Per-pixel scalar math (math.mmg).
void MMMath(GenTexture &out, const GenTexture *in1, const GenTexture *in2,
            sInt op, sF32 def1, sF32 def2, bool clampResult) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 a = def1, b = def2;
      if (in1 && in1->Data) {
        sF32 c[4];
        sampleRGBA(*in1, u, v, c);
        a = (c[0] + c[1] + c[2]) / 3.0f;
      }
      if (in2 && in2->Data) {
        sF32 c[4];
        sampleRGBA(*in2, u, v, c);
        b = (c[0] + c[1] + c[2]) / 3.0f;
      }
      sF32 r;
      switch (op) {
      default:
      case 0: r = a + b; break;
      case 1: r = a - b; break;
      case 2: r = a * b; break;
      case 3: r = b != 0.0f ? a / b : 0.0f; break;
      case 4: r = a > 0.0f ? logf(a) : 0.0f; break;
      case 5: r = a > 0.0f ? log2f(a) : 0.0f; break;
      case 6: r = powf(a, b); break;
      case 7: r = fabsf(a); break;
      case 8: r = roundf(a); break;
      case 9: r = floorf(a); break;
      case 10: r = ceilf(a); break;
      case 11: r = truncf(a); break;
      case 12: r = a - floorf(a); break;
      case 13: r = a < b ? a : b; break;
      case 14: r = a > b ? a : b; break;
      case 15: r = a < b ? 1.0f : 0.0f; break;
      case 16: r = cosf(a * b); break;
      case 17: r = sinf(a * b); break;
      case 18: r = tanf(a * b); break;
      case 19: {
        sF32 s = 1.0f - a * a;
        r = s > 0.0f ? sqrtf(s) : 0.0f;
        break;
      }
      case 20: { // smoothstep(0, 1, a)
        sF32 t = clamp01(a);
        r = t * t * (3.0f - 2.0f * t);
        break;
      }
      case 21: { // ping-pong(a, b)
        if (b != 0.0f) {
          sF32 x = (a - b) / (b * 2.0f);
          r = fabsf((x - floorf(x)) * b * 2.0f - b);
        } else {
          r = 0.0f;
        }
        break;
      }
      case 22: r = a > 0.0f ? 1.0f : (a < 0.0f ? -1.0f : 0.0f); break;
      case 23: r = b != 0.0f ? a - b * floorf(a / b) : 0.0f; break;
      case 24: r = atan2f(a, b); break;
      case 25: r = a >= -1.0f && a <= 1.0f ? asinf(a) : 0.0f; break;
      case 26: r = a >= -1.0f && a <= 1.0f ? acosf(a) : 0.0f; break;
      case 27: r = atanf(a); break;
      case 28: r = sinhf(a); break;
      case 29: r = coshf(a); break;
      case 30: r = tanhf(a); break;
      case 31: r = expf(a); break;
      case 32: r = b != 0.0f ? floorf(a / b) * b : 0.0f; break; // snap
      case 33: r = a * 0.01745329251f; break;                   // radians
      case 34: r = a * 57.2957795131f; break;                   // degrees
      case 35: // log base b
        r = (a > 0.0f && b > 0.0f && b != 1.0f) ? logf(a) / logf(b) : 0.0f;
        break;
      case 36: r = a > 0.0f ? sqrtf(a) : 0.0f; break;
      }
      if (clampResult)
        r = clamp01(r);
      const sU16 g = to16(r);
      Pixel &p = out.Data[py * w + px];
      p.r = p.g = p.b = g;
      p.a = 65535;
    }
  }
}

// Iterative slope-following warp (multi_warp.mmg).
void MMMultiWarp(GenTexture &out, const GenTexture &in,
                 const GenTexture &height, sF32 size, sF32 intensity,
                 sF32 quality, sInt mode) {
  if (!out.Data || !in.Data || !height.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (size < 1.0f)
    size = 1.0f;
  const sF32 dx = 1.0f / size;
  sInt iterations = (sInt)ceilf(intensity * quality);
  if (iterations < 1)
    iterations = 1;
  const sF32 step = intensity * intensity / (sF32)iterations;

  auto heightAt = [&](sF32 u, sF32 v) -> sF32 {
    sF32 c[4];
    sampleRGBA(height, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 v0 = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u0 = (px + 0.5f) / w;
      sF32 ptv[4];
      sampleRGBA(in, u0, v0, ptv);
      sF32 iu = u0, iv = v0;
      sF32 acc[3] = {0.0f, 0.0f, 0.0f};
      for (sInt i = 0; i < iterations; i++) {
        const sF32 hv = heightAt(iu, iv);
        const sF32 sx = (heightAt(iu + dx, iv) - hv) * 2.0f;
        const sF32 sy = (heightAt(iu, iv + dx) - hv) * 2.0f;
        iu += sx * step;
        iv += sy * step;
        sF32 s[4];
        sampleRGBA(in, iu, iv, s);
        for (int k = 0; k < 3; k++) {
          sF32 val = s[k];
          if (mode == 0)
            val = val < ptv[k] ? val : ptv[k];
          else if (mode == 2)
            val = val > ptv[k] ? val : ptv[k];
          acc[k] += val;
        }
      }
      Pixel &p = out.Data[py * w + px];
      p.r = to16(acc[0] / (sF32)iterations);
      p.g = to16(acc[1] / (sF32)iterations);
      p.b = to16(acc[2] / (sF32)iterations);
      p.a = 65535;
    }
  }
}

namespace {

// MM common.glsl rand2/rand3 (same constants as mm_generators.cpp)
inline sF32 glslFract2(sF32 v) { return v - floorf(v); }
inline sF32 glslMod2(sF32 x, sF32 y) { return x - y * floorf(x / y); }

inline void tilerRand2(sF32 x, sF32 y, sF32 o[2]) {
  o[0] = glslFract2(cosf(glslMod2(x * 13.9898f + y * 8.141f, 3.14f)) *
                    43758.5f);
  o[1] = glslFract2(cosf(glslMod2(x * 3.4562f + y * 17.398f, 3.14f)) *
                    43758.5f);
}

inline void tilerRand3(sF32 x, sF32 y, sF32 o[3]) {
  o[0] = glslFract2(cosf(glslMod2(x * 13.9898f + y * 8.141f, 3.14f)) *
                    43758.5f);
  o[1] = glslFract2(cosf(glslMod2(x * 3.4562f + y * 17.398f, 3.14f)) *
                    43758.5f);
  o[2] = glslFract2(cosf(glslMod2(x * 13.254f + y * 5.867f, 3.14f)) *
                    43758.5f);
}

} // namespace

// Instance scatter/tiler (tiler.mmg).
void MMTiler(GenTexture &out, GenTexture *outColor, const GenTexture &in,
             const GenTexture *mask, sF32 tx, sF32 ty, sInt overlap,
             sInt inputs, sF32 scaleX, sF32 scaleY, sF32 fixedOffset,
             sF32 offset, sF32 rotateDeg, sF32 scaleJitter, sF32 value,
             sF32 seed) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (tx < 1.0f)
    tx = 1.0f;
  if (ty < 1.0f)
    ty = 1.0f;
  if (inputs < 1)
    inputs = 1;

  auto grayIn = [&](sF32 u, sF32 v) -> sF32 {
    sF32 c[4];
    sampleRGBA(in, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };
  auto maskAt = [&](sF32 u, sF32 v) -> sF32 {
    if (!mask || !mask->Data)
      return 1.0f;
    sF32 c[4];
    sampleRGBA(*mask, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 uvy = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 uvx = (px + 0.5f) / w;
      sF32 c = 0.0f;
      sF32 rc[3] = {0.0f, 0.0f, 0.0f};

      for (sInt dx = -overlap; dx <= overlap; dx++) {
        for (sInt dy = -overlap; dy <= overlap; dy++) {
          // instance cell center in [-0.5,0.5] tile space
          sF32 posX = uvx * tx + (sF32)dx;
          sF32 posY = uvy * ty + (sF32)dy;
          posX = glslFract2((floorf(glslMod2(posX, tx)) + 0.5f) / tx) - 0.5f;
          posY = glslFract2((floorf(glslMod2(posY, ty)) + 0.5f) / ty) - 0.5f;

          sF32 sd[2];
          tilerRand2(posX + seed, posY + seed, sd);
          sF32 rc1[3];
          tilerRand3(sd[0], sd[1], rc1);

          const sF32 tileIdxY = floorf(uvy * ty) + (sF32)dy;
          posX += (fixedOffset / tx) *
                      glslMod2(glslMod2(tileIdxY, ty), 2.0f) +
                  offset * sd[0] / tx;
          posY += offset * sd[1] / ty;

          const sF32 m = maskAt(glslFract2(posX + 0.5f),
                                glslFract2(posY + 0.5f));
          if (m <= 0.01f)
            continue;

          sF32 pvx = glslFract2(uvx - posX) - 0.5f;
          sF32 pvy = glslFract2(uvy - posY) - 0.5f;
          tilerRand2(sd[0], sd[1], sd);
          const sF32 angle = (sd[0] * 2.0f - 1.0f) * rotateDeg *
                             0.01745329251f;
          const sF32 ca = cosf(angle), sa = sinf(angle);
          sF32 rx = ca * pvx + sa * pvy;
          sF32 ry = -sa * pvx + ca * pvy;
          const sF32 sj = (sd[1] - 0.5f) * 2.0f * scaleJitter + 1.0f;
          rx = rx * sj / scaleX + 0.5f;
          ry = ry * sj / scaleY + 0.5f;
          tilerRand2(sd[0], sd[1], sd);
          if (rx < 0.0f || rx > 1.0f || ry < 0.0f || ry > 1.0f)
            continue;

          // get_from_tileset: pick a random subtile from an NxN tileset
          sF32 su = rx, sv = ry;
          if (inputs > 1) {
            sF32 pick[2];
            tilerRand2(sd[0], sd[0], pick);
            su = (rx + floorf(pick[0] * inputs)) / inputs;
            sv = (ry + floorf(pick[1] * inputs)) / inputs;
            su = clamp01(su);
            sv = clamp01(sv);
          }

          const sF32 c1 = grayIn(su, sv) * m * (1.0f - value * sd[0]);
          if (c1 >= c) {
            c = c1;
            rc[0] = rc1[0];
            rc[1] = rc1[1];
            rc[2] = rc1[2];
          }
        }
      }

      Pixel &p = out.Data[py * w + px];
      const sU16 g = to16(c);
      p.r = p.g = p.b = g;
      p.a = 65535;
      if (outColor && outColor->Data) {
        Pixel &q = outColor->Data[py * w + px];
        q.r = to16(rc[0]);
        q.g = to16(rc[1]);
        q.b = to16(rc[2]);
        q.a = 65535;
      }
    }
  }
}

// Additive scatter tiler (the "Add Tiler" shader inside dirt.mmg).
void MMAddTiler(GenTexture &out, GenTexture *outColor, const GenTexture &in,
                const GenTexture *mask, sF32 tx, sF32 ty, sInt overlap,
                sF32 scaleX, sF32 scaleY, sF32 fixedOffset, sF32 offset,
                sF32 rotateDeg, sF32 scaleJitter, sF32 value, sF32 seed) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (tx < 1.0f)
    tx = 1.0f;
  if (ty < 1.0f)
    ty = 1.0f;
  if (scaleX < 1e-6f)
    scaleX = 1e-6f;
  if (scaleY < 1e-6f)
    scaleY = 1e-6f;

  auto grayIn = [&](sF32 u, sF32 v) -> sF32 {
    sF32 c[4];
    sampleRGBA(in, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };
  auto maskAt = [&](sF32 u, sF32 v) -> sF32 {
    if (!mask || !mask->Data)
      return 1.0f;
    sF32 c[4];
    sampleRGBA(*mask, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 uvy = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 uvx = (px + 0.5f) / w;
      sF32 c = 0.0f;
      sF32 rc[3] = {0.0f, 0.0f, 0.0f};

      for (sInt dx = -overlap; dx <= overlap; dx++) {
        for (sInt dy = -overlap; dy <= overlap; dy++) {
          sF32 posX = uvx * tx + (sF32)dx;
          sF32 posY = uvy * ty + (sF32)dy;
          posX = glslFract2((floorf(glslMod2(posX, tx)) + 0.5f) / tx) - 0.5f;
          posY = glslFract2((floorf(glslMod2(posY, ty)) + 0.5f) / ty) - 0.5f;

          sF32 sd[2];
          tilerRand2(posX + seed, posY + seed, sd);
          sF32 rc1[3];
          tilerRand3(sd[0], sd[1], rc1);

          // fract-wrapped jitter; the fixed row offset uses the cell
          // position itself, exactly as the embedded shader
          posX = glslFract2(posX +
                            (fixedOffset / tx) *
                                floorf(glslMod2(posY * ty, 2.0f)) +
                            offset * sd[0] / tx);
          posY = glslFract2(posY + offset * sd[1] / ty);

          const sF32 m = maskAt(glslFract2(posX + 0.5f),
                                glslFract2(posY + 0.5f));
          if (m <= 0.01f)
            continue;

          sF32 pvx = glslFract2(uvx - posX) - 0.5f;
          sF32 pvy = glslFract2(uvy - posY) - 0.5f;
          tilerRand2(sd[0], sd[1], sd);
          const sF32 angle = (sd[0] * 2.0f - 1.0f) * rotateDeg *
                             0.01745329251f;
          const sF32 ca = cosf(angle), sa = sinf(angle);
          sF32 rx = ca * pvx + sa * pvy;
          sF32 ry = -sa * pvx + ca * pvy;
          const sF32 sj = (sd[1] - 0.5f) * 2.0f * scaleJitter + 1.0f;
          rx = rx * sj / scaleX + 0.5f;
          ry = ry * sj / scaleY + 0.5f;
          tilerRand2(sd[0], sd[1], sd);
          if (rx < 0.0f || rx > 1.0f || ry < 0.0f || ry > 1.0f)
            continue;

          const sF32 c1 = grayIn(rx, ry) * m * (1.0f - value * sd[0]);
          c += c1;
          if (c1 >= c) {
            rc[0] = rc1[0];
            rc[1] = rc1[1];
            rc[2] = rc1[2];
          }
        }
      }

      Pixel &p = out.Data[py * w + px];
      p.r = p.g = p.b = to16(c);
      p.a = 65535;
      if (outColor && outColor->Data) {
        Pixel &q = outColor->Data[py * w + px];
        q.r = to16(rc[0]);
        q.g = to16(rc[1]);
        q.b = to16(rc[2]);
        q.a = 65535;
      }
    }
  }
}

// Directional gaussian along the heightmap slope (slope_blur.mmg).
void MMSlopeBlur(GenTexture &out, const GenTexture &in,
                 const GenTexture &height, sF32 size, sF32 sigma) {
  if (!out.Data || !in.Data || !height.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (size < 1.0f)
    size = 1.0f;
  const sF32 dx = 1.0f / size;

  auto heightAt = [&](sF32 u, sF32 v) -> sF32 {
    sF32 c[4];
    sampleRGBA(height, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 v0 = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u0 = (px + 0.5f) / w;
      const sF32 hv = heightAt(u0, v0);
      sF32 sx = heightAt(u0 + dx, v0) - hv;
      sF32 sy = heightAt(u0, v0 + dx) - hv;
      const sF32 slopeStrength = sqrtf(sx * sx + sy * sy) * size;
      sF32 nx = 0.0f, ny = 1.0f;
      if (slopeStrength != 0.0f) {
        const sF32 len = sqrtf(sx * sx + sy * sy);
        nx = sx / len;
        ny = sy / len;
      }
      const sF32 ex = dx * nx, ey = dx * ny;
      sF32 sg = sigma * slopeStrength;
      if (sg < 0.0001f)
        sg = 0.0001f;
      sF32 acc[4] = {0, 0, 0, 0};
      sF32 sum = 0.0f;
      for (sF32 i = 0.0f; i <= 50.0f; i += 1.0f) {
        const sF32 coef =
            expf(-0.5f * (i / sg) * (i / sg)) / (6.28318530718f * sg * sg);
        sF32 s[4];
        sampleRGBA(in, u0 + i * ex, v0 + i * ey, s);
        for (int k = 0; k < 4; k++)
          acc[k] += s[k] * coef;
        sum += coef;
      }
      Pixel &p = out.Data[py * w + px];
      p.r = to16(acc[0] / sum);
      p.g = to16(acc[1] / sum);
      p.b = to16(acc[2] / sum);
      p.a = to16(acc[3] / sum);
    }
  }
}

// UV mirror (mirror.mmg).
void MMMirror(GenTexture &out, const GenTexture &in, sInt direction,
              sF32 offset, bool flipSides) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 flip = flipSides ? -1.0f : 1.0f;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 su = u, sv = v;
      if (direction == 0) {
        sF32 m = fabsf(u - 0.5f) - 0.5f * offset;
        su = flip * (m > 0.0f ? m : 0.0f) + 0.5f;
      } else {
        sF32 m = fabsf(v - 0.5f) - 0.5f * offset;
        sv = flip * (m > 0.0f ? m : 0.0f) + 0.5f;
      }
      sF32 c[4];
      sampleRGBA(in, su, sv, c);
      Pixel &p = out.Data[py * w + px];
      p.r = to16(c[0]);
      p.g = to16(c[1]);
      p.b = to16(c[2]);
      p.a = to16(c[3]);
    }
  }
}

// Edge detection (edge_detect.mmg).
void MMEdgeDetect(GenTexture &out, const GenTexture &in, sF32 size,
                  sInt width, sF32 threshold) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (size < 1.0f)
    size = 1.0f;
  if (width < 1)
    width = 1;
  const sF32 ex = 1.0f / size;

  auto colorAt = [&](sF32 u, sF32 v, sF32 c[3]) {
    sF32 s[4];
    sampleRGBA(in, u, v, s);
    c[0] = s[0];
    c[1] = s[1];
    c[2] = s[2];
  };
  auto dist = [](const sF32 a[3], const sF32 b[3]) {
    const sF32 dr = a[0] - b[0], dg = a[1] - b[1], db = a[2] - b[2];
    return sqrtf(dr * dr + dg * dg + db * db);
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 ref[3];
      colorAt(u, v, ref);
      sF32 rv = 0.0f;
      sF32 dx = 0.0f, dy = 0.0f;
      for (sInt i = 0; i < width; i++) {
        dx += ex;
        dy -= ex;
        sF32 c[3];
        // e.xy=(dx,dy), e.xx=(dx,dx), e.xz=(dx,0), e.zx=(0,dx)
        colorAt(u + dx, v + dy, c); rv += dist(c, ref);
        colorAt(u - dx, v - dy, c); rv += dist(c, ref);
        colorAt(u + dx, v + dx, c); rv += dist(c, ref);
        colorAt(u - dx, v - dx, c); rv += dist(c, ref);
        colorAt(u + dx, v, c); rv += dist(c, ref);
        colorAt(u - dx, v, c); rv += dist(c, ref);
        colorAt(u, v + dx, c); rv += dist(c, ref);
        colorAt(u, v - dx, c); rv += dist(c, ref);
        rv *= 2.0f;
      }
      rv *= powf(2.0f, -(sF32)width);
      sF32 o = 100.0f * (rv - threshold);
      const sU16 g = to16(o);
      Pixel &p = out.Data[py * w + px];
      p.r = p.g = p.b = g;
      p.a = 65535;
    }
  }
}

// Linear remap with optional quantization (remap.mmg).
void MMRemap(GenTexture &out, const GenTexture &in, sF32 minV, sF32 maxV,
             sF32 step) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 st = step > 0.00000001f ? step : 0.00000001f;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 c[4];
      sampleRGBA(in, u, v, c);
      const sF32 x = ((c[0] + c[1] + c[2]) / 3.0f) * (maxV - minV);
      const sF32 r = minV + x - (x - st * floorf(x / st));
      const sU16 g = to16(step > 0.00000001f ? r : minV + x);
      Pixel &p = out.Data[py * w + px];
      p.r = p.g = p.b = g;
      p.a = 65535;
    }
  }
}

// Quadrant packing (tile2x2.mmg).
void MMTile2x2(GenTexture &out, const GenTexture *in1, const GenTexture *in2,
               const GenTexture *in3, const GenTexture *in4) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const GenTexture *src;
      sF32 su, sv;
      if (v < 0.5f) {
        src = u < 0.5f ? in1 : in2;
        su = 2.0f * u - (u < 0.5f ? 0.0f : 1.0f);
        sv = 2.0f * v;
      } else {
        src = u < 0.5f ? in3 : in4;
        su = 2.0f * u - (u < 0.5f ? 0.0f : 1.0f);
        sv = 2.0f * v - 1.0f;
      }
      Pixel &p = out.Data[py * w + px];
      if (src && src->Data) {
        sF32 c[4];
        sampleRGBA(*src, su, sv, c);
        p.r = to16(c[0]);
        p.g = to16(c[1]);
        p.b = to16(c[2]);
        p.a = to16(c[3]);
      } else {
        p.r = p.g = p.b = 0;
        p.a = 65535;
      }
    }
  }
}

// Normal map convention conversion (normal_map_convert.mmg).
void MMNormalConvert(GenTexture &out, const GenTexture &in, sInt op) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 c[4];
      sampleRGBA(in, u, v, c);
      Pixel &p = out.Data[py * w + px];
      if (op == 0) { // from/to OpenGL: flip R and B
        p.r = to16(1.0f - c[0]);
        p.g = to16(c[1]);
        p.b = to16(1.0f - c[2]);
      } else if (op == 1) { // from/to DirectX: flip all
        p.r = to16(1.0f - c[0]);
        p.g = to16(1.0f - c[1]);
        p.b = to16(1.0f - c[2]);
      } else if (op == 2) { // OpenGL <-> DirectX: flip G only
        p.r = to16(c[0]);
        p.g = to16(1.0f - c[1]);
        p.b = to16(c[2]);
      } else { // identity (op 3): keep the map as-is
        p.r = to16(c[0]);
        p.g = to16(c[1]);
        p.b = to16(c[2]);
      }
      p.a = to16(c[3]);
    }
  }
}

// Per-region UV scatter (custom_uv.mmg).
void MMCustomUV(GenTexture &out, const GenTexture &in, const GenTexture &map,
                sInt inputs, sF32 sx, sF32 sy, sF32 rotateDeg,
                sF32 scaleJitter, sF32 seed) {
  if (!out.Data || !in.Data || !map.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (inputs < 1)
    inputs = 1;
  if (fabsf(sx) < 1e-6f)
    sx = 1e-6f;
  if (fabsf(sy) < 1e-6f)
    sy = 1e-6f;
  const sF32 rotRad = rotateDeg * 0.01745329251f;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 m[4];
      sampleRGBA(map, u, v, m);

      // old_custom_uv_transform(map.xy, (sx,sy), rot, scale,
      //                         (map.z, seed))
      sF32 sd[2];
      tilerRand2(m[2], seed, sd);
      sF32 cu = m[0] - 0.5f, cv = m[1] - 0.5f;
      const sF32 angle = (sd[0] * 2.0f - 1.0f) * rotRad;
      const sF32 ca = cosf(angle), sa = sinf(angle);
      sF32 ru = ca * cu + sa * cv;
      sF32 rv = -sa * cu + ca * cv;
      const sF32 sj = (sd[1] - 0.5f) * 2.0f * scaleJitter + 1.0f;
      ru = ru * sj / sx + 0.5f;
      rv = rv * sj / sy + 0.5f;

      // get_from_tileset(inputs, seed + map.z, uv) — MM clamps the
      // final uv to [0,1] even for a single input, so out-of-range
      // transforms sample the image edge instead of wrapping around
      // (a sphere's black border, not the middle of the sphere).
      sF32 su = clamp01(ru), sv = clamp01(rv);
      if (inputs > 1) {
        sF32 pick[2];
        tilerRand2(seed + m[2], seed + m[2], pick);
        su = clamp01((ru + floorf(pick[0] * inputs)) / inputs);
        sv = clamp01((rv + floorf(pick[1] * inputs)) / inputs);
      }

      sF32 c[4];
      sampleRGBA(in, su, sv, c);
      Pixel &p = out.Data[py * w + px];
      p.r = to16(c[0]);
      p.g = to16(c[1]);
      p.b = to16(c[2]);
      p.a = to16(c[3]);
    }
  }
}

// Smooth curvature (smooth_curvature2.mmg inner shader).
void MMSmoothCurvature(GenTexture &out, const GenTexture &height,
                       sF32 quality, sF32 strength, sF32 radius) {
  if (!out.Data || !height.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (quality < 1.0f)
    quality = 1.0f;
  if (radius < 1e-4f)
    radius = 1e-4f;
  const sF32 r = 0.05f * radius;
  const sF32 s = r / quality;

  auto heightAt = [&](sF32 u, sF32 v) -> sF32 {
    sF32 c[4];
    sampleRGBA(height, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 v0 = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u0 = (px + 0.5f) / w;
      const sF32 H = heightAt(u0, v0) * 4.0f;
      sF32 acc = 0.0f;
      for (sF32 oy = 0.0f; oy < quality; oy += 1.0f) {
        for (sF32 ox = 0.0f; ox < quality; ox += 1.0f) {
          const sF32 dx = ox * s, dy = oy * s;
          const sF32 curve = -heightAt(u0 + dx, v0 + dy) -
                             heightAt(u0 - dx, v0 - dy) -
                             heightAt(u0 + dx, v0 - dy) -
                             heightAt(u0 - dx, v0 + dy);
          const sF32 dist = sqrtf(dx * dx + dy * dy);
          acc += (H + curve) * ((r - dist) / r);
        }
      }
      const sF32 c = (acc / (quality * quality)) * strength / radius;
      const sU16 g = to16(0.5f + c);
      Pixel &p = out.Data[py * w + px];
      p.r = p.g = p.b = g;
      p.a = 65535;
    }
  }
}

// Blur-based ambient occlusion approximation.
void MMAmbientOcclusion(GenTexture &out, const GenTexture &height,
                        sF32 radius, sF32 strength) {
  if (!out.Data || !height.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  sInt rad = (sInt)(radius * height.XRes);
  if (rad < 1)
    rad = 1;

  // separable box blur of the height grayscale
  const sInt hw = height.XRes, hh = height.YRes;
  std::vector<sF32> gray((size_t)hw * hh), tmp((size_t)hw * hh),
      blur((size_t)hw * hh);
  const sF32 sc = 1.0f / (3.0f * 65535.0f);
  for (sInt i = 0; i < hw * hh; i++) {
    const Pixel &p = height.Data[i];
    gray[i] = ((sF32)p.r + (sF32)p.g + (sF32)p.b) * sc;
  }
  const sF32 norm = 1.0f / (2 * rad + 1);
  for (sInt y = 0; y < hh; y++)
    for (sInt x = 0; x < hw; x++) {
      sF32 acc = 0.0f;
      for (sInt k = -rad; k <= rad; k++)
        acc += gray[y * hw + wrapi(x + k, hw)];
      tmp[y * hw + x] = acc * norm;
    }
  for (sInt y = 0; y < hh; y++)
    for (sInt x = 0; x < hw; x++) {
      sF32 acc = 0.0f;
      for (sInt k = -rad; k <= rad; k++)
        acc += tmp[wrapi(y + k, hh) * hw + x];
      blur[y * hw + x] = acc * norm;
    }

  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      const sInt sx = px * hw / w, sy = py * hh / h;
      const sF32 d = blur[sy * hw + sx] - gray[sy * hw + sx];
      const sF32 occ = clamp01(1.0f - strength * (d > 0.0f ? d : 0.0f) * 4.0f);
      Pixel &p = out.Data[py * w + px];
      const sU16 g = to16(occ);
      p.r = p.g = p.b = g;
      p.a = 65535;
    }
  }
}

// Levels adjustment (tones.mmg adjust_levels).
void MMLevels(GenTexture &out, const GenTexture &in, const sF32 inMin[4],
              const sF32 inMid[4], const sF32 inMax[4], const sF32 outMin[4],
              const sF32 outMax[4]) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 c[4];
      sampleRGBA(in, u, v, c);
      sF32 o[4];
      for (int k = 0; k < 4; k++) {
        sF32 range = inMax[k] - inMin[k];
        if (fabsf(range) < 1e-6f)
          range = range < 0.0f ? -1e-6f : 1e-6f;
        sF32 x = clamp01((c[k] - inMin[k]) / range);
        sF32 mid = (inMid[k] - inMin[k]) / range;
        if (mid < 1e-4f)
          mid = 1e-4f;
        if (mid > 1.0f - 1e-4f)
          mid = 1.0f - 1e-4f;
        x = x >= mid ? 0.5f * (1.0f + (x - mid) / (1.0f - mid))
                     : 0.5f * (x / mid);
        o[k] = outMin[k] + x * (outMax[k] - outMin[k]);
      }
      Pixel &p = out.Data[py * w + px];
      p.r = to16(o[0]);
      p.g = to16(o[1]);
      p.b = to16(o[2]);
      p.a = to16(o[3]);
    }
  }
}

// Advanced tiler (tiler_advanced.mmg).
void MMTilerAdvanced(GenTexture &out, GenTexture *outColor1,
                     GenTexture *outColor2, GenTexture *outUV,
                     const GenTexture &in, const GenTexture *mask,
                     const GenTexture *trX, const GenTexture *trY,
                     const GenTexture *rMap, const GenTexture *scX,
                     const GenTexture *scY, const GenTexture *color1,
                     const GenTexture *color2, sF32 tx, sF32 ty,
                     sInt overlap, sInt inputs, sF32 translateX,
                     sF32 translateY, sF32 rotateDeg, sF32 scaleX,
                     sF32 scaleY, sF32 seed) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  if (tx < 1.0f)
    tx = 1.0f;
  if (ty < 1.0f)
    ty = 1.0f;
  if (inputs < 1)
    inputs = 1;

  auto grayAtOr = [](const GenTexture *t, sF32 u, sF32 v, sF32 def) {
    if (!t || !t->Data)
      return def;
    sF32 c[4];
    sampleRGBA(*t, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };
  auto grayIn = [&](sF32 u, sF32 v) {
    sF32 c[4];
    sampleRGBA(in, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 uvy = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 uvx = (px + 0.5f) / w;
      sF32 c = 0.0f;
      sF32 mapU = 0.0f, mapV = 0.0f;
      sF32 instU = 0.0f, instV = 0.0f, instS = 0.0f;

      for (sInt dx = -overlap; dx <= overlap; dx++) {
        for (sInt dy = -overlap; dy <= overlap; dy++) {
          sF32 posX = uvx * tx + (sF32)dx;
          sF32 posY = uvy * ty + (sF32)dy;
          posX = glslFract2((floorf(glslMod2(posX, tx)) + 0.5f) / tx) - 0.5f;
          posY = glslFract2((floorf(glslMod2(posY, ty)) + 0.5f) / ty) - 0.5f;

          const sF32 m =
              grayAtOr(mask, glslFract2(posX + 0.5f), glslFract2(posY + 0.5f),
                       1.0f);
          if (m <= 0.01f)
            continue;

          sF32 pvx = glslFract2(uvx - posX) - 0.5f;
          sF32 pvy = glslFract2(uvy - posY) - 0.5f;
          const sF32 cellU = glslFract2(posX + 0.5f);
          const sF32 cellV = glslFract2(posY + 0.5f);

          pvx -= translateX * grayAtOr(trX, cellU, cellV, 1.0f) / tx;
          pvy -= translateY * grayAtOr(trY, cellU, cellV, 1.0f) / ty;
          const sF32 angle = grayAtOr(rMap, cellU, cellV, 1.0f) * rotateDeg *
                             0.01745329251f;
          const sF32 ca = cosf(angle), sa = sinf(angle);
          sF32 rx = ca * pvx + sa * pvy;
          sF32 ry = -sa * pvx + ca * pvy;
          sF32 sxv = scaleX * grayAtOr(scX, cellU, cellV, 1.0f);
          sF32 syv = scaleY * grayAtOr(scY, cellU, cellV, 1.0f);
          if (fabsf(sxv) < 1e-6f)
            sxv = 1e-6f;
          if (fabsf(syv) < 1e-6f)
            syv = 1e-6f;
          rx = rx / sxv + 0.5f;
          ry = ry / syv + 0.5f;
          if (rx < 0.0f || rx > 1.0f || ry < 0.0f || ry > 1.0f)
            continue;

          sF32 sd[2];
          tilerRand2(seed + cellU, seed + cellV, sd);
          sF32 su = rx, sv = ry;
          if (inputs > 1) {
            sF32 pick[2];
            tilerRand2(sd[0], sd[0], pick);
            su = clamp01((rx + floorf(pick[0] * inputs)) / inputs);
            sv = clamp01((ry + floorf(pick[1] * inputs)) / inputs);
          }

          const sF32 c1 = grayIn(su, sv) * m;
          if (c1 >= c) {
            c = c1;
            mapU = cellU;
            mapV = cellV;
            instU = rx;
            instV = ry;
            instS = sd[0];
          }
        }
      }

      const sInt idx = py * w + px;
      {
        Pixel &p = out.Data[idx];
        const sU16 g = to16(c);
        p.r = p.g = p.b = g;
        p.a = 65535;
      }
      auto writeColorOut = [&](GenTexture *dst, const GenTexture *src,
                               bool alt) {
        if (!dst || !dst->Data)
          return;
        Pixel &p = dst->Data[idx];
        if (src && src->Data) {
          sF32 cc[4];
          sampleRGBA(*src, mapU, mapV, cc);
          p.r = to16(cc[0]);
          p.g = to16(cc[1]);
          p.b = to16(cc[2]);
          p.a = to16(cc[3]);
        } else {
          // MM default: random color per instance
          sF32 rc[3];
          tilerRand3(alt ? -mapU : mapU + (alt ? -1.0f : 0.0f),
                     alt ? -mapV : mapV, rc);
          p.r = to16(rc[0]);
          p.g = to16(rc[1]);
          p.b = to16(rc[2]);
          p.a = 65535;
        }
      };
      writeColorOut(outColor1, color1, false);
      writeColorOut(outColor2, color2, true);
      if (outUV && outUV->Data) {
        Pixel &p = outUV->Data[idx];
        p.r = to16(instU);
        p.g = to16(instV);
        p.b = to16(instS);
        p.a = 65535;
      }
    }
  }
}

// Offset toward a height contour (height_to_offset.mmg).
void MMHeightToOffset(GenTexture &outX, GenTexture &outY,
                      const GenTexture &in, sF32 target) {
  if (!outX.Data || !outY.Data || !in.Data)
    return;
  const sInt w = outX.XRes, h = outX.YRes;
  const sF32 eps = 0.001f;

  auto gray = [&](sF32 u, sF32 v) {
    sF32 c[4];
    sampleRGBA(in, u, v, c);
    return (c[0] + c[1] + c[2]) / 3.0f;
  };

  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const sF32 start = gray(u, v);
      sF32 dhx = (gray(u + eps, v) - gray(u - eps, v)) / (2.0f * eps);
      sF32 dhy = (gray(u, v + eps) - gray(u, v - eps)) / (2.0f * eps);
      const sF32 dd = dhx * dhx + dhy * dhy;
      sF32 ox = 0.0f, oy = 0.0f;
      if (dd > 1e-8f) {
        const sF32 k = 16.0f * (target - start) / dd;
        ox = k * dhx;
        oy = k * dhy;
      }
      const sInt idx = py * w + px;
      Pixel &pX = outX.Data[idx];
      const sU16 gx = to16(ox);
      pX.r = pX.g = pX.b = gx;
      pX.a = 65535;
      Pixel &pY = outY.Data[idx];
      const sU16 gy = to16(oy);
      pY.r = pY.g = pY.b = gy;
      pY.a = 65535;
    }
  }
}

// ============================================================
// Curve + Bevel
// ============================================================

sF32 MMCurveEval(const MMCurvePoint *pts, sInt n, sF32 x) {
  if (!pts || n <= 0)
    return x;
  if (n == 1 || x <= pts[0].x)
    return n == 1 ? pts[0].y : pts[0].y;
  if (x >= pts[n - 1].x)
    return pts[n - 1].y;
  sInt i = 0;
  while (i + 2 < n && x > pts[i + 1].x)
    i++;
  const MMCurvePoint &p0 = pts[i];
  const MMCurvePoint &p1 = pts[i + 1];
  const sF32 dx = p1.x - p0.x;
  if (dx <= 1e-8f)
    return p1.y;
  const sF32 t = (x - p0.x) / dx;
  const sF32 d0 = p0.rs * dx;
  const sF32 d1 = p1.ls * dx;
  const sF32 t2 = t * t, t3 = t2 * t;
  return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0.y + (t3 - 2.0f * t2 + t) * d0 +
         (-2.0f * t3 + 3.0f * t2) * p1.y + (t3 - t2) * d1;
}

// 1D squared-distance transform (Felzenszwalb & Huttenlocher).
// 'a' (optional) receives the argmin site index per output sample.
static void mmDt1d(const sF32 *f, sF32 *d, sInt *v, sF32 *z, sInt n,
                   sInt *a = nullptr) {
  sInt k = 0;
  v[0] = 0;
  z[0] = -1e20f;
  z[1] = 1e20f;
  for (sInt q = 1; q < n; q++) {
    sF32 s;
    for (;;) {
      s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2.0f * q - 2.0f * v[k]);
      if (s <= z[k] && k > 0) {
        k--;
        continue;
      }
      break;
    }
    if (s <= z[k]) {
      v[k] = q; // degenerate: replace
    } else {
      k++;
      v[k] = q;
    }
    z[k] = s;
    z[k + 1] = 1e20f;
  }
  k = 0;
  for (sInt q = 0; q < n; q++) {
    while (z[k + 1] < q)
      k++;
    const sF32 dq = (sF32)(q - v[k]);
    d[q] = dq * dq + f[v[k]];
    if (a)
      a[q] = v[k];
  }
}

void MMBevel(GenTexture &out, const GenTexture &in, sF32 distance,
             const MMCurvePoint *curve, sInt nCurve) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 INF = 1e18f;

  // squared EDT to the gray >= 0.5 region; toroidal via 3x tiling of
  // each 1D pass (max distance < one tile, so one wrap suffices)
  std::vector<sF32> col((size_t)w * h), dist((size_t)w * h);
  {
    const sInt n3 = (h > w ? h : w) * 3;
    std::vector<sF32> f(n3), d(n3), z(n3 + 1);
    std::vector<sInt> v(n3);
    // vertical pass
    for (sInt x = 0; x < w; x++) {
      for (sInt rep = 0; rep < 3; rep++)
        for (sInt y = 0; y < h; y++) {
          sF32 c[4];
          sampleRGBA(in, (x + 0.5f) / w, (y + 0.5f) / h, c);
          const sF32 g = (c[0] + c[1] + c[2]) / 3.0f;
          f[rep * h + y] = g >= 0.5f ? 0.0f : INF;
        }
      mmDt1d(f.data(), d.data(), v.data(), z.data(), 3 * h);
      for (sInt y = 0; y < h; y++)
        col[(size_t)y * w + x] = d[h + y];
    }
    // horizontal pass over the vertical distances
    for (sInt y = 0; y < h; y++) {
      for (sInt rep = 0; rep < 3; rep++)
        for (sInt x = 0; x < w; x++)
          f[rep * w + x] = col[(size_t)y * w + x];
      mmDt1d(f.data(), d.data(), v.data(), z.data(), 3 * w);
      for (sInt x = 0; x < w; x++)
        dist[(size_t)y * w + x] = d[w + x];
    }
  }

  const sF32 radius = distance * (sF32)w;
  for (sInt y = 0; y < h; y++)
    for (sInt x = 0; x < w; x++) {
      const size_t idx = (size_t)y * w + x;
      sF32 ramp = 0.0f;
      if (radius > 1e-6f) {
        ramp = 1.0f - sqrtf(dist[idx]) / radius;
        ramp = ramp < 0.0f ? 0.0f : (ramp > 1.0f ? 1.0f : ramp);
      } else {
        ramp = dist[idx] <= 0.0f ? 1.0f : 0.0f;
      }
      const sF32 g = clamp01(MMCurveEval(curve, nCurve, ramp));
      Pixel &px = out.Data[idx];
      px.r = px.g = px.b = to16(g);
      px.a = 65535;
    }
}

void MMDilate(GenTexture &out, const GenTexture &mask,
              const GenTexture *source, sF32 length, sF32 fill, sInt metric) {
  if (!out.Data || !mask.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 INF = 1e18f;

  // nearest-site squared EDT to the gray >= 0.5 region; toroidal via
  // 3x tiling. The vertical pass records the column-nearest site row,
  // the horizontal pass the winning site column — together they name
  // the exact euclidean-nearest white pixel.
  std::vector<sF32> col((size_t)w * h);
  std::vector<sInt> dyv((size_t)w * h), dxh((size_t)w * h);
  bool any = false;
  {
    const sInt n3 = (h > w ? h : w) * 3;
    std::vector<sF32> f(n3), d(n3), z(n3 + 1);
    std::vector<sInt> v(n3), a(n3);
    for (sInt x = 0; x < w; x++) {
      for (sInt rep = 0; rep < 3; rep++)
        for (sInt y = 0; y < h; y++) {
          sF32 c[4];
          sampleRGBA(mask, (x + 0.5f) / w, (y + 0.5f) / h, c);
          const sF32 g = (c[0] + c[1] + c[2]) / 3.0f;
          if (g >= 0.5f)
            any = true;
          f[rep * h + y] = g >= 0.5f ? 0.0f : INF;
        }
      mmDt1d(f.data(), d.data(), v.data(), z.data(), 3 * h, a.data());
      for (sInt y = 0; y < h; y++) {
        col[(size_t)y * w + x] = d[h + y];
        dyv[(size_t)y * w + x] = a[h + y] - (h + y);
      }
    }
    for (sInt y = 0; y < h; y++) {
      for (sInt rep = 0; rep < 3; rep++)
        for (sInt x = 0; x < w; x++)
          f[rep * w + x] = col[(size_t)y * w + x];
      mmDt1d(f.data(), d.data(), v.data(), z.data(), 3 * w, a.data());
      for (sInt x = 0; x < w; x++)
        dxh[(size_t)y * w + x] = a[w + x] - (w + x);
    }
  }

  const sF32 fillA = clamp01(fill);
  for (sInt y = 0; y < h; y++)
    for (sInt x = 0; x < w; x++) {
      const size_t idx = (size_t)y * w + x;
      Pixel &px = out.Data[idx];
      if (!any) {
        px.r = px.g = px.b = 0;
        px.a = 65535;
        continue;
      }
      const sInt dx = dxh[idx];
      const sInt xs = ((x + dx) % w + w) % w;
      const sInt dy = dyv[(size_t)y * w + xs];
      const sInt ys = ((y + dy) % h + h) % h;
      const sF32 du = (sF32)(dx < 0 ? -dx : dx) / (sF32)w;
      const sF32 dv = (sF32)(dy < 0 ? -dy : dy) / (sF32)h;
      sF32 md;
      if (metric == 1)
        md = du + dv;
      else if (metric == 2)
        md = du > dv ? du : dv;
      else
        md = sqrtf(du * du + dv * dv);
      sF32 ramp;
      if (length > 1e-6f)
        ramp = clamp01(1.0f - md / length);
      else
        ramp = md <= 0.0f ? 1.0f : 0.0f;
      const sF32 fac = ramp + (1.0f - ramp) * fillA;
      sF32 c[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      if (source && source->Data)
        sampleRGBA(*source, (xs + 0.5f) / w, (ys + 0.5f) / h, c);
      px.r = to16(clamp01(c[0]) * fac);
      px.g = to16(clamp01(c[1]) * fac);
      px.b = to16(clamp01(c[2]) * fac);
      px.a = 65535;
    }
}

void MMAnisotropicKuwahara(GenTexture &out, const GenTexture &in,
                           sF32 sizePx, sInt kernel, sF32 sharpness,
                           sF32 eccentricity, sF32 uniformity) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 d = 1.0f / (sizePx > 1.0f ? sizePx : 256.0f);

  auto rgbAt = [&](sF32 u, sF32 v, sF32 c[3]) {
    sF32 t[4];
    sampleRGBA(in, u, v, t);
    c[0] = t[0];
    c[1] = t[1];
    c[2] = t[2];
  };

  // 1) structure tensor from RGB sobel (float precision — the tensor
  //    values exceed the 16-bit texture range)
  std::vector<sF32> txx((size_t)w * h), tyy((size_t)w * h),
      txy((size_t)w * h);
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 c[3], sx[3] = {0, 0, 0}, sy[3] = {0, 0, 0};
      static const sF32 ox[6] = {-1, -1, -1, 1, 1, 1};
      static const sF32 oy[6] = {-1, 0, 1, -1, 0, 1};
      static const sF32 wx[6] = {1, 2, 1, -1, -2, -1};
      for (sInt k = 0; k < 6; k++) {
        rgbAt(u + ox[k] * d, v + oy[k] * d, c);
        for (sInt i = 0; i < 3; i++)
          sx[i] += wx[k] * c[i];
        rgbAt(u + oy[k] * d, v + ox[k] * d, c);
        for (sInt i = 0; i < 3; i++)
          sy[i] += wx[k] * c[i];
      }
      for (sInt i = 0; i < 3; i++) {
        sx[i] *= 0.25f;
        sy[i] *= 0.25f;
      }
      const size_t idx = (size_t)py * w + px;
      txx[idx] = sx[0] * sx[0] + sx[1] * sx[1] + sx[2] * sx[2];
      tyy[idx] = sy[0] * sy[0] + sy[1] * sy[1] + sy[2] * sy[2];
      txy[idx] = sx[0] * sy[0] + sx[1] * sy[1] + sx[2] * sy[2];
    }
  }

  // 2) gaussian blur of the tensor field, sigma = uniformity texels
  //    at the reference resolution (scaled to our resolution)
  {
    const sF32 sigma = uniformity * (sF32)w * d;
    if (sigma > 0.01f) {
      const sInt rad = (sInt)(sigma * 3.0f) + 1;
      std::vector<sF32> kern(rad + 1);
      for (sInt i = 0; i <= rad; i++)
        kern[i] = expf(-0.5f * (i / sigma) * (i / sigma));
      auto blur1d = [&](std::vector<sF32> &f, bool horiz) {
        std::vector<sF32> tmp = f;
        for (sInt py = 0; py < h; py++)
          for (sInt px = 0; px < w; px++) {
            sF32 acc = 0.0f, wsum = 0.0f;
            for (sInt i = -rad; i <= rad; i++) {
              const sInt xx = horiz ? (px + i + w * 8) % w : px;
              const sInt yy = horiz ? py : (py + i + h * 8) % h;
              const sF32 kw = kern[i < 0 ? -i : i];
              acc += tmp[(size_t)yy * w + xx] * kw;
              wsum += kw;
            }
            f[(size_t)py * w + px] = acc / wsum;
          }
      };
      blur1d(txx, true);
      blur1d(txx, false);
      blur1d(tyy, true);
      blur1d(tyy, false);
      blur1d(txy, true);
      blur1d(txy, false);
    }
  }

  // 3) main pass (Gunnell's 8-sector polynomial weighting)
  const sF32 sharp = 3.0f + 15.0f * clamp01(sharpness);
  const sInt kernelSize = kernel * 2 > 0 ? kernel * 2 : 0;
  const sF32 alpha = 1.0f, hardness = 8.0f, zeroCross = 0.58f;
  const sF32 zeta = kernelSize > 0 ? 1.0f / ((sF32)kernelSize / 2.0f) : 1.0f;
  const sF32 ecc = 0.7f + 0.8f * clamp01(eccentricity * 0.5f);
  const sF32 sinZC = sinf(zeroCross);
  const sF32 eta = (zeta + cosf(zeroCross)) / (sinZC * sinZC);

  for (sInt py = 0; py < h; py++) {
    const sF32 uvY = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 uvX = (px + 0.5f) / w;
      const size_t idx = (size_t)py * w + px;
      const sF32 gx = txx[idx], gy = tyy[idx], gz = txy[idx];
      const sF32 disc =
          sqrtf(gy * gy - 2.0f * gx * gy + gx * gx + 4.0f * gz * gz);
      const sF32 l1 = 0.5f * (gy + gx + disc);
      const sF32 l2 = 0.5f * (gy + gx - disc);
      sF32 vx = l1 - gx, vy = -gz;
      const sF32 vlen = sqrtf(vx * vx + vy * vy);
      if (vlen > 0.0f) {
        vx /= vlen;
        vy /= vlen;
      } else {
        vx = 0.0f;
        vy = 1.0f;
      }
      const sF32 A = (l1 + l2 > 0.0f) ? (l1 - l2) / (l1 + l2) : 0.0f;

      const sInt kernelRadius = kernelSize / 2;
      auto clampf = [](sF32 x, sF32 lo, sF32 hi) {
        return x < lo ? lo : (x > hi ? hi : x);
      };
      const sF32 a = kernelRadius * clampf((alpha + A) / alpha, 0.1f, 2.0f);
      const sF32 b = kernelRadius * clampf(alpha / (alpha + A), 0.1f, 2.0f);
      const sF32 cp = vx, sp = vy;
      // SR = S * R with GLSL column-major mats
      const sF32 s00 = 0.5f / a / ecc, s11 = 0.5f / b * ecc;
      const sF32 sr00 = s00 * cp, sr01 = -s11 * sp;
      const sF32 sr10 = s00 * sp, sr11 = s11 * cp;
      const sInt maxX = (sInt)sqrtf(a * a * cp * cp + b * b * sp * sp);
      const sInt maxY = (sInt)sqrtf(a * a * sp * sp + b * b * cp * cp);

      sF32 m[8][4], s2[8][3];
      for (sInt k = 0; k < 8; k++) {
        m[k][0] = m[k][1] = m[k][2] = m[k][3] = 0.0f;
        s2[k][0] = s2[k][1] = s2[k][2] = 0.0f;
      }

      for (sInt y = -maxY; y <= maxY; y++)
        for (sInt x = -maxX; x <= maxX; x++) {
          sF32 wvx = sr00 * (sF32)x + sr10 * (sF32)y;
          sF32 wvy = sr01 * (sF32)x + sr11 * (sF32)y;
          if (wvx * wvx + wvy * wvy > 0.125f)
            continue;
          sF32 c[3];
          rgbAt(uvX + x * d, uvY + y * d, c);
          for (sInt i = 0; i < 3; i++)
            c[i] = clamp01(c[i]);
          sF32 wgt[8], sum = 0.0f, z, vxx, vyy;
          vxx = zeta - eta * wvx * wvx;
          vyy = zeta - eta * wvy * wvy;
          z = wvy + vxx > 0.0f ? wvy + vxx : 0.0f;
          wgt[0] = z * z;
          z = -wvx + vyy > 0.0f ? -wvx + vyy : 0.0f;
          wgt[2] = z * z;
          z = -wvy + vxx > 0.0f ? -wvy + vxx : 0.0f;
          wgt[4] = z * z;
          z = wvx + vyy > 0.0f ? wvx + vyy : 0.0f;
          wgt[6] = z * z;
          const sF32 rx = 0.70710678f * (wvx - wvy);
          const sF32 ry = 0.70710678f * (wvx + wvy);
          vxx = zeta - eta * rx * rx;
          vyy = zeta - eta * ry * ry;
          z = ry + vxx > 0.0f ? ry + vxx : 0.0f;
          wgt[1] = z * z;
          z = -rx + vyy > 0.0f ? -rx + vyy : 0.0f;
          wgt[3] = z * z;
          z = -ry + vxx > 0.0f ? -ry + vxx : 0.0f;
          wgt[5] = z * z;
          z = rx + vyy > 0.0f ? rx + vyy : 0.0f;
          wgt[7] = z * z;
          for (sInt k = 0; k < 8; k++)
            sum += wgt[k];
          if (sum <= 0.0f)
            continue;
          const sF32 g =
              expf(-3.125f * (wvx * wvx + wvy * wvy)) / sum;
          for (sInt k = 0; k < 8; k++) {
            const sF32 wk = wgt[k] * g;
            for (sInt i = 0; i < 3; i++) {
              m[k][i] += c[i] * wk;
              s2[k][i] += c[i] * c[i] * wk;
            }
            m[k][3] += wk;
          }
        }

      sF32 res[4] = {0, 0, 0, 0};
      for (sInt k = 0; k < 8; k++) {
        if (m[k][3] <= 0.0f)
          continue;
        sF32 mean[3], var = 0.0f;
        for (sInt i = 0; i < 3; i++) {
          mean[i] = m[k][i] / m[k][3];
          sF32 sv = s2[k][i] / m[k][3] - mean[i] * mean[i];
          var += sv < 0.0f ? -sv : sv;
        }
        const sF32 wk =
            1.0f / (1.0f + powf(hardness * 1000.0f * var, 0.5f * sharp));
        for (sInt i = 0; i < 3; i++)
          res[i] += mean[i] * wk;
        res[3] += wk;
      }
      Pixel &p = out.Data[idx];
      if (res[3] > 0.0f) {
        p.r = to16(clamp01(res[0] / res[3]));
        p.g = to16(clamp01(res[1] / res[3]));
        p.b = to16(clamp01(res[2] / res[3]));
      } else {
        sF32 c[3];
        rgbAt(uvX, uvY, c);
        p.r = to16(c[0]);
        p.g = to16(c[1]);
        p.b = to16(c[2]);
      }
      p.a = 65535;
    }
  }
}

void MMBinarySmooth(GenTexture &out, const GenTexture &in, sF32 sizePx,
                    sF32 smoothPx, sF32 offset, sF32 bevel) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  GenTexture th;
  th.Init(w, h);
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      sF32 c[4];
      sampleRGBA(in, (px + 0.5f) / w, (py + 0.5f) / h, c);
      const sF32 g = (c[0] + c[1] + c[2]) / 3.0f;
      Pixel &p = th.Data[(size_t)py * w + px];
      p.r = p.g = p.b = g > 0.5f ? 65535 : 0;
      p.a = 65535;
    }
  GenTexture bl;
  bl.Init(w, h);
  const sF32 s = smoothPx / (sizePx > 1.0f ? sizePx : 256.0f);
  bl.Blur(th, s, s, 2, 3);
  // tones_step: linear ramp of width 'bevel' centered at 'offset'
  const sF32 bw = bevel > 1e-6f ? bevel : 1e-6f;
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      const Pixel &b = bl.Data[(size_t)py * w + px];
      const sF32 g = b.r / 65535.0f;
      const sF32 v = clamp01((g - offset) / bw + 0.5f);
      Pixel &p = out.Data[(size_t)py * w + px];
      p.r = p.g = p.b = to16(v);
      p.a = 65535;
    }
}

void MMEdgeDetect2(GenTexture &out, const GenTexture &in, sF32 sizePx) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 e = 1.0f / (sizePx > 1.0f ? sizePx : 1.0f);
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      const sF32 su = (px + 0.5f) / w, sv = (py + 0.5f) / h;
      sF32 c[4], n1[4], n2[4], n3[4], n4[4];
      sampleRGBA(in, su, sv, c);
      sampleRGBA(in, su + e, sv, n1);
      sampleRGBA(in, su - e, sv, n2);
      sampleRGBA(in, su, sv + e, n3);
      sampleRGBA(in, su, sv - e, n4);
      sF32 mx = 0.0f;
      for (sInt k = 0; k < 3; k++) {
        const sF32 d =
            fabsf(4.0f * c[k] - n1[k] - n2[k] - n3[k] - n4[k]) * sizePx;
        mx = d > mx ? d : mx;
      }
      Pixel &o = out.Data[(size_t)py * w + px];
      o.r = o.g = o.b = to16(clamp01(mx));
      o.a = 65535;
    }
}

void MMSmoothMinMax(GenTexture &out, const GenTexture *in1,
                    const GenTexture *in2, sInt op, sF32 k, sF32 def1,
                    sF32 def2) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 kk = k > 1e-6f ? k : 1e-6f;
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      const sF32 su = (px + 0.5f) / w, sv = (py + 0.5f) / h;
      sF32 a = def1, b = def2;
      sF32 c[4];
      if (in1 && in1->Data) {
        sampleRGBA(*in1, su, sv, c);
        a = (c[0] + c[1] + c[2]) / 3.0f;
      }
      if (in2 && in2->Data) {
        sampleRGBA(*in2, su, sv, c);
        b = (c[0] + c[1] + c[2]) / 3.0f;
      }
      sF32 hmix, r;
      if (op == 1) { // smax
        hmix = clamp01(0.5f - 0.5f * (b - a) / kk);
        r = b + (a - b) * hmix + kk * hmix * (1.0f - hmix);
      } else { // smin
        hmix = clamp01(0.5f + 0.5f * (b - a) / kk);
        r = b + (a - b) * hmix - kk * hmix * (1.0f - hmix);
      }
      Pixel &o = out.Data[(size_t)py * w + px];
      o.r = o.g = o.b = to16(r);
      o.a = 65535;
    }
}

void MMDirectionalBlur(GenTexture &out, const GenTexture &in,
                       const GenTexture *amountMap, sF32 sizePx, sF32 sigma,
                       sF32 angleDeg, sInt mode) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 step = 1.0f / (sizePx > 1.0f ? sizePx : 1.0f);
  const sF32 a = angleDeg * 0.01745329251f;
  const sF32 ex = cosf(a) * step, ey = -sinf(a) * step;
  const sInt first = mode < 0 ? 0 : (mode > 1 ? 1 : mode);
  for (sInt py = 0; py < h; py++)
    for (sInt px = 0; px < w; px++) {
      const sF32 su = (px + 0.5f) / w, sv = (py + 0.5f) / h;
      sF32 sg = sigma;
      if (amountMap && amountMap->Data) {
        sF32 m[4];
        sampleRGBA(*amountMap, su, sv, m);
        sg *= (m[0] + m[1] + m[2]) / 3.0f;
      }
      Pixel &o = out.Data[(size_t)py * w + px];
      if (sg <= 1e-4f) { // degenerate gaussian: passthrough
        sF32 c[4];
        sampleRGBA(in, su, sv, c);
        o.r = to16(c[0]);
        o.g = to16(c[1]);
        o.b = to16(c[2]);
        o.a = to16(c[3]);
        continue;
      }
      sF32 rv[4] = {0, 0, 0, 0}, sum = 0.0f;
      for (sInt i = first; i <= 50; i++) {
        const sF32 t = (sF32)i / sg;
        const sF32 coef = expf(-0.5f * t * t) / (6.28318530718f * sg * sg);
        sF32 c[4];
        sampleRGBA(in, su + i * ex, sv + i * ey, c);
        for (sInt k = 0; k < 4; k++)
          rv[k] += c[k] * coef;
        sum += coef;
      }
      o.r = to16(rv[0] / sum);
      o.g = to16(rv[1] / sum);
      o.b = to16(rv[2] / sum);
      o.a = to16(rv[3] / sum);
    }
}

void MMNormalBlend(GenTexture &out, const GenTexture *fg,
                   const GenTexture *bg, const GenTexture *mask, sF32 amount) {
  if (!out.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt y = 0; y < h; y++)
    for (sInt x = 0; x < w; x++) {
      const sF32 su = (x + 0.5f) / w, sv = (y + 0.5f) / h;
      sF32 n1[4] = {0.5f, 0.5f, 1.0f, 1.0f};
      sF32 n2[4] = {0.5f, 0.5f, 1.0f, 1.0f};
      if (fg && fg->Data)
        sampleRGBA(*fg, su, sv, n1);
      if (bg && bg->Data)
        sampleRGBA(*bg, su, sv, n2);
      sF32 a = clamp01(amount);
      if (mask && mask->Data) {
        sF32 m[4];
        sampleRGBA(*mask, su, sv, m);
        a *= clamp01((m[0] + m[1] + m[2]) / 3.0f);
      }
      // whiteout RNM (normal_blend.mmg minus its Default-format z
      // inversion): flat foreground (t = 0,0,2) is an exact identity
      const sF32 tx = 2.0f * n1[0] - 1.0f, ty = 2.0f * n1[1] - 1.0f,
                 tz = 2.0f * n1[2];
      const sF32 ux = 1.0f - 2.0f * n2[0], uy = 1.0f - 2.0f * n2[1],
                 uz = 2.0f * n2[2] - 1.0f;
      const sF32 dot = tx * ux + ty * uy + tz * uz;
      const sF32 div = (tz > -1e-6f && tz < 1e-6f) ? 1e-6f : tz;
      sF32 r[3] = {tx * dot / div - ux, ty * dot / div - uy,
                   tz * dot / div - uz};
      const sF32 b[3] = {2.0f * n2[0] - 1.0f, 2.0f * n2[1] - 1.0f,
                         2.0f * n2[2] - 1.0f};
      for (sInt i = 0; i < 3; i++)
        r[i] = b[i] + (r[i] - b[i]) * a;
      Pixel &px = out.Data[(size_t)y * w + x];
      px.r = to16(clamp01(r[0] * 0.5f + 0.5f));
      px.g = to16(clamp01(r[1] * 0.5f + 0.5f));
      px.b = to16(clamp01(r[2] * 0.5f + 0.5f));
      px.a = 65535;
    }
}
