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
void MMTransform(GenTexture &out, const GenTexture &in, sF32 tx, sF32 ty,
                 sF32 rotDeg, sF32 scaleX, sF32 scaleY, bool repeat);
