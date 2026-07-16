#pragma once
// mm_fill — Material Maker fill-family ports (fill.mmg / fill2.mmg and
// fill_to_*.mmg consumers). MM computes the fill iteratively in shaders;
// here it is an exact connected-components pass on the CPU.
//
// The fill map convention (matches MM): each region pixel carries the
// normalized bounding box of its region as RGBA = (x, y, w, h); edge
// pixels (input luminance > 0.5) carry (0, 0, 0, 0). Regions connect
// across the texture borders (torus topology), like MM's fract-based
// iteration.

#include "gentexture.hpp"

// Connected components of the dark areas of 'in' -> fill map.
void MMFill(GenTexture &out, const GenTexture &in);

// Local UVs inside each region (fill_to_uv.mmg): RG = position within
// the region bbox (mode 0 stretches, 1 keeps the aspect square), B = a
// per-region random value.
void MMFillToUV(GenTexture &out, const GenTexture &fill, sInt mode,
                sF32 seed);

// Random grey/color per region (fill_to_random_grey/color.mmg); edge
// pixels get edgeR/G/B.
void MMFillToRandomGray(GenTexture &out, const GenTexture &fill,
                        sF32 edgecolor, sF32 seed);
void MMFillToRandomColor(GenTexture &out, const GenTexture &fill,
                         sF32 edgeR, sF32 edgeG, sF32 edgeB, sF32 seed);

// Samples 'map' at each region's center (fill_to_color.mmg); edge
// pixels get the edge color.
void MMFillToColor(GenTexture &out, const GenTexture &fill,
                   const GenTexture *map, sF32 edgeR, sF32 edgeG,
                   sF32 edgeB, sF32 edgeA);
