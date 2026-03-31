/*
 * extra_generators.hpp
 *
 * Texture generation functions ported from fr_public (RG2 and werkkzeug3)
 * that are NOT present in the base gentexture library.
 *
 * Sources:
 *   - fr_public/RG2/texgenerate.cpp  (frTGCrystal, frTGGradient, frTGPerlinNoise)
 *   - fr_public/RG2/texfilter.cpp    (blur with kernel type selection)
 *   - fr_public/werkkzeug3/w3texlib/genbitmap.cpp  (HSCB, Wavelet, ColorBalance, Bricks)
 *
 * All code is in the public domain (original sources: (C) Farbrausch, public domain).
 */
#pragma once

#include "gentexture.hpp"

// ---------------------------------------------------------------------------
// From fr_public/RG2/texgenerate.cpp  (frTGCrystal)
//
// Voronoi (crystal) pattern with a distance-based 2-color ramp.
//   seed:      random seed for cell positions
//   count:     number of Voronoi cells (1..256)
//   colorNear: color at cell centers  (0xaarrggbb)
//   colorFar:  color at cell edges    (0xaarrggbb)
// dst must already be Init()'d.
// ---------------------------------------------------------------------------
void Crystal(GenTexture& dst, sU32 seed, sInt count, sU32 colorNear, sU32 colorFar);

// ---------------------------------------------------------------------------
// From fr_public/RG2/texgenerate.cpp  (frTGGradient, linear mode)
//
// Linear gradient from (x1,y1) to (x2,y2) in texture UV space [0..1].
//   col1 / col2: start / end colors (0xaarrggbb)
// dst must already be Init()'d.
// ---------------------------------------------------------------------------
void DirectionalGradient(GenTexture& dst,
                         sF32 x1, sF32 y1, sF32 x2, sF32 y2,
                         sU32 col1, sU32 col2);

// ---------------------------------------------------------------------------
// From fr_public/RG2/texgenerate.cpp  (frTGGradient, glow mode)
//
// Radial glow effect.
//   cx, cy:    center position in UV [0..1]
//   scale:     radius scale  (0.01..5)
//   exponent:  falloff sharpness
//   intensity: peak brightness
//   bgCol:     background color  (0xaarrggbb)
//   glowCol:   glow center color (0xaarrggbb)
// dst must already be Init()'d.
// ---------------------------------------------------------------------------
void GlowEffect(GenTexture& dst,
                sF32 cx, sF32 cy, sF32 scale, sF32 exponent, sF32 intensity,
                sU32 bgCol, sU32 glowCol);

// ---------------------------------------------------------------------------
// From fr_public/RG2/texgenerate.cpp  (frTGPerlinNoise)
//
// Value noise with 2-color ramp; uses a different noise basis than ktg Noise().
//   octaves:     number of octaves (1..12)
//   persistence: amplitude scale per octave  (0..1)
//   freqScale:   starting frequency as power-of-2 exponent (0..10)
//   seed:        random seed
//   contrast:    output contrast multiplier
//   col1 / col2: low / high color ramp (0xaarrggbb)
//   startOctave: first octave to accumulate (0..octaves-1)
// dst must already be Init()'d.
// ---------------------------------------------------------------------------
void PerlinNoiseRG2(GenTexture& dst,
                    sInt octaves, sF32 persistence, sInt freqScale, sU32 seed,
                    sF32 contrast, sU32 col1, sU32 col2,
                    sInt startOctave = 0);

// ---------------------------------------------------------------------------
// From fr_public/RG2/texfilter.cpp  (blur helper with kernel type)
//
// Blur with selectable kernel shape.
//   radiusX/Y:  blur radius in texture-coordinate fraction (0..1)
//   kernelType: 0 = box, 1 = triangle, 2 = Gaussian
//   wrapMode:   GenTexture::WrapU | GenTexture::WrapV  (or Clamp)
// dst must already be Init()'d with the same size as src.
// ---------------------------------------------------------------------------
void BlurKernel(GenTexture& dst, const GenTexture& src,
                sF32 radiusX, sF32 radiusY,
                sInt kernelType, sInt wrapMode);

// ---------------------------------------------------------------------------
// From fr_public/werkkzeug3/w3texlib/genbitmap.cpp  (Bitmap_HSCB)
//
// Adjust Hue, Saturation, Contrast (gamma) and Brightness.
//   hue:        hue rotation in turns [0..1]  (0 = no change)
//   sat:        saturation scale  (1.0 = unchanged)
//   contrast:   gamma exponent    (1.0 = unchanged; <1 = brighter mid-tones)
//   brightness: linear brightness multiplier (1.0 = unchanged)
// dst and src may be the same texture (in-place).
// dst must already be Init()'d.
// ---------------------------------------------------------------------------
void HSCB(GenTexture& dst, const GenTexture& src,
          sF32 hue, sF32 sat, sF32 contrast, sF32 brightness);

// ---------------------------------------------------------------------------
// From fr_public/werkkzeug3/w3texlib/genbitmap.cpp  (Bitmap_Wavelet)
//
// Haar wavelet transform (or inverse).
//   mode:  0 = forward decompose, 1 = inverse recompose
//   count: number of transform steps
// Operates in-place; dst is modified directly (src == &dst is intended).
// dst must already be Init()'d.
// ---------------------------------------------------------------------------
void Wavelet(GenTexture& dst, sInt mode, sInt count);

// ---------------------------------------------------------------------------
// From fr_public/werkkzeug3/w3texlib/genbitmap.cpp  (Bitmap_ColorBalance)
//
// Adjust shadows, midtones and highlights independently per RGB channel.
// Each parameter is in the range [-1..1] (0 = no change).
//   shadow*/mid*/highlight*:  per-channel adjustments for R, G, B
// dst must already be Init()'d with the same size as src.
// ---------------------------------------------------------------------------
void ColorBalance(GenTexture& dst, const GenTexture& src,
                  sF32 shadowR,    sF32 shadowG,    sF32 shadowB,
                  sF32 midR,       sF32 midG,       sF32 midB,
                  sF32 highlightR, sF32 highlightG, sF32 highlightB);

// ---------------------------------------------------------------------------
// From fr_public/werkkzeug3/w3texlib/genbitmap.cpp  (Bitmap_Bricks)
//
// Procedural brick / tile pattern.
//   col0, col1:    brick color range (dark / light) (0xaarrggbb)
//   colFuge:       mortar / grout color             (0xaarrggbb)
//   fugeX, fugeY:  mortar thickness as fraction of brick size (0..1)
//   tileX, tileY:  number of bricks across / down
//   seed:          random seed
//   heads:         probability of a new brick starting (0=never, 255=always)
//   colorBalance:  gamma exponent for random brick color variation (>0)
// dst must already be Init()'d with the desired output resolution.
// ---------------------------------------------------------------------------
void Bricks(GenTexture& dst,
            sU32 col0, sU32 col1, sU32 colFuge,
            sF32 fugeX, sF32 fugeY,
            sInt tileX, sInt tileY,
            sU32 seed, sInt heads, sF32 colorBalance);
