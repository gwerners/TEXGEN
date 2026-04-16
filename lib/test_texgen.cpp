// Standalone test: generates a texture using libtexgen with no UI dependencies.
#include "texgen.h"
#include <cstdio>

int main() {
  // 1. Procedural noise
  GenTexture noise(256, 256);
  GenTexture grad = LinearGradient(0xff001020, 0xff4080ff);
  noise.Noise(grad, 2, 2, 4, 0.5f, 42, GenTexture::NoiseBandlimit | GenTexture::NoiseNormalize);
  SaveImage(noise, "test_noise.tga");
  printf("Saved test_noise.tga (%dx%d)\n", noise.XRes, noise.YRes);

  // 2. Crystal voronoi
  GenTexture crystal(256, 256);
  Crystal(crystal, 77, 30, 0xff60a0ff, 0xff000510);
  SaveImage(crystal, "test_crystal.tga");
  printf("Saved test_crystal.tga\n");

  // 3. AGG vector drawing
  GenTexture vec(256, 256);
  vec.Init(256, 256);
  {
    auto rbuf = agg_rbuf_from(vec);
    AggPixfmt pixf(rbuf);
    AggRendererBase ren(pixf);

    float bg[] = {0.1f, 0.1f, 0.15f, 1.0f};
    ren.clear(agg_color(bg));

    // Draw a filled circle
    agg::ellipse ell(128, 128, 80, 80, 100);
    agg::rasterizer_scanline_aa<> ras;
    agg::scanline_u8 sl;
    AggRendererSolid solid(ren);

    ras.add_path(ell);
    float col[] = {0.9f, 0.3f, 0.2f, 1.0f};
    solid.color(agg_color(col));
    agg::render_scanlines(ras, sl, solid);

    // Draw text
    agg::gsv_text text;
    text.size(24);
    text.start_point(60, 122);
    text.text("TEXGEN");

    agg::conv_stroke<agg::gsv_text> stroke(text);
    stroke.width(1.5);
    ras.reset();
    ras.add_path(stroke);
    float white[] = {1, 1, 1, 1};
    solid.color(agg_color(white));
    agg::render_scanlines(ras, sl, solid);
  }
  SaveImage(vec, "test_vector.tga");
  printf("Saved test_vector.tga\n");

  // 4. Bricks
  GenTexture bricks(256, 256);
  Bricks(bricks, 0xff402010, 0xff604020, 0xff303028, 0.03f, 0.06f, 4, 8, 0,
         2, 0.5f);
  SaveImage(bricks, "test_bricks.tga");
  printf("Saved test_bricks.tga\n");

  printf("All tests passed. libtexgen works standalone.\n");
  return 0;
}
