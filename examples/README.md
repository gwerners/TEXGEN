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

### Utility Nodes

| Node | Description |
|------|------------|
| **Output** | Saves result to TGA file |
| **Comment** | Visual note for organizing pipelines |
