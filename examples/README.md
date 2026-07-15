# TEXGEN Examples

Example projects demonstrating the node-based texture generation pipeline.
Open these `.json` files from the TEXGEN editor via **File > Load**.

## Examples

### 01 - Basic Shapes
**File:** `01_basic_shapes.json`

Demonstrates the three fundamental AGG drawing nodes chained together via
background input:

- **Rect** node with rounded corners (fill + stroke) as the base
- **Circle** node drawn on top
- **Line** node drawn on top of everything

Shows how AGG nodes can be composited by connecting the output of one node to
the "Bg" input of the next.

### 02 - Star Polygon
**File:** `02_star_polygon.json`

A five-pointed star using the **Polygon** node with `innerRadius < 1.0`.
Demonstrates:

- Regular polygon with N sides
- Inner radius ratio to create star shapes
- Combined fill and stroke rendering

### 03 - Text on Noise
**File:** `03_text_on_noise.json`

Procedural Perlin noise as a background with vector text overlaid:

- **Noise** node generates a tileable base texture
- **Text** node renders "TEXGEN" on top using AGG's embedded vector font
- Demonstrates connecting a procedural generator to an AGG node's Bg input

### 04 - Procedural Brick
**File:** `04_procedural_brick.json`

A classic procedural brick texture pipeline:

- **Bricks** node generates the tile pattern
- **Blur** node softens the edges

### 05 - Crystal Glow
**File:** `05_crystal_glow.json`

Voronoi crystal pattern with a glow effect:

- **Crystal** node generates distance-based Voronoi cells
- **GlowEffect** node adds a radial glow highlight

### 06 - Badge Composition
**File:** `06_badge_composition.json`

A multi-layer vector badge built entirely from AGG nodes:

- **Circle** (filled blue with gold stroke) as the base
- **Polygon** (5-point star) centered on top
- **Text** ("AGG") rendered in the center

Demonstrates the full compositing workflow: each AGG node receives the previous
node's output as its background.

### 07 - Bezier Curves
**File:** `07_bezier_curves.json`

Two cubic Bezier curves layered with different colors:

- Two **Bezier** nodes with mirrored control points
- Demonstrates S-curve shapes with 4-point control

### 08 - Dashed Arc
**File:** `08_dashed_arc.json`

Arc and dashed line primitives:

- **Arc** node drawing a partial ellipse (30-330 degrees)
- **DashLine** node drawn on top with configurable dash/gap length

### 09 - Gradient Fill
**File:** `09_gradient_fill.json`

Radial gradient background with a vector overlay:

- **GradientFill** node with radial type (warm center to dark edge)
- **Polygon** (6-point star, stroke only) drawn on top

### 10 - FBM + Warp + Colorize (lava)
**File:** `10_fbm_warp_colorize.json`

Material Maker-style lava material, exported as PNG:

- **FBM** (cellular, 1 fold) as the base pattern
- **Colorize** mapping it through a 4-stop lava ramp
- **Warp** displacing it along a second **FBM** (perlin) height map

### 11 - Voronoi + Blend + Bricks v2 (herringbone)
**File:** `11_voronoi_blend_bricks.json`

Herringbone brick wall with procedural variation:

- **BricksMM** with the herringbone pattern
- **Voronoi** edge distance multiplied on top via **Blend**
  (multiply mode, masked by the Voronoi F1 output)

### 12 - PBR Material Export
**File:** `12_pbr_material.json`

Full PBR material: one graph, four texture maps on disk:

- **BricksMM** (colored) feeds the **Material** node's Albedo channel
- A grayscale **BricksMM** drives Height, **NormalMap** and (via
  **Colorize**) Roughness
- The **Material** node saves `brick_pbr_albedo.png`, `_normal.png`,
  `_height.png` and `_roughness.png`

### 13 - SDF Badge
**File:** `13_sdf_badge.json`

Signed-distance-field composition:

- **SdfShape** star and circle combined with **SdfOp**
  (smooth subtraction)
- **SdfTransform** annular ring, **SdfShow** to grayscale,
  **Colorize** golden ramp

### 14 - Subgraph + Remote
**File:** `14_subgraph_remote.json`

Structural nodes:

- A **Subgraph** ("LavaGen") encapsulating an FBM → Colorize chain,
  exposed as a single node with an `Out` port
- A **Remote** node whose sliders drive the height FBM's `scaleX` and
  the **Warp** amount (values are applied before evaluation)
- In the editor, select nodes and right-click → *Group Selected* /
  *Ungroup Selected*

---

## Node Reference

### Source Nodes (no inputs required)

| Node | Description |
|------|------------|
| **Color** | Solid color fill with configurable size |
| **Noise** | Perlin noise with octaves, gradient coloring |
| **Cells** | Voronoi cells with gradient or random colors |
| **Crystal** | Voronoi distance map with near/far color blend |
| **Bricks** | Brick/tile pattern generator |
| **Gradient** | Two-color gradient (2px lookup table) |
| **DirectionalGradient** | Linear gradient between two points |
| **GlowEffect** | Radial glow with falloff |
| **PerlinNoiseRG2** | Alternative Perlin implementation |
| **Image** | Loads PNG/JPG/TGA/BMP/PSD/GIF/HDR files |

### AGG Vector Drawing Nodes

All AGG nodes have an optional **Bg** input for compositing on a background.

| Node | Description |
|------|------------|
| **Line** | Line segment with configurable endpoints and thickness |
| **Circle** | Ellipse with independent fill and stroke |
| **Rect** | Rectangle with optional rounded corners |
| **Polygon** | Regular N-sided polygon; inner radius < 1.0 creates stars |
| **Text** | Vector text using AGG's embedded font |
| **Arc** | Partial ellipse arc with start/end angles |
| **Bezier** | Cubic Bezier curve with 4 control points |
| **DashLine** | Dashed line with configurable dash/gap length |
| **GradientFill** | Linear or radial gradient fill |

### Filter Nodes (require input)

| Node | Description |
|------|------------|
| **Blur** | Gaussian blur |
| **BlurKernel** | Box/Triangle/Gaussian kernel blur |
| **ColorMatrix** | 4x4 color transform |
| **CoordMatrix** | 4x4 coordinate transform (rotate, scale, tile) |
| **ColorRemap** | Remap channels through lookup textures |
| **CoordRemap** | Distort coordinates via displacement map |
| **Derive** | Generate gradient or normal map |
| **HSCB** | Hue, Saturation, Contrast, Brightness |
| **ColorBalance** | 3-way color balance |
| **Wavelet** | Forward/inverse wavelet transform |

### Combiner Nodes (multiple inputs)

| Node | Description |
|------|------------|
| **Paste** | Composite with blend modes (Add/Sub/Mul/Screen/Over/etc.) |
| **Bump** | Bump/normal mapping with lighting |
| **LinearCombine** | Weighted sum of up to 4 inputs |
| **Ternary** | Lerp or select between two textures via mask |
| **GlowRect** | Draw glowing rectangle on background |

### Material Maker Ports

| Node | Description |
|------|------------|
| **Voronoi** | Tileable Voronoi with Color / F1 / Edge outputs |
| **FBM** | Fractal noise (value/perlin/cellular variants, folds) |
| **Blend** | 15 Photoshop-style blend modes + mask + opacity |
| **Warp** | Displace UVs along a height map gradient |
| **Colorize** | Multi-stop gradient map |
| **BricksMM** | Brick layouts: running bond, herringbone, basket weave, spanish bond |
| **Material** | PBR sink: saves each connected channel as `<base>_<channel>.png` |
| **NormalMap** | Height to tangent-space normal map (OpenGL/DirectX) |
| **SdfShape** | 2D signed distance shapes: circle, box, line, star, n-gon, rhombus |
| **SdfOp** | SDF booleans (union/subtract/intersect), smooth variants, morph |
| **SdfTransform** | SDF translate/rotate/scale, rounding, annular rings |
| **SdfShow** | SDF to grayscale image with bevel |
| **MakeTileable** | Hides seams by blending offset copies |
| **Quantize** | Posterize RGB into N steps |
| **Emboss** | Directional relief from a grayscale input |

### Structural Nodes

| Node | Description |
|------|------------|
| **Subgraph** | A nested graph exposed as one node (Group/Ungroup in the editor) |
| **Remote** | Sliders that drive parameters of other nodes |

### Utility Nodes

| Node | Description |
|------|------------|
| **Output** | Saves result to TGA or PNG file (by extension) |
| **Comment** | Visual note for organizing pipelines |
