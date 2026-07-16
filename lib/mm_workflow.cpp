// mm_workflow — layered-material workflow ports. See mm_workflow.h.
#include "mm_workflow.h"

#include "mm_filters.h"

#include <cmath>
#include <vector>

namespace {

inline sF32 clamp01(sF32 v) {
  return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
inline sF32 mixf(sF32 a, sF32 b, sF32 t) {
  return a + (b - a) * t;
}
inline sU16 to16(sF32 v) {
  return (sU16)(clamp01(v) * 65535.0f + 0.5f);
}

inline sInt wrapi(sInt a, sInt n) {
  a %= n;
  return a < 0 ? a + n : a;
}

// Bilinear wrap RGBA sample in [0,1] (same convention as mm_filters).
void sampleRGBA(const GenTexture &t, sF32 u, sF32 v, sF32 out[4]) {
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

// Sample a bundle channel with its unconnected-input default.
// channel: 0=H, 1=C, 2=ORM, 3=EM, 4=NM
void sampleChannel(const GenTexture *t, int channel, sF32 u, sF32 v,
                   sF32 out[4]) {
  if (t && t->Data) {
    sampleRGBA(*t, u, v, out);
    return;
  }
  if (channel == 4) { // flat normal
    out[0] = 0.5f;
    out[1] = 0.5f;
    out[2] = 1.0f;
    out[3] = 1.0f;
  } else { // H/C/ORM/EM default to black
    out[0] = out[1] = out[2] = 0.0f;
    out[3] = channel == 0 ? 0.0f : 1.0f;
  }
}

inline sF32 grayOf(const sF32 c[4]) {
  return (c[0] + c[1] + c[2]) / 3.0f;
}

inline void writePixel(GenTexture &t, sInt idx, const sF32 c[4]) {
  t.Data[idx].r = to16(c[0]);
  t.Data[idx].g = to16(c[1]);
  t.Data[idx].b = to16(c[2]);
  t.Data[idx].a = to16(c[3]);
}

inline void writeGray(GenTexture &t, sInt idx, sF32 g) {
  sF32 c[4] = {g, g, g, 1.0f};
  writePixel(t, idx, c);
}

// Separable box blur of the grayscale of t (values in [0,1]).
std::vector<sF32> boxBlurGray(const GenTexture &t, sInt radius) {
  const sInt w = t.XRes, h = t.YRes;
  std::vector<sF32> gray((size_t)w * h), tmp((size_t)w * h),
      out((size_t)w * h);
  const sF32 s = 1.0f / (3.0f * 65535.0f);
  for (sInt i = 0; i < w * h; i++) {
    const Pixel &p = t.Data[i];
    gray[i] = ((sF32)p.r + (sF32)p.g + (sF32)p.b) * s;
  }
  const sF32 norm = 1.0f / (2 * radius + 1);
  for (sInt y = 0; y < h; y++) {
    for (sInt x = 0; x < w; x++) {
      sF32 acc = 0.0f;
      for (sInt k = -radius; k <= radius; k++)
        acc += gray[y * w + wrapi(x + k, w)];
      tmp[y * w + x] = acc * norm;
    }
  }
  for (sInt y = 0; y < h; y++) {
    for (sInt x = 0; x < w; x++) {
      sF32 acc = 0.0f;
      for (sInt k = -radius; k <= radius; k++)
        acc += tmp[wrapi(y + k, h) * w + x];
      out[y * w + x] = acc * norm;
    }
  }
  return out;
}

} // namespace

void MMLayerMix(GenTexture *outs[5], const GenTexture *l1[5],
                const GenTexture *l2[5], sInt mode, sF32 width) {
  if (!outs[0] || !outs[0]->Data)
    return;
  const sInt w = outs[0]->XRes, h = outs[0]->YRes;
  if (width < 1e-4f)
    width = 1e-4f;

  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const sInt idx = py * w + px;

      sF32 c1[4], c2[4];
      sampleChannel(l1[0], 0, u, v, c1);
      sampleChannel(l2[0], 0, u, v, c2);
      const sF32 h1 = grayOf(c1), h2 = grayOf(c2);

      sF32 a;
      if (mode == 0) {
        a = h2 > h1 ? 1.0f : 0.0f;
      } else {
        sF32 t = clamp01((h2 - h1) / width);
        a = t * t * (3.0f - 2.0f * t);
      }

      writeGray(*outs[0], idx, h1 > h2 ? h1 : h2);
      for (int ch = 1; ch < 5; ch++) {
        if (!outs[ch] || !outs[ch]->Data)
          continue;
        sF32 x1[4], x2[4], o[4];
        sampleChannel(ch < 5 ? l1[ch] : nullptr, ch, u, v, x1);
        sampleChannel(ch < 5 ? l2[ch] : nullptr, ch, u, v, x2);
        for (int i = 0; i < 4; i++)
          o[i] = mixf(x1[i], x2[i], a);
        writePixel(*outs[ch], idx, o);
      }
    }
  }
}

void MMWorkflowOutput(GenTexture &albedo, GenTexture &metallic,
                      GenTexture &roughness, GenTexture &emission,
                      GenTexture &normal, GenTexture &occlusion,
                      GenTexture &depth, const GenTexture *height,
                      const GenTexture *albedoIn, const GenTexture *orm,
                      const GenTexture *emissionIn,
                      const GenTexture *normalIn, sF32 matNormal,
                      sF32 occStrength) {
  if (!albedo.Data)
    return;
  const sInt w = albedo.XRes, h = albedo.YRes;

  // Height-derived normal (MM template: normal_map with default strength)
  GenTexture heightNormal;
  if (height && height->Data) {
    heightNormal.Init(w, h);
    MMNormalMap(heightNormal, *height, 1.0f, 1 /*OpenGL*/);
  }

  // Blur-based AO from the heightmap
  std::vector<sF32> blurred;
  if (height && height->Data)
    blurred = boxBlurGray(*height, height->XRes > 32 ? height->XRes / 32 : 1);

  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      const sInt idx = py * w + px;

      sF32 c[4];
      sampleChannel(albedoIn, 1, u, v, c);
      writePixel(albedo, idx, c);

      sampleChannel(orm, 2, u, v, c);
      writeGray(roughness, idx, c[1]);
      writeGray(metallic, idx, c[2]);

      sampleChannel(emissionIn, 3, u, v, c);
      writePixel(emission, idx, c);

      // Normal: height normal + bundle normal xy offsets scaled by
      // matNormal, renormalized
      sF32 nh[4], nm[4];
      sampleChannel(heightNormal.Data ? &heightNormal : nullptr, 4, u, v, nh);
      sampleChannel(normalIn, 4, u, v, nm);
      sF32 nx = (nh[0] * 2.0f - 1.0f) + (nm[0] * 2.0f - 1.0f) * matNormal;
      sF32 ny = (nh[1] * 2.0f - 1.0f) + (nm[1] * 2.0f - 1.0f) * matNormal;
      sF32 nz = nh[2] * 2.0f - 1.0f;
      if (nz < 0.1f)
        nz = 0.1f;
      sF32 len = sqrtf(nx * nx + ny * ny + nz * nz);
      sF32 n[4] = {nx / len * 0.5f + 0.5f, ny / len * 0.5f + 0.5f,
                   nz / len * 0.5f + 0.5f, 1.0f};
      writePixel(normal, idx, n);

      sF32 hg = 0.0f;
      if (height && height->Data) {
        sF32 hc[4];
        sampleRGBA(*height, u, v, hc);
        hg = grayOf(hc);
      }
      sF32 occ = 1.0f;
      if (!blurred.empty()) {
        // sample blur at nearest source texel
        sInt sx = wrapi((sInt)(u * height->XRes), height->XRes);
        sInt sy = wrapi((sInt)(v * height->YRes), height->YRes);
        sF32 d = blurred[sy * height->XRes + sx] - hg;
        occ = clamp01(1.0f - occStrength * (d > 0.0f ? d : 0.0f) * 4.0f);
      }
      writeGray(occlusion, idx, occ);
      writeGray(depth, idx, 1.0f - hg);
    }
  }
}
