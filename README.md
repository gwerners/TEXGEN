# TEXGEN

Node-based procedural texture generator for Linux. Connect generator, filter, and vector drawing nodes in a visual graph to create tileable textures, noise patterns, and materials — all in real time.

Built with C++20, raylib, Dear ImGui, and [AGG 2.4](https://github.com/gwerners/agg) for anti-aliased vector drawing.

Texture generation core based on [gentexture](https://github.com/farbrausch/fr_public) by Fabian Giesen (public domain).

## Screenshot

<p align="center">
  <img src="images/screen.png" alt="TEXGEN node editor" width="100%"/>
</p>

## Features

- **32 node types** — generators, filters, combiners, AGG vector drawing, and utility nodes
- **AGG 2.4 vector nodes** — Line, Circle, Rect (rounded corners), Polygon (stars), Text with anti-aliasing
- **Real-time preview** — auto-updates when parameters change
- **Export to C** — generate a standalone `.h` header that reproduces the pipeline with `libtexgen` calls
- **libtexgen** — core texture generation as a standalone static/shared library (no UI dependencies)
- **Undo/Redo** — 50 levels, with debounced parameter tracking
- **Copy/Paste** — duplicate node subgraphs with connections
- **Minimap** — click-drag navigation in the bottom-right corner
- **16-bit precision** — per-channel RGBA for high-quality compositing

## Requirements

- Linux
- g++ (C++20)
- cmake
- ninja
- git

## Build and Run

```bash
./run.bash
```

Clones all dependencies, builds raylib, builds the project, and runs it.

```bash
./clean.bash        # remove everything (deps + build)
./clean.bash build  # remove build products only
```

## Test Suite

Export C headers from all example projects, compile, and run them standalone (no raylib dependency):

```bash
./run_tests.bash
```

## Keyboard Shortcuts

### Canvas

| Shortcut | Action |
|----------|--------|
| Right-click | Open context menu (create nodes) |
| Middle-drag | Pan canvas |
| Scroll wheel | Zoom in/out |
| Delete | Remove selected nodes |
| Ctrl+Z | Undo (up to 50 levels) |
| Ctrl+Y | Redo |
| Ctrl+C | Copy selected nodes (with connections) |
| Ctrl+V | Paste copied nodes |

### Sliders

| Shortcut | Action |
|----------|--------|
| Scroll wheel (hover) | Increment/decrement value |
| Arrow Up/Down (hover) | Increment/decrement value by one step |
| Ctrl+Click | Type an exact value (can exceed min/max range) |

## Interface

The interface is split into three panels:

- **Left Panel** — Generate button, project save/load, Export C header
- **Bottom Panel** — Output image preview with zoom controls (+, -, 1:1). Updates automatically when parameters change
- **Right Panel** — Node canvas with minimap (bottom-right corner, click-drag to navigate)

## Nodes

### Sources (no inputs)

| Node | Description |
|------|-------------|
| **Color** | Solid color fill (configurable size and RGBA) |
| **Image** | Load an image file (PNG, JPG, TGA, BMP, PSD, GIF, HDR) with file browser |
| **Gradient** | 2-pixel gradient from Color1 to Color2. Used as color ramp for Noise/GlowRect |

### Generators (procedural patterns)

| Node | Description |
|------|-------------|
| **Noise** | Perlin noise with configurable frequency, octaves, fadeoff, and seed. Accepts optional Gradient input |
| **Cells** | Voronoi cell pattern. Color mode: Gradient or Random |
| **Crystal** | Voronoi diagram with near/far coloring |
| **Bricks** | Brick/tile pattern with configurable size, fuge, and color variation |
| **Perlin Noise RG2** | Alternative Perlin noise with contrast and start octave controls |
| **Directional Gradient** | Spatial gradient between two points |
| **Glow Effect** | Radial glow centered at a point with falloff exponent |

### AGG Vector Drawing

All AGG nodes have an optional **Bg** input for compositing on a background texture.

| Node | Description |
|------|-------------|
| **Line** | Line segment with configurable endpoints, thickness, and color |
| **Circle** | Ellipse with independent fill and stroke (color, thickness) |
| **Rect** | Rectangle with optional rounded corners, fill and stroke |
| **Polygon** | Regular N-sided polygon. Inner radius < 1.0 creates star shapes |
| **Text** | Vector text using AGG's embedded font. Configurable size and thickness |

### Filters (modify input textures)

| Node | Inputs | Description |
|------|--------|-------------|
| **Blur** | In | Gaussian blur with configurable radius and order |
| **Blur Kernel** | In | Kernel blur (Box, Triangle, Gaussian) with wrap modes |
| **Color Matrix** | In | 4x4 color transform matrix |
| **Coord Matrix** | In | 4x4 coordinate transform (rotation, scale, tiling) |
| **Color Remap** | In, MapR, MapG, MapB | Remap each color channel through a lookup texture |
| **Coord Remap** | In, Remap | Distort coordinates using a displacement map |
| **Derive** | In | Compute gradient or normal map from input |
| **HSCB** | In | Hue, Saturation, Contrast, Brightness adjustment |
| **Color Balance** | In | Shadow/Midtone/Highlight color balance (3-way) |
| **Wavelet** | In | Wavelet transform (forward/inverse) |

### Combiners (merge multiple textures)

| Node | Inputs | Description |
|------|--------|-------------|
| **Paste** | Background, Snippet | Combine with blend op (Add, Sub, Multiply, Screen, etc.) |
| **Bump** | Surface, Normals, (Specular), (Falloff) | Bump/normal mapping with lighting |
| **Linear Combine** | Image1-4 | Weighted sum of up to 4 textures with UV shift |
| **Ternary** | Image1, Image2, Mask | Lerp or select between two textures |
| **Glow Rect** | Background, Gradient | Draw a glowing rectangle on a background |

### Utility

| Node | Description |
|------|-------------|
| **Output** | Save result to TGA file |
| **Comment** | Visual note for organizing pipelines |

## Export to C

Click **Export C Header** in the left panel to generate a standalone `.h` file. The generated header contains a single function that reproduces the entire node graph:

```c
#include "texgen.h"

static inline void my_texture_generate(GenTexture* out) {
    // ... pipeline steps in topological order ...
}
```

The exported code depends only on `libtexgen` (no raylib, no imgui). You can compile and link it against the static library:

```bash
g++ -std=c++17 -I lib -I work -I work/ktg -I agg/agg_lib/include \
    my_program.cpp -ltexgen -lagg -lm
```

## libtexgen

The core texture generation engine is available as a standalone library with no UI dependencies:

- `lib/texgen.h` — public header (includes gentexture, extra generators, AGG bridge, utilities)
- `build/lib/libtexgen.a` — static library
- `build/lib/libtexgen.so` — shared library

## Example Projects

See [examples/README.md](examples/README.md) for descriptions and a complete node reference.

| Example | Description |
|---------|-------------|
| 01_basic_shapes | Rect + Circle + Line composited via Bg inputs |
| 02_star_polygon | Polygon star + Crystal voronoi blend |
| 03_text_on_noise | Perlin noise background with AGG text overlay |
| 04_procedural_brick | Bricks generator + Blur filter |
| 05_crystal_glow | Crystal voronoi + GlowEffect blend |
| 06_badge_composition | Multi-layer badge: Circle + Star + Text |

## Libraries

All libraries are cloned automatically by `run.bash`:

| Library | Fork | Upstream |
|---------|------|----------|
| AGG 2.4 | [gwerners/agg](https://github.com/gwerners/agg) | [antigrain.com](http://www.antigrain.com) |
| fmt | [gwerners/fmt](https://github.com/gwerners/fmt) | [fmtlib/fmt](https://github.com/fmtlib/fmt) |
| stb | [gwerners/stb](https://github.com/gwerners/stb) | [nothings/stb](https://github.com/nothings/stb) |
| imgui (docking) | [gwerners/imgui](https://github.com/gwerners/imgui) | [ocornut/imgui](https://github.com/ocornut/imgui) |
| nlohmann/json | [gwerners/json](https://github.com/gwerners/json) | [nlohmann/json](https://github.com/nlohmann/json) |
| raylib | [gwerners/raylib](https://github.com/gwerners/raylib) | [raysan5/raylib](https://github.com/raysan5/raylib) |
| rlImGui | [gwerners/rlImGui](https://github.com/gwerners/rlImGui) | [raylib-extras/rlImGui](https://github.com/raylib-extras/rlImGui) |
| ImNodes | [gwerners/ImNodes](https://github.com/gwerners/ImNodes) | [rokups/ImNodes](https://github.com/rokups/ImNodes) |

## Project Structure

```
CMakeLists.txt          root cmake
run.bash                build and run script
run_tests.bash          export C test suite
clean.bash              cleanup script
examples/               example project JSONs + README
images/                 screenshots
res/                    fonts (FiraCode)
ref/                    KTG reference images

work/                   editor application source
  AggNodes.cpp/h        AGG vector drawing nodes (Line, Circle, Rect, Polygon, Text)
  AllNodes.cpp/h        procedural/filter/combiner node definitions
  CExport.cpp/h         C header export from node graph
  Nodes.cpp/h           node graph logic, undo/redo, copy/paste, minimap
  TextureNode.h         base class for texture nodes
  agg_gentexture.h      AGG <-> GenTexture zero-copy bridge
  Ide.cpp/h             imgui IDE layout, docking panels
  FileDialog.h          imgui file browser dialog
  ProjectIO.cpp/h       project save/load (JSON)
  Utils.cpp/h           utilities (matrix, image I/O)
  extra_generators.*    custom texture generators (Crystal, Bricks, etc.)
  ktg/                  gentexture library (Fabian Giesen)

lib/                    libtexgen standalone library (no UI deps)
  texgen.h              public umbrella header
  texgen_utils.h/cpp    pure utility functions
  CMakeLists.txt        builds libtexgen.a + libtexgen.so

tests/                  export C test infrastructure
  texgen_export.cpp     CLI tool: JSON -> C header
  test_template.cpp.in  test main template

agg/                    AGG 2.4 library (from github.com/gwerners/agg)
```

## License

TEXGEN is released under the [MIT License](LICENSE).

**Generated output is public domain.** Files produced by the "Export C Header"
feature carry no license restrictions — you may use them for any purpose without
attribution.

Third-party components carry their own permissive licenses:
- [gentexture](https://github.com/farbrausch/fr_public) (Fabian Giesen) — public domain
- [AGG 2.4](https://github.com/gwerners/agg) (Maxim Shemanarev) — permissive (copy/use/modify/sell with copyright notice)
