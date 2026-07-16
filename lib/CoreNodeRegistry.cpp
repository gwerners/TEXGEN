#include "CoreNodeRegistry.h"

#include "AggCoreNodes.h"
#include "CoreNodes.h"
#include "GraphCoreNodes.h"
#include "MMCoreNodes.h"

void CoreNodeRegistry::add(const std::string &typeName,
                           CoreNodeFactory factory) {
  m_factories[typeName] = std::move(factory);
}

std::unique_ptr<CoreNode>
CoreNodeRegistry::create(const std::string &typeName) const {
  auto it = m_factories.find(typeName);
  if (it != m_factories.end())
    return it->second();
  return nullptr;
}

bool CoreNodeRegistry::has(const std::string &typeName) const {
  return m_factories.count(typeName) > 0;
}

template <typename T> static void registerType(CoreNodeRegistry &reg) {
  reg.add(T().typeName(), [] { return std::make_unique<T>(); });
}

const std::vector<const char *> &nodeCategoryOrder() {
  static const std::vector<const char *> order = {
      "Generator", "Vector", "SDF",       "Filter",
      "Combine",   "Material", "Structure", "Output"};
  return order;
}

const NodeMeta *getNodeMeta(const std::string &typeName) {
  static const std::map<std::string, NodeMeta> meta = {
      // Generators
      {"Color", {"Generator", "Uniform color fill"}},
      {"Gradient", {"Generator", "Multi-stop gradient generator"}},
      {"Image", {"Generator", "Loads an image file from disk"}},
      {"Noise", {"Generator", "Value noise with octaves and modes"}},
      {"Cells", {"Generator", "Cellular pattern generator"}},
      {"Crystal", {"Generator", "Faceted crystal-like pattern"}},
      {"Bricks", {"Generator", "Classic brick pattern generator"}},
      {"BricksMM",
       {"Generator", "Brick pattern with bevel, mortar and randomness"}},
      {"PerlinNoiseRG2", {"Generator", "Two-channel (RG) perlin noise"}},
      {"DirectionalGradient", {"Generator", "Linear gradient at an angle"}},
      {"GlowEffect", {"Generator", "Glowing ellipse/ring generator"}},
      {"Voronoi",
       {"Generator", "Voronoi cell noise with distance and color outputs"}},
      {"FBM", {"Generator", "Fractal Brownian Motion noise (octaves)"}},
      {"Shape", {"Generator", "Parametric shape (polygon, star, arc...)"}},
      {"Pattern",
       {"Generator", "Procedural wave patterns (sine, triangle, square...)"}},
      // Vector drawing (AGG)
      {"AggLine", {"Vector", "Draws an anti-aliased line"}},
      {"AggCircle", {"Vector", "Draws an anti-aliased circle"}},
      {"AggRect", {"Vector", "Draws an anti-aliased rectangle"}},
      {"AggPolygon", {"Vector", "Draws an anti-aliased polygon"}},
      {"AggText", {"Vector", "Renders vector text"}},
      {"AggArc", {"Vector", "Draws an anti-aliased arc"}},
      {"AggBezier", {"Vector", "Draws a bezier curve"}},
      {"AggDashLine", {"Vector", "Draws a dashed line"}},
      {"AggGradient", {"Vector", "Draws a vector gradient shape"}},
      // SDF
      {"SdfShape",
       {"SDF", "Signed distance field primitive (circle, box, line...)"}},
      {"SdfOp",
       {"SDF", "Boolean/smooth operations on SDFs (union, subtract...)"}},
      {"SdfTransform", {"SDF", "Translates, rotates and scales an SDF"}},
      {"SdfShow", {"SDF", "Renders an SDF to a grayscale image"}},
      // Filters
      {"Blur", {"Filter", "Directional box blur with multiple passes"}},
      {"BlurKernel", {"Filter", "Convolution blur with custom kernel"}},
      {"ColorMatrix", {"Filter", "4x4 color matrix transform"}},
      {"CoordMatrix", {"Filter", "2D coordinate/UV matrix transform"}},
      {"ColorRemap", {"Filter", "Remaps colors through gradient inputs"}},
      {"CoordRemap", {"Filter", "Remaps coordinates using an input map"}},
      {"Derive", {"Filter", "Derivative (gradient/normal) of a heightmap"}},
      {"HSCB", {"Filter", "Hue, saturation, contrast and brightness"}},
      {"ColorBalance", {"Filter", "Per-channel color balance"}},
      {"Wavelet", {"Filter", "Wavelet-based distortion"}},
      {"Colorize", {"Filter", "Maps grayscale values through a gradient"}},
      {"Warp",
       {"Filter", "Distorts the input using a heightmap as displacement"}},
      {"MakeTileable", {"Filter", "Makes the input seamlessly tileable"}},
      {"Quantize", {"Filter", "Reduces color values to discrete steps"}},
      {"Emboss", {"Filter", "Emboss/relief effect from a heightmap"}},
      {"Transform2D",
       {"Filter", "Translate, rotate and scale with tiling control"}},
      {"Invert", {"Filter", "Inverts colors (1 - value)"}},
      {"NormalMap", {"Filter", "Generates a normal map from a heightmap"}},
      // Combiners
      {"Blend",
       {"Combine", "Blends two inputs with selectable mode and opacity"}},
      {"Paste", {"Combine", "Pastes one texture onto another"}},
      {"Bump", {"Combine", "Bump-lights a texture with a normal map"}},
      {"LinearCombine", {"Combine", "Weighted sum of multiple inputs"}},
      {"Ternary", {"Combine", "Per-pixel conditional select between inputs"}},
      {"GlowRect", {"Combine", "Draws a glowing rectangle over a background"}},
      {"Combine",
       {"Combine", "Combines up to 4 grayscale channels into RGBA"}},
      {"Decompose",
       {"Combine", "Splits RGBA into separate grayscale channels"}},
      // Material
      {"Material",
       {"Material", "PBR material: albedo, normal, roughness, metallic..."}},
      {"LayerMix",
       {"Material", "Mixes two material layers by height (hard or smooth)"}},
      {"WorkflowOutput",
       {"Material", "Unpacks a material bundle into PBR maps"}},
      // Structure
      {"Subgraph",
       {"Structure", "Nested node graph with exposed input/output ports"}},
      {"Remote", {"Structure", "Remote-controls parameters of other nodes"}},
      {"Comment", {"Structure", "Text note in the graph"}},
      // Output
      {"Output", {"Output", "Marks the graph result shown in the preview"}},
  };
  auto it = meta.find(typeName);
  return it != meta.end() ? &it->second : nullptr;
}

CoreNodeRegistry &getCoreNodeRegistry() {
  static CoreNodeRegistry reg = [] {
    CoreNodeRegistry r;
    // Sources
    registerType<ColorCoreNode>(r);
    registerType<GradientCoreNode>(r);
    registerType<ImageCoreNode>(r);
    // Procedural generators
    registerType<NoiseCoreNode>(r);
    registerType<CellsCoreNode>(r);
    registerType<CrystalCoreNode>(r);
    registerType<BricksCoreNode>(r);
    registerType<PerlinNoiseRG2CoreNode>(r);
    registerType<DirectionalGradientCoreNode>(r);
    registerType<GlowEffectCoreNode>(r);
    // AGG vector nodes
    registerType<AggLineCoreNode>(r);
    registerType<AggCircleCoreNode>(r);
    registerType<AggRectCoreNode>(r);
    registerType<AggPolygonCoreNode>(r);
    registerType<AggTextCoreNode>(r);
    registerType<AggArcCoreNode>(r);
    registerType<AggBezierCoreNode>(r);
    registerType<AggDashLineCoreNode>(r);
    registerType<AggGradientCoreNode>(r);
    // Filters
    registerType<BlurCoreNode>(r);
    registerType<BlurKernelCoreNode>(r);
    registerType<ColorMatrixCoreNode>(r);
    registerType<CoordMatrixCoreNode>(r);
    registerType<ColorRemapCoreNode>(r);
    registerType<CoordRemapCoreNode>(r);
    registerType<DeriveCoreNode>(r);
    registerType<HSCBCoreNode>(r);
    registerType<ColorBalanceCoreNode>(r);
    registerType<WaveletCoreNode>(r);
    // Combiners
    registerType<PasteCoreNode>(r);
    registerType<BumpCoreNode>(r);
    registerType<LinearCombineCoreNode>(r);
    registerType<TernaryCoreNode>(r);
    registerType<GlowRectCoreNode>(r);
    // Material Maker ports
    registerType<VoronoiCoreNode>(r);
    registerType<FBMCoreNode>(r);
    registerType<BlendCoreNode>(r);
    registerType<WarpCoreNode>(r);
    registerType<ColorizeCoreNode>(r);
    registerType<MMBricksCoreNode>(r);
    registerType<MaterialCoreNode>(r);
    registerType<NormalMapCoreNode>(r);
    registerType<SdfShapeCoreNode>(r);
    registerType<SdfOpCoreNode>(r);
    registerType<SdfTransformCoreNode>(r);
    registerType<SdfShowCoreNode>(r);
    registerType<MakeTileableCoreNode>(r);
    registerType<QuantizeCoreNode>(r);
    registerType<EmbossCoreNode>(r);
    registerType<Transform2DCoreNode>(r);
    registerType<ShapeCoreNode>(r);
    registerType<PatternCoreNode>(r);
    registerType<CombineCoreNode>(r);
    registerType<DecomposeCoreNode>(r);
    registerType<InvertCoreNode>(r);
    registerType<LayerMixCoreNode>(r);
    registerType<WorkflowOutputCoreNode>(r);
    // Structural
    registerType<SubgraphCoreNode>(r);
    registerType<RemoteCoreNode>(r);
    // Utility
    registerType<OutputCoreNode>(r);
    registerType<CommentCoreNode>(r);
    return r;
  }();
  return reg;
}
