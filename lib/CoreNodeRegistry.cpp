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
