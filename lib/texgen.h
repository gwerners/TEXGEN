#pragma once
// libtexgen — procedural texture generation library.
//
// Includes the full texture generation pipeline:
//   - GenTexture pixel buffer (16-bit RGBA, compositing, sampling, transforms)
//   - Procedural generators (Noise, Cells, Crystal, Bricks, Perlin, Voronoi)
//   - AGG 2.4 vector drawing bridge (lines, curves, text, anti-aliasing)
//   - Matrix and color utilities
//   - TGA file export
//
// No UI or graphics API dependencies.

// Core pixel buffer and types
#include "types.hpp"
#include "gentexture.hpp"

// Procedural generators (Crystal, Perlin, Bricks, Blur, HSCB, etc.)
#include "extra_generators.hpp"

// AGG 2.4 vector drawing bridge (zero-copy rendering into GenTexture)
#include "agg_gentexture.h"

// Pure utility functions (matrix ops, gradients, SaveImage, Voronoi)
#include "texgen_utils.h"

// Headless graph evaluation (JSON project -> GenTexture output)
#include "HeadlessEval.h"

// Core node base class and registry (for extending with custom nodes)
#include "CoreNode.h"
#include "CoreNodeRegistry.h"
