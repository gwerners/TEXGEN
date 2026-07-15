#pragma once
// mm_sdf — 2D signed distance field nodes ported from Material Maker.
//
// Ported from Material Maker (https://github.com/RodZill4/material-maker)
// MIT License, Copyright (c) 2018-present Rodolphe Suescun and contributors.
// Star SDF based on https://www.shadertoy.com/view/3tSGDy
// (MIT License, Copyright (c) 2019 Inigo Quilez).
//
// Distances are stored in grayscale GenTextures with the encoding
//   gray = d * MMSDF_SCALE + MMSDF_OFFSET   (d in [-2, +2] covers [0, 1])
// so SDF textures can flow through the regular node graph.

#include "gentexture.hpp"

const sF32 MMSDF_SCALE = 0.25f;
const sF32 MMSDF_OFFSET = 0.5f;

inline sF32 mmSdfDecode(sF32 gray) {
  return (gray - MMSDF_OFFSET) / MMSDF_SCALE;
}
inline sF32 mmSdfEncode(sF32 d) {
  sF32 g = d * MMSDF_SCALE + MMSDF_OFFSET;
  return g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
}

enum MMSdfShapeType {
  MMSdfCircle = 0,  // center (cx,cy), radius w
  MMSdfBox,         // center (cx,cy), half-size (w,h)
  MMSdfLine,        // segment (ax,ay)-(bx,by), width w
  MMSdfStar,        // center, radius w, points n, inner ratio ir, rotation
  MMSdfNgon,        // center, radius w, sides n, rotation
  MMSdfRhombus,     // center (cx,cy), half-size (w,h)
  MMSdfShapeCount
};

struct MMSdfShapeParams {
  sInt shape = MMSdfCircle;
  sF32 cx = 0.5f, cy = 0.5f; // center (uv space)
  sF32 w = 0.3f, h = 0.2f;   // radius / half-size / line width
  sInt n = 5;                // star points / ngon sides
  sF32 ir = 0.5f;            // star inner ratio (0..1)
  sF32 rot = 0.0f;           // rotation in degrees
  sF32 ax = 0.2f, ay = 0.2f; // line endpoint A
  sF32 bx = 0.8f, by = 0.8f; // line endpoint B
};

// Render a shape's SDF into 'out' (encoded grayscale).
void MMSdfShape(GenTexture &out, const MMSdfShapeParams &p);

enum MMSdfOpType {
  MMSdfUnion = 0,
  MMSdfSubtraction, // removes A from B
  MMSdfIntersection,
  MMSdfSmoothUnion,
  MMSdfSmoothSubtraction,
  MMSdfSmoothIntersection,
  MMSdfMorph, // mix(A, B, k)
  MMSdfOpCount
};

// Combine two SDF textures (encoded grayscale in, encoded grayscale out).
// k = smoothness for the smooth ops / mix amount for morph.
void MMSdfOp(GenTexture &out, const GenTexture &a, const GenTexture &b,
             sInt op, sF32 k);

// Transform an SDF: translate/rotate/scale, then round (d-r) and
// annular ripples (d = abs(d)-w, repeated).
void MMSdfTransform(GenTexture &out, const GenTexture &in, sF32 tx, sF32 ty,
                    sF32 rotDeg, sF32 scale, sF32 roundR, sF32 annularW,
                    sInt annularCount);

// Convert an SDF to a grayscale image: clamp(base - d/max(bevel,eps), 0, 1).
void MMSdfShow(GenTexture &out, const GenTexture &in, sF32 base, sF32 bevel);
