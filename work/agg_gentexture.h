#pragma once
// Bridge between AGG 2.4 and GenTexture pixel buffers.
//
// GenTexture::Pixel is {uint16_t r, g, b, a} — exactly agg::rgba16 in
// order_rgba layout.  This header provides a helper to wrap a GenTexture
// into an AGG rendering pipeline with zero copies.

#include "agg_basics.h"
#include "agg_color_rgba.h"
#include "agg_conv_stroke.h"
#include "agg_ellipse.h"
#include "agg_path_storage.h"
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_renderer_base.h"
#include "agg_renderer_scanline.h"
#include "agg_rendering_buffer.h"
#include "agg_rounded_rect.h"
#include "agg_scanline_u.h"
#include "agg_gsv_text.h"

#include "gentexture.hpp"

// Convenience types for GenTexture's 16-bit RGBA pixel layout.
using AggPixfmt = agg::pixfmt_rgba64_plain;
using AggRendererBase = agg::renderer_base<AggPixfmt>;
using AggRendererSolid = agg::renderer_scanline_aa_solid<AggRendererBase>;

// Wrap a GenTexture into an AGG rendering_buffer (zero-copy).
// GenTexture has Y=0 at the bottom (OpenGL convention), while AGG has Y=0
// at the top. A negative stride flips the vertical axis so AGG draws with
// the correct orientation. Pass the buffer start — AGG internally computes
// the last-row pointer when stride is negative.
inline agg::rendering_buffer agg_rbuf_from(GenTexture &tex) {
  int stride = tex.XRes * sizeof(Pixel);
  return agg::rendering_buffer(reinterpret_cast<agg::int8u *>(tex.Data),
                               tex.XRes, tex.YRes, -stride);
}

// Convert a float[4] color (0..1 range) to agg::rgba16.
inline agg::rgba16 agg_color(const float *c) {
  auto to16 = [](float v) -> agg::int16u {
    return static_cast<agg::int16u>(
        agg::iround(v * static_cast<float>(agg::rgba16::base_mask)));
  };
  return agg::rgba16(to16(c[0]), to16(c[1]), to16(c[2]), to16(c[3]));
}

// Convert a uint32_t 0xAARRGGBB (D3D style) color to agg::rgba16.
inline agg::rgba16 agg_color(sU32 rgba) {
  auto to16 = [](unsigned v8) -> agg::int16u {
    return static_cast<agg::int16u>((v8 << 8) | v8);
  };
  return agg::rgba16(to16((rgba >> 16) & 0xFF), to16((rgba >> 8) & 0xFF),
                     to16(rgba & 0xFF), to16((rgba >> 24) & 0xFF));
}
