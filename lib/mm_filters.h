#pragma once
// mm_filters — image filters ported from Material Maker.
// MIT License, Copyright (c) 2018-present Rodolphe Suescun and contributors.
// See mm_generators.h for full attribution notes.

#include "gentexture.hpp"

enum MMNormalFormat {
  MMNormalDefault = 0, // -Z convention (Material Maker default)
  MMNormalOpenGL,      // +Z, Y up
  MMNormalDirectX,     // +Z, Y down
};

// Height (grayscale) -> tangent-space normal map, Sobel-based, tileable.
void MMNormalMap(GenTexture &out, const GenTexture &height, sF32 amount,
                 sInt format);

// Blends the image with offset copies of itself to hide seams.
// w = transition width (0..0.25 in MM's UI; internally doubled).
void MMMakeTileable(GenTexture &out, const GenTexture &in, sF32 w);

// Quantize RGB into 'steps' levels (alpha preserved).
void MMQuantize(GenTexture &out, const GenTexture &in, sInt steps);

// Directional emboss of a grayscale input.
// angleDeg = light direction, width = kernel radius in pixels.
void MMEmboss(GenTexture &out, const GenTexture &in, sF32 angleDeg,
              sF32 amount, sInt width);

// 2D affine transform (transform.mmg): translate, rotate around the
// center, scale, then wrap (repeat=true) or clamp the source UVs.
// The optional maps modulate each parameter per pixel, MM-style:
// effective = param * (2*map(uv) - 1); a null map means neutral (1).
void MMTransform(GenTexture &out, const GenTexture &in, sF32 tx, sF32 ty,
                 sF32 rotDeg, sF32 scaleX, sF32 scaleY, bool repeat,
                 const GenTexture *mapTx = nullptr,
                 const GenTexture *mapTy = nullptr,
                 const GenTexture *mapRot = nullptr,
                 const GenTexture *mapSx = nullptr,
                 const GenTexture *mapSy = nullptr);

// Channel plumbing (combine.mmg / decompose.mmg / invert.mmg).
// Combine: builds RGBA from four grayscale inputs (null r/g/b -> 0,
// null a -> 1). Decompose: splits RGBA into four grayscale outputs.
void MMCombine(GenTexture &out, const GenTexture *r, const GenTexture *g,
               const GenTexture *b, const GenTexture *a);
void MMDecompose(GenTexture &outR, GenTexture &outG, GenTexture &outB,
                 GenTexture &outA, const GenTexture &in);
void MMInvert(GenTexture &out, const GenTexture &in);

// Per-pixel scalar math on grayscale inputs (math.mmg). Ops:
//  0 a+b   1 a-b    2 a*b     3 a/b      4 log(a)   5 log2(a)
//  6 a^b   7 |a|    8 round   9 floor   10 ceil    11 trunc
// 12 fract 13 min  14 max    15 a<b     16 cos(ab) 17 sin(ab)
// 18 tan(ab) 19 sqrt(1-a*a) 20 smoothstep(0,1,a) 21 pingpong(a,b)
// 22 sign 23 mod 24 atan2 25 asin 26 acos 27 atan 28 sinh 29 cosh
// 30 tanh 31 exp 32 snap(a,b) 33 radians 34 degrees 35 log_b(a)
// 36 sqrt(a)
// Unconnected inputs (null) use the def1/def2 constants.
void MMMath(GenTexture &out, const GenTexture *in1, const GenTexture *in2,
            sInt op, sF32 def1, sF32 def2, bool clampResult);

// Iterative slope-following warp (multi_warp.mmg): each pixel walks
// along the heightmap gradient for ceil(intensity*quality) steps and
// accumulates samples of 'in'. mode: 0 min, 1 blur (average), 2 max.
void MMMultiWarp(GenTexture &out, const GenTexture &in,
                 const GenTexture &height, sF32 size, sF32 intensity,
                 sF32 quality, sInt mode);

// Instance scatter/tiler (tiler.mmg): composites tx*ty jittered copies
// of 'in' (random rotate/scale/offset, per-instance value attenuation,
// optional mask), keeping the max per pixel. out receives the grayscale
// composite; outColor (optional) a random color per winning instance.
// inputs = tileset subdivision of 'in' (1, 2 or 4); the per-instance
// 'variation' sampling of MM is approximated by plain input sampling.
void MMTiler(GenTexture &out, GenTexture *outColor, const GenTexture &in,
             const GenTexture *mask, sF32 tx, sF32 ty, sInt overlap,
             sInt inputs, sF32 scaleX, sF32 scaleY, sF32 fixedOffset,
             sF32 offset, sF32 rotateDeg, sF32 scaleJitter, sF32 value,
             sF32 seed);

// Directional gaussian blur along the heightmap slope (slope_blur.mmg):
// each pixel accumulates 50 samples of 'in' along the normalized slope
// direction, with sigma scaled by the local slope strength.
void MMSlopeBlur(GenTexture &out, const GenTexture &in,
                 const GenTexture &height, sF32 size, sF32 sigma);

// UV mirror (mirror.mmg). direction: 0 horizontal, 1 vertical.
void MMMirror(GenTexture &out, const GenTexture &in, sInt direction,
              sF32 offset, bool flipSides);

// Edge detection (edge_detect.mmg): 8-direction color-distance sum at
// 1/size texel steps, 'width' rings, thresholded to a hard mask.
void MMEdgeDetect(GenTexture &out, const GenTexture &in, sF32 size,
                  sInt width, sF32 threshold);

// Linear remap with optional quantization (remap.mmg):
// out = min + q(in * (max - min)), q quantizes to 'step' when > 0.
void MMRemap(GenTexture &out, const GenTexture &in, sF32 minV, sF32 maxV,
             sF32 step);

// Packs four inputs into quadrants (tile2x2.mmg); null inputs = black.
void MMTile2x2(GenTexture &out, const GenTexture *in1, const GenTexture *in2,
               const GenTexture *in3, const GenTexture *in4);

// Normal map convention conversion (normal_map_convert.mmg):
// op 0 = from/to OpenGL (flip R and B), 1 = from/to DirectX (flip all).
void MMNormalConvert(GenTexture &out, const GenTexture &in, sInt op);

// Per-region UV scatter (custom_uv.mmg): samples 'in' at map.xy
// transformed by a random rotation/scale seeded from map.z (typically
// a FillToUV output). inputs = tileset subdivision (1, 2 or 4).
void MMCustomUV(GenTexture &out, const GenTexture &in, const GenTexture &map,
                sInt inputs, sF32 sx, sF32 sy, sF32 rotateDeg,
                sF32 scaleJitter, sF32 seed);

// Smooth curvature of a heightmap (smooth_curvature2.mmg): quality^2
// laplacian samples weighted by distance inside 0.05*radius.
void MMSmoothCurvature(GenTexture &out, const GenTexture &height,
                       sF32 quality, sF32 strength, sF32 radius);

// Ambient occlusion approximation from a heightmap (occlusion2.mmg /
// hbao.mmg are shader-based; this uses the blur-minus-height trick).
void MMAmbientOcclusion(GenTexture &out, const GenTexture &height,
                        sF32 radius, sF32 strength);

// Levels adjustment (tones.mmg): per-channel input range remap with a
// midtone gamma pivot, then output range remap.
void MMLevels(GenTexture &out, const GenTexture &in, const sF32 inMin[4],
              const sF32 inMid[4], const sF32 inMax[4], const sF32 outMin[4],
              const sF32 outMax[4]);

// Advanced tiler (tiler_advanced.mmg): grid scatter with PER-INSTANCE
// translate/rotate/scale modulation maps (sampled at each instance's
// cell position). Outputs: composite grayscale, two colors sampled at
// the winning cell (random when the color inputs are missing) and the
// instance UV map (xy = local uv, z = per-instance random) for
// CustomUV-style consumers.
void MMTilerAdvanced(GenTexture &out, GenTexture *outColor1,
                     GenTexture *outColor2, GenTexture *outUV,
                     const GenTexture &in, const GenTexture *mask,
                     const GenTexture *trX, const GenTexture *trY,
                     const GenTexture *rMap, const GenTexture *scX,
                     const GenTexture *scY, const GenTexture *color1,
                     const GenTexture *color2, sF32 tx, sF32 ty,
                     sInt overlap, sInt inputs, sF32 translateX,
                     sF32 translateY, sF32 rotateDeg, sF32 scaleX,
                     sF32 scaleY, sF32 seed);

// Offset toward a height contour (height_to_offset.mmg): per pixel,
// offset = 16 * grad(h) / |grad(h)|^2 * (target - h), split into X/Y
// outputs. MM emits signed floats; this 16-bit pipeline clamps the
// offsets to [0, 1], so strong pushes saturate.
void MMHeightToOffset(GenTexture &outX, GenTexture &outY,
                      const GenTexture &in, sF32 target);

// Control point of an MM curve widget (x, y, left/right slopes).
struct MMCurvePoint {
  sF32 x, y, ls, rs;
};

// Piecewise cubic Hermite curve through MM control points; clamps to
// the end values outside [first.x, last.x]. Identity when empty.
sF32 MMCurveEval(const MMCurvePoint *pts, sInt n, sF32 x);

// Bevel (bevel.mmg = dilate + tonality): grows the white areas
// (gray >= 0.5) of the mask outward with a linear ramp over
// 'distance' (UV units) using an exact toroidal Euclidean distance
// transform, then reshapes the ramp with the curve. Soft masks are
// thresholded, which sharpens interiors slightly vs MM's dilate.
void MMBevel(GenTexture &out, const GenTexture &in, sF32 distance,
             const MMCurvePoint *curve, sInt nCurve);

// Dilate (dilate.mmg): spreads the source colors outward from the
// white (gray >= 0.5) areas of the mask with a ramp over 'length'
// (UV units). Each pixel takes the source color at its nearest white
// pixel (exact toroidal EDT), scaled by mix(ramp, 1, fill).
// metric: 0 euclidean (exact), 1 manhattan, 2 chebyshev — the last
// two are measured to the euclidean-nearest site, a close
// approximation of MM's scanline passes. Null source = white
// (grayscale ramp output, matching MM's default_color).
void MMDilate(GenTexture &out, const GenTexture &mask,
              const GenTexture *source, sF32 length, sF32 fill, sInt metric);

// Normal Blend (normal_blend.mmg): reoriented normal mapping — the
// foreground normal detail is rotated onto the background normal.
// amount is scaled per pixel by the optional grayscale mask. MM's
// shader is written for its "Default" normal format (Z stored
// inverted, flat = 0); our maps are OpenGL-style (flat = blue), so
// this applies the same whiteout RNM in our convention. Missing
// inputs default to the flat normal (0.5, 0.5, 1).
void MMNormalBlend(GenTexture &out, const GenTexture *fg,
                   const GenTexture *bg, const GenTexture *mask, sF32 amount);
