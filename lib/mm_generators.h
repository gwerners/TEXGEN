#pragma once
// mm_generators — CPU ports of Material Maker procedural generators.
//
// Ported from Material Maker (https://github.com/RodZill4/material-maker)
// MIT License, Copyright (c) 2018-present Rodolphe Suescun and contributors.
// Voronoi cell math based on https://www.shadertoy.com/view/ldl3W8
// (MIT License, Copyright (c) 2013 Inigo Quilez).
//
// All functions are deterministic (same params -> same pixels on any
// machine), operate on 16-bit GenTexture buffers and have no UI or I/O
// dependencies. Output textures must be Init()'d by the caller.

#include "gentexture.hpp"

// Tileable Voronoi. Fills up to three outputs (any may be null):
//   outColor - random color per cell
//   outF1    - grayscale distance to cell centers (scaled by intensity)
//   outEdge  - grayscale distance to cell borders
void MMVoronoi(GenTexture *outColor, GenTexture *outF1, GenTexture *outEdge,
               sInt scaleX, sInt scaleY, sF32 stretchX, sF32 stretchY,
               sF32 intensity, sF32 randomness, sF32 seed);

// FBM noise variants (all tileable).
enum MMFbmMode {
  MMFbmValue = 0,
  MMFbmPerlin,
  MMFbmPerlinAbs,
  MMFbmCellular,   // euclidean F1
  MMFbmCellular2,  // euclidean F2-F1
  MMFbmCellular3,  // manhattan F1
  MMFbmCellular4,  // manhattan F2-F1
  MMFbmCellular5,  // chebyshev F1
  MMFbmCellular6,  // chebyshev F2-F1
  MMFbmModeCount
};
void MMFbm(GenTexture &out, sInt mode, sInt scaleX, sInt scaleY, sInt folds,
           sInt octaves, sF32 persistence, sF32 seed);

// Photoshop-style blend of a (top) over b (bottom).
// mask (optional, grayscale) and opacity modulate the blend amount.
enum MMBlendMode {
  MMBlendNormal = 0,
  MMBlendDissolve,
  MMBlendMultiply,
  MMBlendScreen,
  MMBlendOverlay,
  MMBlendHardLight,
  MMBlendSoftLight,
  MMBlendBurn,
  MMBlendDodge,
  MMBlendLighten,
  MMBlendDarken,
  MMBlendDifference,
  MMBlendAdditive,
  MMBlendAddSub,
  MMBlendLinearLight,
  MMBlendModeCount
};
void MMBlend(GenTexture &out, const GenTexture &a, const GenTexture &b,
             const GenTexture *mask, sInt mode, sF32 opacity);

// Warp: displaces the UVs of 'in' along the gradient (finite differences,
// half-pixel epsilon scaled by 'epsilon') of the grayscale 'height' map.
// strength (optional, grayscale) modulates the displacement per pixel.
void MMWarp(GenTexture &out, const GenTexture &in, const GenTexture &height,
            const GenTexture *strength, sF32 amount, sF32 epsilon);

// Colorize: maps input luminance through a multi-stop color ramp.
// stops = nStops entries of {pos, r, g, b, a}, sorted by pos ascending.
struct MMGradientStop {
  sF32 pos;
  sF32 r, g, b, a;
};
void MMColorize(GenTexture &out, const GenTexture &in,
                const MMGradientStop *stops, sInt nStops);

// Brick tessellation patterns (Material Maker layouts).
enum MMBricksPattern {
  MMBricksRunningBond = 0,
  MMBricksRunningBond2,  // alternating row density
  MMBricksHerringbone,
  MMBricksBasketWeave,
  MMBricksSpanishBond,
  MMBricksPatternCount
};
// Renders colored bricks: mortar/round/bevel via a rounded-rect SDF per
// brick, random per-brick color blend between col0 and col1.
void MMBricks(GenTexture &out, sInt pattern, sInt countX, sInt countY,
              sInt repeat, sF32 offset, sF32 mortar, sF32 roundRadius,
              sF32 bevel, sU32 col0, sU32 col1, sU32 colMortar,
              sF32 colorBalance, sF32 seed);
