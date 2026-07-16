#include "Icons.h"

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg.h>
#include <nanosvgrast.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

// 24x24 line icons, white strokes on transparent background
static const struct {
  const char* category;
  const char* svg;
} kIconSvgs[] = {
    {"Generator",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none" stroke-linecap="round">
<line x1="12" y1="3" x2="12" y2="21"/>
<line x1="4.2" y1="7.5" x2="19.8" y2="16.5"/>
<line x1="19.8" y1="7.5" x2="4.2" y2="16.5"/>
</g></svg>)"},
    {"Vector",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none" stroke-linecap="round">
<path d="M4 18 C 8 5 16 5 20 18"/>
<circle cx="4" cy="18" r="2"/>
<circle cx="20" cy="18" r="2"/>
</g></svg>)"},
    {"SDF",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none">
<circle cx="12" cy="12" r="4"/>
<circle cx="12" cy="12" r="9"/>
</g></svg>)"},
    {"Filter",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none" stroke-linejoin="round" stroke-linecap="round">
<path d="M3 4 L21 4 L14 12 L14 19 L10 17 L10 12 Z"/>
</g></svg>)"},
    {"Combine",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none">
<circle cx="9" cy="12" r="6"/>
<circle cx="15" cy="12" r="6"/>
</g></svg>)"},
    {"Material",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none" stroke-linejoin="round" stroke-linecap="round">
<path d="M12 3 L21 8 L12 13 L3 8 Z"/>
<path d="M3 13 L12 18 L21 13"/>
</g></svg>)"},
    {"Structure",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none" stroke-linejoin="round" stroke-linecap="round">
<rect x="3" y="3" width="6" height="5"/>
<rect x="15" y="3" width="6" height="5"/>
<rect x="9" y="16" width="6" height="5"/>
<line x1="6" y1="8" x2="12" y2="16"/>
<line x1="18" y1="8" x2="12" y2="16"/>
</g></svg>)"},
    {"Output",
     R"(<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
<g stroke="#e6e6e6" stroke-width="2" fill="none" stroke-linejoin="round" stroke-linecap="round">
<rect x="3" y="4" width="18" height="12" rx="1"/>
<line x1="12" y1="16" x2="12" y2="20"/>
<line x1="8" y1="20" x2="16" y2="20"/>
</g></svg>)"},
};

static Texture2D rasterizeIcon(const char* svg, int side) {
  Texture2D tex = {};

  // nsvgParse mutates the buffer, so parse a scratch copy
  char* copy = strdup(svg);
  if (!copy)
    return tex;
  NSVGimage* image = nsvgParse(copy, "px", 96.0f);
  free(copy);
  if (!image)
    return tex;

  NSVGrasterizer* rast = nsvgCreateRasterizer();
  if (rast) {
    std::vector<unsigned char> pixels(side * side * 4, 0);
    float scale = (image->width > 0) ? side / image->width : 1.0f;
    nsvgRasterize(rast, image, 0, 0, scale, pixels.data(), side, side,
                  side * 4);
    nsvgDeleteRasterizer(rast);

    Image img = {};
    img.data = pixels.data();
    img.width = side;
    img.height = side;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
  }
  nsvgDelete(image);
  return tex;
}

Texture2D* categoryIcon(const std::string& category) {
  static std::map<std::string, Texture2D> cache;

  auto it = cache.find(category);
  if (it == cache.end()) {
    Texture2D tex = {};
    for (const auto& entry : kIconSvgs) {
      if (category == entry.category) {
        tex = rasterizeIcon(entry.svg, 32);
        break;
      }
    }
    it = cache.emplace(category, tex).first;
  }
  return it->second.id != 0 ? &it->second : nullptr;
}
