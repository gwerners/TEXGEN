// mm_filters — image filter ports. See mm_filters.h for attribution.
#include "mm_filters.h"

#include <cmath>

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
                 sF32 rotDeg, sF32 scaleX, sF32 scaleY, bool repeat) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sF32 rot = rotDeg * 0.01745329251f;
  const sF32 c = cosf(rot), s = sinf(rot);
  const sF32 sx = fabsf(scaleX) > 1e-6f ? scaleX : 1e-6f;
  const sF32 sy = fabsf(scaleY) > 1e-6f ? scaleY : 1e-6f;
  for (sInt py = 0; py < h; py++) {
    for (sInt px = 0; px < w; px++) {
      sF32 u = (px + 0.5f) / w - tx - 0.5f;
      sF32 v = (py + 0.5f) / h - ty - 0.5f;
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
