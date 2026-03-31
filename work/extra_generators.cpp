/*
 * extra_generators.cpp
 *
 * Implementations ported from fr_public (RG2 and werkkzeug3).
 * Original code is in the public domain.
 *
 * Porting notes:
 *  - Pixel format: fr_public uses BGRA channel order (index 0=B, 2=R).
 *    GenTexture uses RGBA (Pixel.r / index 0=R, index 2=B). Channels are
 *    swapped where necessary.
 *  - fr_public uses 15-bit pixel values (0..32767); GenTexture uses 16-bit
 *    (0..65535). Crystal and PerlinNoiseRG2 output is kept near-16-bit via
 *    adjusted color scaling. HSCB and ColorBalance operate in 16-bit
 *    internally with adapted tables.
 *  - x86 inline assembly (MMX / imul) replaced with portable C++ equivalents.
 */

#include "extra_generators.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// Internal helpers
// ============================================================

static inline sInt clamp16(sInt x) {
  return x < 0 ? 0 : (x > 65535 ? 65535 : x);
}

static inline sInt clamp15(sInt x) {
  return x < 0 ? 0 : (x > 0x7fff ? 0x7fff : x);
}

// Fixed-point 16.16 multiply: result = (a * b) >> 16
static inline sS32 FMul16(sS32 a, sS32 b) {
  return (sS32)(((sS64)a * (sS64)b) >> 16);
}

// Fixed-point division: result = (a << 16) / b
static inline sS32 FDiv16(sS32 a, sS32 b) {
  return (sS32)(((sS64)a << 16) / b);
}

// ============================================================
// Crystal (fr_public/RG2/texgenerate.cpp  frTGCrystal)
// ============================================================

void Crystal(GenTexture& dst,
             sU32 seed,
             sInt count,
             sU32 colorNear,
             sU32 colorFar) {
  if (count < 1)
    count = 1;
  if (count > 256)
    count = 256;

  // cells[i] = {x, y, dist_sq}
  sInt cells[256][3];

  const sInt width = dst.XRes;
  const sInt height = dst.YRes;
  sU16* dptr = (sU16*)dst.Data;

  // Scale factors: map texture pixels to a [0..1023] coordinate space
  sInt stepx = 1, stepy = 1;
  while (width * stepx < 1024)
    stepx *= 2;
  while (height * stepy < 1024)
    stepy *= 2;

  // Randomize cell positions with a simple LCG (same as original)
  seed = seed * 0x8af6f2ac + 0x1757286;
  sInt* flat = cells[0];  // treat cells as a flat count*3 array
  for (sInt i = 0; i < count * 3; i++) {
    flat[i] = (seed >> 10) & 1023;
    seed = (seed * 0x15a4e35) + 1;
  }

  sU32 maxdist = 0;

  // Pass 1: store squared-distance to nearest cell center per pixel
  for (sInt y = 0; y < 1024; y += stepy) {
    // Insertion-sort cells by squared Y-distance to this scanline
    for (sInt i = 0; i < count; i++) {
      sInt px = cells[i][0];
      sInt py = cells[i][1];
      sInt dy = ((py - y) & 1023) - 512;
      sInt dist = dy * dy;

      sInt j = i;
      while (j > 0 && cells[j - 1][2] > dist) {
        cells[j][0] = cells[j - 1][0];
        cells[j][1] = cells[j - 1][1];
        cells[j][2] = cells[j - 1][2];
        j--;
      }
      cells[j][0] = px;
      cells[j][1] = py;
      cells[j][2] = dist;
    }

    for (sInt x = 0; x < 1024; x += stepx) {
      sInt best = 2048 * 2048;
      for (sInt i = 0; i < count && cells[i][2] < best; i++) {
        sInt dx = ((cells[i][0] - x) & 1023) - 512;
        sInt dist = dx * dx + cells[i][2];
        if (dist < best)
          best = dist;
      }
      if ((sU32)best > maxdist)
        maxdist = best;

      // Store raw distance as 32-bit value in first two sU16 slots
      *((sU32*)dptr) = (sU32)best;
      dptr += 4;
    }
  }

  // Pass 2: map distances to colors via 2-color lerp
  // Colors: extract 8-bit components from 0xaarrggbb
  sInt rN = (colorNear >> 16) & 0xFF;
  sInt gN = (colorNear >> 8) & 0xFF;
  sInt bN = colorNear & 0xFF;
  sInt rF = (colorFar >> 16) & 0xFF;
  sInt gF = (colorFar >> 8) & 0xFF;
  sInt bF = colorFar & 0xFF;

  // Scale start colors to near-16-bit (<<8 maps 0..255 to 0..65280)
  sInt sr = rN << 8, dr = rF - rN;
  sInt sg = gN << 8, dg = gF - gN;
  sInt sb = bN << 8, db = bF - bN;

  // rescale: normalize distance to [0..32768]
  sU32 rescale = (maxdist > 1) ? ((1U << 31) / maxdist) : ((1U << 31) - 1);

  dptr = (sU16*)dst.Data;
  for (sInt s = 0; s < width * height; s++) {
    sU32 rawdist = *((sU32*)dptr);
    // fade in [0..32768]
    sS32 fade = FMul16((sS32)rawdist, (sS32)rescale);

    // GenTexture RGBA order: [0]=r, [1]=g, [2]=b, [3]=a
    dptr[0] = (sU16)clamp16(sr + ((dr * fade) >> 7));
    dptr[1] = (sU16)clamp16(sg + ((dg * fade) >> 7));
    dptr[2] = (sU16)clamp16(sb + ((db * fade) >> 7));
    dptr[3] = 0xFFFF;
    dptr += 4;
  }
}

// ============================================================
// DirectionalGradient  (fr_public/RG2/texgenerate.cpp  frTGGradient, linear)
// ============================================================

void DirectionalGradient(GenTexture& dst,
                         sF32 x1,
                         sF32 y1,
                         sF32 x2,
                         sF32 y2,
                         sU32 col1,
                         sU32 col2) {
  sF32 xd = x2 - x1, yd = y2 - y1;
  sF32 sc = (xd * xd + yd * yd);
  if (sc < 1e-10f)
    sc = 1e-10f;
  sc = 1.0f / sc;
  xd *= sc;
  yd *= sc;

  sS32 r1 = (col1 >> 16) & 0xFF, g1 = (col1 >> 8) & 0xFF, b1 = col1 & 0xFF;
  sS32 r2 = (col2 >> 16) & 0xFF, g2 = (col2 >> 8) & 0xFF, b2 = col2 & 0xFF;
  sS32 dr = r2 - r1, dg = g2 - g1, db = b2 - b1;

  const sF32 iw = 1.0f / dst.XRes;
  const sF32 ih = 1.0f / dst.YRes;

  sU16* ptr = (sU16*)dst.Data;
  for (sInt y = 0; y < dst.YRes; y++) {
    sF32 p = y * ih - y1;
    for (sInt x = 0; x < dst.XRes; x++) {
      sF32 n = x * iw - x1;
      sF32 f = n * xd + p * yd;
      if (f < 0.0f)
        f = 0.0f;
      if (f > 1.0f)
        f = 1.0f;
      sS32 fd = (sS32)(f * 65536.0f);

      // GenTexture RGBA: [0]=r, [1]=g, [2]=b
      ptr[0] = (sU16)(((r1 << 16) + dr * fd) >> 8);
      ptr[1] = (sU16)(((g1 << 16) + dg * fd) >> 8);
      ptr[2] = (sU16)(((b1 << 16) + db * fd) >> 8);
      ptr[3] = 0xFFFF;
      ptr += 4;
    }
  }
}

// ============================================================
// GlowEffect  (fr_public/RG2/texgenerate.cpp  frTGGradient, glow mode)
// ============================================================

void GlowEffect(GenTexture& dst,
                sF32 cx,
                sF32 cy,
                sF32 scale,
                sF32 exponent,
                sF32 intensity,
                sU32 bgCol,
                sU32 glowCol) {
  sF32 sc = (2.0f / scale) * (2.0f / scale);

  sS32 r1 = (bgCol >> 16) & 0xFF, g1 = (bgCol >> 8) & 0xFF, b1 = bgCol & 0xFF;
  sS32 r2 = (glowCol >> 16) & 0xFF, g2 = (glowCol >> 8) & 0xFF,
       b2 = glowCol & 0xFF;
  sS32 dr = r2 - r1, dg = g2 - g1, db = b2 - b1;

  const sF32 iw = 1.0f / dst.XRes;
  const sF32 ih = 1.0f / dst.YRes;

  sU16* ptr = (sU16*)dst.Data;
  for (sInt y = 0; y < dst.YRes; y++) {
    sF32 p = y * ih - cy;
    sF32 pp = p * p;
    for (sInt x = 0; x < dst.XRes; x++) {
      sF32 n = x * iw - cx;
      sF32 f = 1.0f - sqrtf((n * n + pp) * sc);
      if (f < 0.0f)
        f = 0.0f;
      if (f > 1.0f)
        f = 1.0f;
      f = powf(f, exponent) * intensity;
      if (f < 0.0f)
        f = 0.0f;
      if (f > 1.0f)
        f = 1.0f;
      sS32 fd = (sS32)(f * 65536.0f);

      ptr[0] = (sU16)(((r1 << 16) + dr * fd) >> 8);
      ptr[1] = (sU16)(((g1 << 16) + dg * fd) >> 8);
      ptr[2] = (sU16)(((b1 << 16) + db * fd) >> 8);
      ptr[3] = 0xFFFF;
      ptr += 4;
    }
  }
}

// ============================================================
// PerlinNoiseRG2  (fr_public/RG2/texgenerate.cpp  frTGPerlinNoise)
// x86 assembly (srandom2, imul16) replaced with portable C++.
// ============================================================

// Smooth-step weight table (1024 entries), equivalent to 6t^5-15t^4+10t^3
static sU16 g_wtab[512];
static bool g_wtab_ready = false;

static void ensure_wtab() {
  if (g_wtab_ready)
    return;
  for (sInt i = 0; i < 512; i++) {
    sF32 val = i / 512.0f;
    g_wtab[i] = (sU16)(1024.0f * val * val * val *
                       (10.0f + val * (val * 6.0f - 15.0f)));
  }
  g_wtab_ready = true;
}

// Portable replacement for the x86 asm srandom2 (same bit pattern)
static inline sU32 srandom2(sU32 n) {
  n ^= (n << 13);
  sU32 t = n * n * 15731u + 789221u;
  n = n * t + 1376312589u;
  n &= 0x7fffffffu;
  return 2048u - (n >> 19);
}

// Portable replacement for x86 asm imul16: (a*b)>>16
static inline sS32 imul16(sS32 a, sS32 b) {
  return (sS32)(((sS64)a * b) >> 16);
}

static sS32 inoise(sU32 x, sU32 y, sInt msk, sU32 seed_val) {
  sInt ix = (sInt)(x >> 10), iy = (sInt)(y >> 10);
  sInt inx = (ix + 1) & msk, iny = (iy + 1) & msk;

  ix += (sInt)seed_val;
  inx += (sInt)seed_val;
  iy *= 31337;
  iny *= 31337;

  sU16 fx = g_wtab[(x >> 1) & 511];

  sS32 t1 = (sS32)srandom2((sU32)(ix + iy));
  t1 = (t1 << 10) + ((sS32)srandom2((sU32)(inx + iy)) - t1) * (sS32)fx;
  sS32 t2 = (sS32)srandom2((sU32)(ix + iny));
  t2 = (t2 << 10) + ((sS32)srandom2((sU32)(inx + iny)) - t2) * (sS32)fx;

  return ((t1 << 10) + (t2 - t1) * (sS32)g_wtab[(y >> 1) & 511]) >> 20;
}

void PerlinNoiseRG2(GenTexture& dst,
                    sInt octaves,
                    sF32 persistence,
                    sInt freqScale,
                    sU32 seed,
                    sF32 contrast,
                    sU32 col1,
                    sU32 col2,
                    sInt startOctave) {
  ensure_wtab();

  if (startOctave >= octaves)
    startOctave = 0;

  sInt frq = 1 << freqScale;
  sS32 prs = (sS32)(persistence * 65536.0f);

  sInt fx = 1024 * frq / dst.XRes;
  sInt fy = 1024 * frq / dst.YRes;

  // Compute normalization sum starting from startOctave
  sS32 ampSum = 0, samp = 0;
  sS32 amp = 32768;
  for (sInt o = 0; o < octaves; o++) {
    if (o == startOctave) {
      samp = amp;
      ampSum += amp;
    } else if (o > startOctave)
      ampSum += amp;
    amp = (amp * prs) >> 16;
  }
  if (ampSum == 0)
    ampSum = 1;

  sS32 sc = (sS32)(contrast * 65536.0f * 16.0f / (sF32)ampSum);

  sS32 r1 = (sS32)((col1 >> 16) & 0xFF) << 8;
  sS32 g1 = (sS32)((col1 >> 8) & 0xFF) << 8;
  sS32 b1 = (sS32)(col1 & 0xFF) << 8;
  sS32 r2 = ((sS32)((col2 >> 16) & 0xFF) << 8) - r1;
  sS32 g2 = ((sS32)((col2 >> 8) & 0xFF) << 8) - g1;
  sS32 b2 = ((sS32)(col2 & 0xFF) << 8) - b1;

  sInt fxBase = fx << startOctave;
  sInt fyBase = fy << startOctave;
  sInt fbBase = frq << startOctave;

  const sU32 seedBase = (seed + 1) * 0xb37fa184u + 303u * 31337u;

  sU16* out = (sU16*)dst.Data;
  for (sInt y = 0; y < dst.YRes; y++) {
    for (sInt x = 0; x < dst.XRes; x++) {
      amp = samp;
      sS32 sum = 0;
      sU32 xv = (sU32)(x * fxBase);
      sU32 yv = (sU32)(y * fyBase);
      sInt f = fbBase;
      sU32 curseed = seedBase;

      for (sInt o = startOctave; o < octaves; o++) {
        sum += amp * inoise(xv, yv, f - 1, curseed);
        xv *= 2;
        yv *= 2;
        f <<= 1;
        curseed += 0xdeadbeefu;
        amp = (amp * prs) >> 16;
      }

      sS32 sm = imul16(sum, sc) + 32768;
      if (sm < 0)
        sm = 0;
      if (sm > 65536)
        sm = 65536;

      out[0] = (sU16)(r1 + ((sm * r2) >> 16));
      out[1] = (sU16)(g1 + ((sm * g2) >> 16));
      out[2] = (sU16)(b1 + ((sm * b2) >> 16));
      out[3] = 0xFFFF;
      out += 4;
    }
  }
}

// ============================================================
// BlurKernel  (fr_public/RG2/texfilter.cpp)
// Kernel calculation ported; blur loop written in portable C++.
// ============================================================

static void buildKernel(sF32 radius,
                        sInt type,
                        int*& kern,
                        int& halfLen,
                        int& divisor) {
  static int buf[1027];
  int* center = buf + 512;

  if (radius <= 1.0f) {
    center[-1] = 0;
    center[0] = 512;
    center[1] = 0;
    buf[1026] = 512;
    halfLen = 1;
    kern = center;
    divisor = 512;
    return;
  }

  int len = (int)ceilf(radius);
  halfLen = len;

  if (type == 0) {  // box
    int div = 512;
    center[0] = 512;
    for (int i = 1; i < len - 1; i++)
      div += 2 * (center[-i] = center[i] = 512);
    if (len > 1) {
      if (radius != floorf(radius))
        div += 2 * (center[-(len - 1)] = center[len - 1] =
                        (int)(512 * (radius - floorf(radius))));
      else
        div += 2 * (center[-(len - 1)] = center[len - 1] = 512);
    }
    buf[1026] = div;
  } else if (type == 1) {  // triangle
    int div = 512;
    center[0] = 512;
    for (int i = 1; i <= len; i++)
      div += 2 * (center[-i] = center[i] = (int)(512.0f * (1.0f - i / radius)));
    buf[1026] = div;
  } else {  // gaussian
    float sigma = -2.0f / (radius * radius);
    int div = 512;
    center[0] = 512;
    for (int i = 1; i <= len; i++)
      div += 2 * (center[-i] = center[i] = (int)(512.0f * expf(i * i * sigma)));
    buf[1026] = div;
  }

  kern = center;
  divisor = buf[1026];
}

static void blurAxis(sU16* dst,
                     const sU16* src,
                     int w,
                     int h,
                     int axis,
                     int* kern,
                     int halfLen,
                     int divisor,
                     int wrapMode) {
  // axis 0 = horizontal, axis 1 = vertical
  int rd = axis ? h : w;     // length along blur axis
  int f = axis ? w * 4 : 4;  // stride to next pixel along axis

  long long inv = (1LL << 24) / divisor;

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int o = (y * w + x) * 4;
      int rc = axis ? y : x;

      long long acc[4] = {0, 0, 0, 0};
      for (int j = -halfLen + 1; j < halfLen; j++) {
        int rc2 = rc + j;
        int so;
        if ((unsigned)rc2 < (unsigned)rd) {
          so = j * f;
        } else {
          bool wrapU = !(wrapMode & GenTexture::ClampU);
          bool wrapV = !(wrapMode & GenTexture::ClampV);
          bool doWrap = axis ? wrapV : wrapU;
          if (doWrap) {
            rc2 = ((rc2 % rd) + rd) % rd;
            so = (rc2 - rc) * f;
          } else {
            // clamp
            rc2 = rc2 < 0 ? 0 : rd - 1;
            so = (rc2 - rc) * f;
          }
        }
        const sU16* px = src + o + so;
        int w_coeff = kern[j];
        for (int c = 0; c < 4; c++)
          acc[c] += (long long)px[c] * w_coeff;
      }
      for (int c = 0; c < 4; c++) {
        long long v = (acc[c] * inv) >> 24;
        dst[o + c] = (sU16)(v < 0 ? 0 : v > 65535 ? 65535 : v);
      }
    }
  }
}

void BlurKernel(GenTexture& dst,
                const GenTexture& src,
                sF32 radiusX,
                sF32 radiusY,
                sInt kernelType,
                sInt wrapMode) {
  int w = src.XRes, h = src.YRes;
  int* kern;
  int halfLen, divisor;

  // Temporary buffer for intermediate pass
  sU16* tmp = new sU16[w * h * 4];

  // Horizontal pass
  buildKernel(radiusX * w, kernelType, kern, halfLen, divisor);
  blurAxis(tmp, (const sU16*)src.Data, w, h, 0, kern, halfLen, divisor,
           wrapMode);

  // Vertical pass
  buildKernel(radiusY * h, kernelType, kern, halfLen, divisor);
  blurAxis((sU16*)dst.Data, tmp, w, h, 1, kern, halfLen, divisor, wrapMode);

  delete[] tmp;
}

// ============================================================
// HSCB  (fr_public/werkkzeug3/w3texlib/genbitmap.cpp  Bitmap_HSCB)
// Adapted for 16-bit GenTexture pixel range (0..65535).
// ============================================================

void HSCB(GenTexture& dst,
          const GenTexture& src,
          sF32 hue,
          sF32 sat,
          sF32 contrast,
          sF32 brightness) {
  // Build gamma/brightness lookup table (1025 entries, input 0..65535)
  static float gammaTable[1025];
  float fc = contrast * contrast;
  for (int i = 0; i < 1025; i++)
    gammaTable[i] =
        powf((i * 64.0f + 0.01f) / 65536.0f, fc) * 65535.0f * brightness;

  auto getGamma = [&](sU16 v) -> sS32 { return (sS32)gammaTable[v >> 6]; };
  auto clampOut = [](sS32 v) -> sU16 {
    return v < 0 ? 0 : v > 65535 ? 65535 : (sU16)v;
  };

  // hue in turns [0..1) → units of 1/6 circle, scaled by 65536
  sS32 ffh = (sS32)(hue * 6.0f * 65536.0f) % (6 * 65536);
  if (ffh < 0)
    ffh += 6 * 65536;
  sS32 ffs = (sS32)(sat * 65536.0f);

  const sU16* s = (const sU16*)src.Data;
  sU16* d = (sU16*)dst.Data;

  for (int i = 0; i < src.NPixels; i++) {
    // Read channels (GenTexture RGBA: s[0]=R, s[1]=G, s[2]=B)
    sS32 cr = getGamma(s[0]);
    sS32 cg = getGamma(s[1]);
    sS32 cb = getGamma(s[2]);

    // HSV adjust when hue or saturation changes
    if (ffh || ffs != 65536) {
      sS32 mx = cr > cg ? cr : cg;
      if (cb > mx)
        mx = cb;
      sS32 mn = cr < cg ? cr : cg;
      if (cb < mn)
        mn = cb;
      sS32 mm = mx - mn;

      sS32 ch;
      if (!mm) {
        ch = 0;
      } else {
        if (cr == mx)
          ch = 1 * 65536 + FDiv16(cg - cb, mm);
        else if (cg == mx)
          ch = 3 * 65536 + FDiv16(cb - cr, mm);
        else
          ch = 5 * 65536 + FDiv16(cr - cg, mm);
      }

      ch += ffh;
      mm = FMul16(mm, ffs);
      mn = mx - mm;

      if (ch > 6 * 65536)
        ch -= 6 * 65536;
      if (ch < 0)
        ch += 6 * 65536;

      sS32 rch = ch & 131071, m1, m2;
      m1 = mn + ((rch >= 65536) ? FMul16(rch - 65536, mm) : 0);
      m2 = mn + ((rch < 65536) ? FMul16(65536 - rch, mm) : 0);

      if (ch < 2 * 65536) {
        cr = mx;
        cg = m1;
        cb = m2;
      } else if (ch < 4 * 65536) {
        cr = m2;
        cg = mx;
        cb = m1;
      } else {
        cr = m1;
        cg = m2;
        cb = mx;
      }
    }

    d[0] = clampOut(cr);
    d[1] = clampOut(cg);
    d[2] = clampOut(cb);
    d[3] = s[3];
    s += 4;
    d += 4;
  }
}

// ============================================================
// Wavelet  (fr_public/werkkzeug3/w3texlib/genbitmap.cpp  Bitmap_Wavelet)
// Haar wavelet transform; operates on dst in-place.
// ============================================================

void Wavelet(GenTexture& dst, sInt mode, sInt count) {
  int xs = dst.XRes, ys = dst.YRes;
  int f = xs * 4;  // row stride in sU16 units
  int g = xs * 8;  // 2x row stride

  sU16* mem = new sU16[xs * ys * 4];

  for (int steps = 0; steps < count; steps++) {
    // Horizontal pass: src = dst.Data, dst = mem
    sU16* s = (sU16*)dst.Data;
    sU16* d = mem;
    for (int y = 0; y < ys; y++) {
      for (int x = 0; x < xs / 2; x++) {
        for (int c = 0; c < 4; c++) {
          int a = s[x * 4 + c];
          int b = s[x * 4 + c + xs * 2];
          if (mode) {
            // inverse: reconstruct from low/high
            d[x * 8 + c] = (sU16)sClamp<int>(b - (a - 0x4000), 0, 0x7fff);
            d[x * 8 + c + 4] = (sU16)sClamp<int>(b + (a - 0x4000), 0, 0x7fff);
          } else {
            // forward: split into low/high
            d[x * 4 + c] = (sU16)((b - a + 0x8000L) / 2);
            d[x * 4 + c + xs * 2] = (sU16)((b + a) / 2);
          }
        }
      }
      s += xs * 4;
      d += xs * 4;
    }

    // Vertical pass: src = mem, dst = dst.Data
    s = mem;
    d = (sU16*)dst.Data;
    for (int x = 0; x < xs; x++) {
      for (int y = 0; y < ys / 2; y++) {
        for (int c = 0; c < 4; c++) {
          int a = s[y * f + c];
          int b = s[y * f + c + ys * f / 2];
          if (mode) {
            d[y * g + c] = (sU16)sClamp<int>(b - (a - 0x4000), 0, 0x7fff);
            d[y * g + c + f] = (sU16)sClamp<int>(b + (a - 0x4000), 0, 0x7fff);
          } else {
            d[y * f + c] = (sU16)((b - a + 0x8000L) / 2);
            d[y * f + c + ys * f / 2] = (sU16)((b + a) / 2);
          }
        }
      }
      s += 4;
      d += 4;
    }
  }

  delete[] mem;
}

// ============================================================
// ColorBalance  (fr_public/werkkzeug3/w3texlib/genbitmap.cpp)
// Adapted for 16-bit GenTexture pixel range.
// ============================================================

void ColorBalance(GenTexture& dst,
                  const GenTexture& src,
                  sF32 shadowR,
                  sF32 shadowG,
                  sF32 shadowB,
                  sF32 midR,
                  sF32 midG,
                  sF32 midB,
                  sF32 highlightR,
                  sF32 highlightG,
                  sF32 highlightB) {
  static const sF32 sc = 100.0f / 255.0f;

  // Build 3 lookup tables (one per channel R/G/B)
  // Input: 16-bit value (0..65535); table indexed by v >> 8 (256 entries)
  static sU16 table[3][257];

  sF32 sha[3] = {shadowR, shadowG, shadowB};
  sF32 mid[3] = {midR, midG, midB};
  sF32 hil[3] = {highlightR, highlightG, highlightB};

  for (int j = 0; j < 3; j++) {
    sF32 vsha = sha[j], vmid = mid[j], vhil = hil[j];
    sF32 p = powf(0.5f, vsha * 0.5f + vmid + vhil * 0.5f);
    sF32 mn = -sMin(vsha, 0.0f) * sc;
    sF32 mx = 1.0f - sMax(vhil, 0.0f) * sc;
    sF32 msc = 1.0f / (mx - mn);

    for (int i = 0; i <= 256; i++) {
      sF32 xv = i / 256.0f;
      xv = sClamp(xv, mn, mx);
      xv = powf((xv - mn) * msc, p);
      table[j][i] = (sU16)clamp16((sS32)(xv * 65535.0f));
    }
  }

  auto lookup = [](sU16* tbl, sU16 v) -> sU16 { return tbl[v >> 8]; };

  const sU16* s = (const sU16*)src.Data;
  sU16* d = (sU16*)dst.Data;

  for (int i = 0; i < src.NPixels; i++) {
    // RGBA: s[0]=R → table[0], s[1]=G → table[1], s[2]=B → table[2]
    d[0] = lookup(table[0], s[0]);
    d[1] = lookup(table[1], s[1]);
    d[2] = lookup(table[2], s[2]);
    d[3] = s[3];
    s += 4;
    d += 4;
  }
}

// ============================================================
// Bricks  (fr_public/werkkzeug3/w3texlib/genbitmap.cpp  Bitmap_Bricks)
// ============================================================

static sU32 g_rnd = 0;
static void rndSeed(sU32 seed) {
  g_rnd = seed;
}
static sU32 rndNext() {
  g_rnd = g_rnd * 0x15a4e35u + 1;
  return g_rnd >> 16;
}
// Returns random in [0, n-1]
static sU32 rndN(sU32 n) {
  return n ? rndNext() % n : 0;
}
// Returns random float in [0..1)
static sF32 rndF() {
  return (rndNext() & 0x7fff) / (sF32)0x8000;
}

// Lerp: out = c1 + (c2 - c1) * t / 0x10000  (per 16-bit channel, using Pixel)
static Pixel fadePixel(const Pixel& c1, const Pixel& c2, sS32 t) {
  Pixel out;
  out.Lerp(t, c1, c2);
  return out;
}

void Bricks(GenTexture& dst,
            sU32 col0,
            sU32 col1,
            sU32 colFuge,
            sF32 fugeX,
            sF32 fugeY,
            sInt tileX,
            sInt tileY,
            sU32 seed,
            sInt heads,
            sF32 colorBalance) {
  Pixel c0, c1, cf;
  c0.Init(col0);
  c1.Init(col1);
  cf.Init(colFuge);

  int fugex = (int)(fugeX * 0x2000);
  int fugey = (int)(fugeY * 0x2000);

  struct Cell {
    Pixel color;
    int kopf;
  };
  Cell* cells = new Cell[tileX * tileY];

  rndSeed(seed);

  // Distribute brick-start markers (kopf = start of new brick)
  int kopf = 0;
  for (int y = 0; y < tileY; y++) {
    int kopftrigger = 0;
    for (int x = 0; x < tileX; x++) {
      kopftrigger--;
      cells[y * tileX + x].color.Init(0xff000000);
      cells[y * tileX + x].kopf = kopf;
      if (kopf == 0) {
        kopf = 1;
      } else {
        if ((int)rndN(255) > heads || kopftrigger >= 0)
          kopf = 0;
      }
    }
    if (!cells[y * tileX].kopf && !cells[y * tileX + tileX - 1].kopf)
      cells[y * tileX].kopf = 1;
  }

  // Assign a random color to each brick
  Pixel cb = fadePixel(c0, c1, (sS32)(powf(rndF(), colorBalance) * 0x10000));
  for (int y = 0; y < tileY; y++) {
    for (int x = 0; x < tileX; x++) {
      if (cells[y * tileX + x].kopf)
        cb = fadePixel(c0, c1, (sS32)(powf(rndF(), colorBalance) * 0x10000));
      cells[y * tileX + x].color = cb;
    }
    if (!cells[y * tileX].kopf)
      cells[y * tileX].color = cells[y * tileX + tileX - 1].color;
  }

  // Paint each pixel
  Pixel* d = dst.Data;
  for (int y = 0; y < dst.YRes; y++) {
    int fy = (0x4000 / dst.YRes) * y * tileY;
    int by = fy >> 14;
    if (by >= tileY)
      by = tileY - 1;
    fy &= 0x3fff;

    for (int x = 0; x < dst.XRes; x++) {
      int fx = (0x4000 / dst.XRes) * x * tileX;
      int bx = fx >> 14;
      if (bx >= tileX)
        bx -= tileX;
      fx &= 0x3fff;

      int f = 0x4000;
      if (cells[by * tileX + (bx) % tileX].kopf)
        if (fx < fugex)
          f = 0x4000 * fx / fugex;
      if (cells[by * tileX + (bx + 1) % tileX].kopf)
        if (0x4000 - fx < fugex)
          f = 0x4000 * (0x4000 - fx) / fugex;
      if (fy < fugey)
        f = sMin(f, 0x4000 * fy / fugey);
      if (0x4000 - fy < fugey)
        f = sMin(f, 0x4000 * (0x4000 - fy) / fugey);

      *d++ = fadePixel(cf, cells[by * tileX + bx % tileX].color, f * 4);
    }
  }

  delete[] cells;
}
