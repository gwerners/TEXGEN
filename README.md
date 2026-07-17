# TEXGEN

Node-based procedural texture and PBR material generator for Linux. Connect generator, filter, and vector drawing nodes in a visual graph to create tileable textures and complete PBR materials — all in real time, with 2D and 3D preview.

Built with C++20, raylib, Dear ImGui, and [AGG 2.4](https://github.com/gwerners/agg) for anti-aliased vector drawing.

Texture generation core based on [gentexture](https://github.com/farbrausch/fr_public) by Fabian Giesen (public domain). Many nodes are CPU ports of [Material Maker](https://github.com/RodZill4/material-maker) algorithms (MIT) — see [THIRD_PARTY.md](THIRD_PARTY.md).

## Screenshot

<p align="center">
  <img src="images/screen.png" alt="TEXGEN node editor" width="100%"/>
</p>

## Features

- **100+ node types** — generators, filters, combiners, SDF 2D, AGG vector drawing, and structure nodes, all searchable from the in-editor menu with tooltips
- **PBR materials** — the Material node collects albedo, normal, roughness, metallic, depth, AO, and emission maps, exports them as PNGs, and renders a lit preview
- **3D preview** — sphere/cube/plane on the GPU with normal mapping, height displacement, specular/Fresnel, and configurable texture tiling
- **Material Maker import** — open `.ptex` projects directly; the graph is converted automatically (library macros are expanded, unsupported nodes are pruned gracefully)
- **Material library** — folder browser with thumbnails, built in parallel by background workers with progress reporting
- **Incremental evaluation** — editing a parameter re-runs only the affected node and its downstream cone, streamed from a worker thread so the UI never blocks
- **Subgraphs** — group nodes into reusable graphs with exposed parameters
- **Export to C** — generate a standalone `.h` header that reproduces the pipeline bit-exactly with `libtexgen` calls, deterministic and UI-free (ready for game runtimes)
- **AGG 2.4 vector nodes** — Line, Circle, Rect, Polygon, Text, Arc, Bezier with anti-aliasing
- **Undo/Redo, Copy/Paste, minimap, 16-bit RGBA precision**

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
./clean.bash        # remove everything (deps + build + generated images)
./clean.bash build  # remove build products only
```

## Test Suite

Export C headers from all example projects, compile, and run them standalone (no raylib dependency):

```bash
./run_tests.bash
```

## CLI Tools

Built alongside the editor in `build/tests/`:

| Tool | Description |
|------|-------------|
| `texgen_render <p.json\|p.ptex> [out.tga]` | Render a project headless (Material Maker projects are converted on the fly) |
| `texgen_export <p.json\|p.ptex> <func> <out.h>` | Export a project to a standalone C header |
| `texgen_debug <p.json\|p.ptex> <dir>` | Dump every node's output as PNG (fidelity debugging) plus the converted graph |

## Keyboard Shortcuts

### Canvas

| Shortcut | Action |
|----------|--------|
| Right-click | Open context menu (create nodes, searchable) |
| Middle-drag | Pan canvas |
| Scroll wheel | Zoom in/out |
| P | Toggle all node previews |
| Delete | Remove selected nodes |
| Ctrl+Z / Ctrl+Y | Undo / Redo (up to 50 levels) |
| Ctrl+C / Ctrl+V | Copy / paste nodes with connections |

### Sliders

| Shortcut | Action |
|----------|--------|
| Scroll wheel (hover) | Increment/decrement value |
| Arrow Up/Down (hover) | Increment/decrement value by one step |
| Ctrl+Click | Type an exact value (can exceed min/max range) |

## Interface

- **Left Panel** — project save/load, Material Maker import, Export C header, material library with thumbnails
- **Bottom Panel** — output preview with zoom controls, or the 3D material preview (sphere/cube/plane, orbit/zoom/spin, height and tiling sliders)
- **Canvas** — node graph with minimap; the hint bar at the bottom shows contextual help and background task status
- **Context menu** — searchable node creation menu organized by category, with tooltips

## Nodes

Node types by category (the in-editor menu lists all of them with descriptions):

| Category | Examples |
|----------|----------|
| **Generator** | Voronoi (F1/edge/color/fill), FBM (perlin/cellular variants), Bricks, Weave, Crystal, Scratches, Shape, Sphere, DotNoise, ColorNoise, WaveletNoise, AnisotropicNoise, Gradient (linear/radial/circular), Pattern |
| **Vector (AGG)** | Line, Circle, Rect, Polygon, Text, Arc, Bezier, DashLine, Gradient shapes |
| **SDF** | Shape primitives, boolean/smooth ops, transforms, render |
| **Filter** | Blur, DirectionalBlur, SlopeBlur, Warp, MultiWarp, Bevel, Dilate, EdgeDetect, Emboss, Colorize, Levels, Transform2D, Tiler, TilerAdvanced, AddTiler, NormalMap, NormalBlend, Fill family (region UVs, random colors, gradients, sizes), AmbientOcclusion, MakeTileable, CustomUV, Mirror, Remap |
| **Combine** | Blend (15 modes), MathOp (37 ops), SmoothMinMax, Combine/Decompose, Tile2x2, LayerMix |
| **Material** | Material (PBR multi-map + lit preview), WorkflowOutput |
| **Structure** | Subgraph, Remote, Comment, Output |

## Material Maker Import

TEXGEN opens [Material Maker](https://github.com/RodZill4/material-maker) `.ptex` projects via the import button in the toolbar (or by passing them to the CLI tools). The converter maps MM nodes to native TEXGEN nodes, expands library graph macros, collapses passthroughs, and prunes unsupported chains so the rest of the material still renders. Author-written custom `shader` nodes cannot be converted automatically.

## Export to C

Click **Export C Header** in the left panel (or use `texgen_export`) to generate a standalone `.h` file. The generated header contains a single function that reproduces the entire node graph bit-exactly:

```c
#include "texgen.h"

static inline void my_texture_generate(GenTexture* out) {
    // ... pipeline steps in topological order ...
}
```

The exported code depends only on `libtexgen` (no raylib, no imgui):

```bash
g++ -std=c++17 -I lib -I work -I work/ktg -I agg/agg_lib/include \
    my_program.cpp libtexgen.a libagg.a -lm
```

## libtexgen

The core texture generation engine is a standalone library with no UI dependencies — this is what a game links to generate textures deterministically at runtime:

- `lib/texgen.h` — public umbrella header
- `build/lib/libtexgen.a` — static library
- `build/lib/libtexgen.so` — shared library

## Example Projects

`examples/` ships 40 projects: classic pipelines (shapes, badges, noise compositions) plus 22 curated Material Maker ports (`mm_*.json`) covering bricks, metals, stones, and organic materials. See [examples/README.md](examples/README.md).

## Libraries

Cloned automatically by `run.bash`:

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

Vendored in-tree: [NanoSVG](https://github.com/memononen/nanosvg) (`nanosvg/`, zlib license) for the toolbar/menu icons.

## Project Structure

```
CMakeLists.txt          root cmake
run.bash                build and run script
run_tests.bash          export C test suite
clean.bash              cleanup script
examples/               example project JSONs + README
images/                 screenshots
res/                    fonts (FiraCode)
docs/                   design notes (Material Maker study)
nanosvg/                vendored NanoSVG (icons)
tools/                  helper scripts

lib/                    libtexgen standalone library (no UI deps)
  texgen.h              public umbrella header
  CoreNodes.*           classic generator/filter core nodes
  MMCoreNodes.*         Material Maker port core nodes
  AggCoreNodes.*        AGG vector core nodes
  GraphCoreNodes.*      subgraph/structure core nodes
  CoreNodeRegistry.*    node factory + menu metadata (single source)
  mm_generators.*       MM generator algorithms (CPU ports)
  mm_filters.*          MM filter algorithms
  mm_fill.*             MM fill family (exact connected components)
  mm_sdf.*              SDF 2D
  mm_workflow.*         MM workflow layers (LayerMix etc.)
  PtexImport.*          .ptex -> TEXGEN graph converter
  HeadlessEval.*        UI-free graph evaluation

work/                   editor application source
  Ide.cpp/h             imgui IDE layout, docking panels, hint bar
  Nodes.cpp/h           node graph logic, undo/redo, copy/paste, minimap
  AllNodes/MMNodes/AggNodes/StructNodes  UI wrappers over core nodes
  CExport.cpp/h         C header export from node graph
  GraphRunner.*         worker-thread incremental evaluation
  Preview3D.*           GPU 3D material preview
  Library.*             material library with thumbnail batch
  ProjectIO.cpp/h       project save/load, MM import
  Icons.*               SVG icon rasterization
  ktg/                  gentexture library (Fabian Giesen)

tests/                  CLI tools + test infrastructure
  texgen_render.cpp     headless render (json/ptex)
  texgen_export.cpp     CLI: json/ptex -> C header
  texgen_debug.cpp      per-node output dump
  test_template.cpp.in  test main template
```

## License

TEXGEN is released under the [MIT License](LICENSE).

**Generated output is public domain.** Files produced by the "Export C Header"
feature carry no license restrictions — you may use them for any purpose without
attribution.

Third-party components carry their own permissive licenses — see [THIRD_PARTY.md](THIRD_PARTY.md):
- [gentexture](https://github.com/farbrausch/fr_public) (Fabian Giesen) — public domain
- [AGG 2.4](https://github.com/gwerners/agg) (Maxim Shemanarev) — permissive (copy/use/modify/sell with copyright notice)
- [Material Maker](https://github.com/RodZill4/material-maker) (Rodolphe Suescun) — MIT (algorithm ports)
- [NanoSVG](https://github.com/memononen/nanosvg) (Mikko Mononen) — zlib
