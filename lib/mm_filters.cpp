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
