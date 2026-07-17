// mm_fill — fill-family ports. See mm_fill.h.
#include "mm_fill.h"
#include "mm_generators.h"

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
inline sF32 glslFract(sF32 v) {
  return v - floorf(v);
}
inline sF32 glslMod(sF32 x, sF32 y) {
  return x - y * floorf(x / y);
}
// MM common.glsl rand
inline sF32 mmRand(sF32 x, sF32 y) {
  return glslFract(cosf(glslMod(x * 13.9898f + y * 8.141f, 3.14f)) * 43758.5f);
}
inline void mmRand3v(sF32 x, sF32 y, sF32 o[3]) {
  o[0] = glslFract(cosf(glslMod(x * 13.9898f + y * 8.141f, 3.14f)) * 43758.5f);
  o[1] = glslFract(cosf(glslMod(x * 3.4562f + y * 17.398f, 3.14f)) * 43758.5f);
  o[2] = glslFract(cosf(glslMod(x * 13.254f + y * 5.867f, 3.14f)) * 43758.5f);
}

inline sInt wrapi(sInt a, sInt n) {
  a %= n;
  return a < 0 ? a + n : a;
}

// union-find
struct DSU {
  std::vector<sInt> parent;
  explicit DSU(sInt n) : parent(n) {
    for (sInt i = 0; i < n; i++)
      parent[i] = i;
  }
  sInt find(sInt a) {
    while (parent[a] != a) {
      parent[a] = parent[parent[a]];
      a = parent[a];
    }
    return a;
  }
  void unite(sInt a, sInt b) {
    a = find(a);
    b = find(b);
    if (a != b)
      parent[b] = a;
  }
};

// Fill map sample (nearest, wrap) in [0,1]^4.
inline void fillAt(const GenTexture &t, sF32 u, sF32 v, sF32 bb[4]) {
  sInt x = wrapi((sInt)floorf(u * t.XRes), t.XRes);
  sInt y = wrapi((sInt)floorf(v * t.YRes), t.YRes);
  const Pixel &p = t.Data[y * t.XRes + x];
  const sF32 s = 1.0f / 65535.0f;
  bb[0] = p.r * s;
  bb[1] = p.g * s;
  bb[2] = p.b * s;
  bb[3] = p.a * s;
}

inline bool isEdge(const sF32 bb[4]) {
  return bb[2] + bb[3] < 0.0000001f;
}

// per-region random value used by fill_to_random_*:
// rand(vec2(seed, rand(vec2(rand(bb.xy), rand(bb.zw)))))
inline sF32 regionRandKey(const sF32 bb[4]) {
  return mmRand(mmRand(bb[0], bb[1]), mmRand(bb[2], bb[3]));
}

// Extent of a component along one wrapped axis: given the occupancy
// bitmap, returns {start, size} in texels (torus-aware: the box starts
// after the largest empty gap).
void wrappedExtent(const std::vector<char> &occupied, sInt n, sInt &start,
                   sInt &size) {
  // largest run of empty cells (circular)
  sInt bestLen = -1, bestEnd = 0;
  sInt runLen = 0;
  for (sInt i = 0; i < 2 * n; i++) {
    if (!occupied[i % n]) {
      runLen++;
      if (runLen >= n)
        break; // fully empty cannot happen (component non-empty)
      if (runLen > bestLen) {
        bestLen = runLen;
        bestEnd = i % n;
      }
    } else {
      runLen = 0;
    }
  }
  if (bestLen <= 0) {
    start = 0;
    size = n;
    return;
  }
  start = (bestEnd + 1) % n;
  size = n - bestLen;
}

} // namespace

void MMFill(GenTexture &out, const GenTexture &in) {
  if (!out.Data || !in.Data)
    return;
  const sInt w = in.XRes, h = in.YRes;
  const sInt n = w * h;

  // region mask: dark pixels (luminance <= 0.5) form regions
  std::vector<char> region(n);
  for (sInt i = 0; i < n; i++) {
    const Pixel &p = in.Data[i];
    const sF32 lum = ((sF32)p.r + (sF32)p.g + (sF32)p.b) / (3.0f * 65535.0f);
    region[i] = lum <= 0.5f ? 1 : 0;
  }

  // connected components, 4-neighborhood with wrap
  DSU dsu(n);
  for (sInt y = 0; y < h; y++) {
    for (sInt x = 0; x < w; x++) {
      const sInt i = y * w + x;
      if (!region[i])
        continue;
      const sInt right = y * w + (x + 1) % w;
      const sInt down = ((y + 1) % h) * w + x;
      if (region[right])
        dsu.unite(i, right);
      if (region[down])
        dsu.unite(i, down);
    }
  }

  // occupancy per component (columns and rows) for torus bboxes
  std::vector<sInt> compOf(n, -1);
  std::vector<sInt> roots;
  for (sInt i = 0; i < n; i++) {
    if (!region[i])
      continue;
    sInt r = dsu.find(i);
    if (compOf[r] < 0) {
      compOf[r] = (sInt)roots.size();
      roots.push_back(r);
    }
  }
  const sInt nComp = (sInt)roots.size();
  std::vector<std::vector<char>> cols(nComp, std::vector<char>(w, 0));
  std::vector<std::vector<char>> rows(nComp, std::vector<char>(h, 0));
  for (sInt y = 0; y < h; y++) {
    for (sInt x = 0; x < w; x++) {
      const sInt i = y * w + x;
      if (!region[i])
        continue;
      const sInt c = compOf[dsu.find(i)];
      cols[c][x] = 1;
      rows[c][y] = 1;
    }
  }
  std::vector<sF32> bbx(nComp), bby(nComp), bbw(nComp), bbh(nComp);
  for (sInt c = 0; c < nComp; c++) {
    sInt sx, szx, sy, szy;
    wrappedExtent(cols[c], w, sx, szx);
    wrappedExtent(rows[c], h, sy, szy);
    bbx[c] = (sF32)sx / w;
    bby[c] = (sF32)sy / h;
    bbw[c] = (sF32)szx / w;
    bbh[c] = (sF32)szy / h;
  }

  // write the fill map (out matches the input size 1:1)
  for (sInt i = 0; i < n; i++) {
    Pixel &p = out.Data[i];
    if (!region[i]) {
      p.r = p.g = p.b = p.a = 0;
      continue;
    }
    const sInt c = compOf[dsu.find(i)];
    p.r = to16(bbx[c]);
    p.g = to16(bby[c]);
    p.b = to16(bbw[c]);
    p.a = to16(bbh[c]);
  }
}

void MMFillToUV(GenTexture &out, const GenTexture &fill, sInt mode,
                sF32 seed) {
  if (!out.Data || !fill.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 bb[4];
      fillAt(fill, u, v, bb);
      sF32 ru = 0.0f, rv = 0.0f, rz = 0.0f;
      if (!isEdge(bb)) {
        rz = mmRand(seed + bb[0] + bb[2], seed + bb[1] + bb[3]);
        if (mode == 1) { // square: pad the smaller axis, divide by max
          if (bb[2] > bb[3]) {
            ru = glslFract(u - bb[0]) / bb[2];
            rv = glslFract(v + (bb[2] - bb[3]) * 0.5f - bb[1]) / bb[2];
          } else {
            ru = glslFract(u + (bb[3] - bb[2]) * 0.5f - bb[0]) / bb[3];
            rv = glslFract(v - bb[1]) / bb[3];
          }
        } else { // stretch
          ru = bb[2] > 0.0f ? glslFract(u - bb[0]) / bb[2] : 0.0f;
          rv = bb[3] > 0.0f ? glslFract(v - bb[1]) / bb[3] : 0.0f;
        }
      }
      Pixel &p = out.Data[py * w + px];
      p.r = to16(ru);
      p.g = to16(rv);
      p.b = to16(rz);
      p.a = 65535;
    }
  }
}

void MMFillToRandomGray(GenTexture &out, const GenTexture &fill,
                        sF32 edgecolor, sF32 seed) {
  if (!out.Data || !fill.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 bb[4];
      fillAt(fill, u, v, bb);
      const sF32 g =
          isEdge(bb) ? edgecolor : mmRand(seed, regionRandKey(bb));
      Pixel &p = out.Data[py * w + px];
      const sU16 g16 = to16(g);
      p.r = p.g = p.b = g16;
      p.a = 65535;
    }
  }
}

void MMFillToRandomColor(GenTexture &out, const GenTexture &fill,
                         sF32 edgeR, sF32 edgeG, sF32 edgeB, sF32 seed) {
  if (!out.Data || !fill.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 bb[4];
      fillAt(fill, u, v, bb);
      sF32 rgb[3] = {edgeR, edgeG, edgeB};
      if (!isEdge(bb))
        mmRand3v(seed, regionRandKey(bb), rgb);
      Pixel &p = out.Data[py * w + px];
      p.r = to16(rgb[0]);
      p.g = to16(rgb[1]);
      p.b = to16(rgb[2]);
      p.a = 65535;
    }
  }
}

void MMFillToColor(GenTexture &out, const GenTexture &fill,
                   const GenTexture *map, sF32 edgeR, sF32 edgeG,
                   sF32 edgeB, sF32 edgeA) {
  if (!out.Data || !fill.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 bb[4];
      fillAt(fill, u, v, bb);
      sF32 rgba[4] = {edgeR, edgeG, edgeB, edgeA};
      if (!isEdge(bb)) {
        if (map && map->Data) {
          const sF32 cu = glslFract(bb[0] + 0.5f * bb[2]);
          const sF32 cv = glslFract(bb[1] + 0.5f * bb[3]);
          sInt x = wrapi((sInt)floorf(cu * map->XRes), map->XRes);
          sInt y = wrapi((sInt)floorf(cv * map->YRes), map->YRes);
          const Pixel &mp = map->Data[y * map->XRes + x];
          const sF32 s = 1.0f / 65535.0f;
          rgba[0] = mp.r * s;
          rgba[1] = mp.g * s;
          rgba[2] = mp.b * s;
          rgba[3] = mp.a * s;
        } else {
          rgba[0] = rgba[1] = rgba[2] = 1.0f;
          rgba[3] = 1.0f;
        }
      }
      Pixel &p = out.Data[py * w + px];
      p.r = to16(rgba[0]);
      p.g = to16(rgba[1]);
      p.b = to16(rgba[2]);
      p.a = to16(rgba[3]);
    }
  }
}

namespace {
// vec2 rand2(vec2 x) — same constants as MM's rand2
inline void mmRand2v(sF32 x, sF32 y, sF32 o[2]) {
  o[0] =
      glslFract(cosf(glslMod(x * 13.9898f + y * 8.141f, 3.14f)) * 43758.5f);
  o[1] =
      glslFract(cosf(glslMod(x * 3.4562f + y * 17.398f, 3.14f)) * 43758.5f);
}

// local region UVs + per-region random key (fill_to_uv_* in MM)
inline void fillLocalUV(const sF32 bb[4], sF32 u, sF32 v, sInt mode,
                        sF32 seed, sF32 o[3]) {
  o[0] = o[1] = o[2] = 0.0f;
  if (isEdge(bb))
    return;
  o[2] = mmRand(seed + bb[0] + bb[2], seed + bb[1] + bb[3]);
  if (mode == 1) { // square: pad the smaller axis, divide by max
    if (bb[2] > bb[3]) {
      o[0] = glslFract(u - bb[0]) / bb[2];
      o[1] = glslFract(v + (bb[2] - bb[3]) * 0.5f - bb[1]) / bb[2];
    } else {
      o[0] = glslFract(u + (bb[3] - bb[2]) * 0.5f - bb[0]) / bb[3];
      o[1] = glslFract(v - bb[1]) / bb[3];
    }
  } else { // stretch
    o[0] = bb[2] > 0.0f ? glslFract(u - bb[0]) / bb[2] : 0.0f;
    o[1] = bb[3] > 0.0f ? glslFract(v - bb[1]) / bb[3] : 0.0f;
  }
}
} // namespace

void MMFillToGradient(GenTexture &out, const GenTexture &fill,
                      const MMGradientStop *stops, sInt nStops, sInt mode,
                      sInt layers, sF32 rotate, sF32 rndRotate,
                      sF32 rndOffset, sF32 seed) {
  if (!out.Data || !fill.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  const sInt nl = layers < 1 ? 1 : layers;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 bb[4], cuv[3];
      fillAt(fill, u, v, bb);
      fillLocalUV(bb, u, v, mode, seed, cuv);
      sF32 value[4] = {1.0f, 1.0f, 1.0f, 1.0f};
      for (sInt i = 0; i < nl; i++) {
        // gradient_uv(): per-region random rotation and offset
        sF32 s2[2];
        mmRand2v(cuv[2], seed + (sF32)i, s2);
        sF32 gu = cuv[0] - 0.5f, gv = cuv[1] - 0.5f;
        const sF32 angle =
            (rotate + (s2[0] * 2.0f - 1.0f) * rndRotate) * 0.01745329251f;
        const sF32 ca = cosf(angle), sa = sinf(angle);
        sF32 t = ca * gu + sa * gv;
        t += s2[1] * rndOffset;
        t = clamp01(t / 1.41421356237f + 0.5f);
        sF32 c[4];
        MMGradientEval(stops, nStops, t, c);
        for (sInt k = 0; k < 4; k++)
          value[k] = c[k] < value[k] ? c[k] : value[k];
      }
      Pixel &p = out.Data[(size_t)py * w + px];
      p.r = to16(value[0]);
      p.g = to16(value[1]);
      p.b = to16(value[2]);
      p.a = to16(value[3]);
    }
  }
}

void MMFillToSize(GenTexture &out, const GenTexture &fill, sInt formula) {
  if (!out.Data || !fill.Data)
    return;
  const sInt w = out.XRes, h = out.YRes;
  for (sInt py = 0; py < h; py++) {
    const sF32 v = (py + 0.5f) / h;
    for (sInt px = 0; px < w; px++) {
      const sF32 u = (px + 0.5f) / w;
      sF32 bb[4];
      fillAt(fill, u, v, bb);
      sF32 s;
      switch (formula) {
      case 1: s = bb[2]; break;
      case 2: s = bb[3]; break;
      case 3: s = bb[2] > bb[3] ? bb[2] : bb[3]; break;
      default: s = sqrtf(bb[2] * bb[3]); break;
      }
      Pixel &p = out.Data[(size_t)py * w + px];
      p.r = p.g = p.b = to16(s);
      p.a = 65535;
    }
  }
}
