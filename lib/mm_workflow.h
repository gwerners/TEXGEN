#pragma once
// mm_workflow — Material Maker layered-material workflow (mwf_*) ports.
// A material travels as a 5-channel bundle: Height (grayscale), Albedo,
// ORM (occlusion/roughness/metallic packed in RGB), Emission, Normal.
// Ported from material-maker (MIT, see THIRD_PARTY.md): mwf_mix.mmg,
// mwf_mix_smooth.mmg and mwf_output.mmg.

#include "gentexture.hpp"

// Height-based mix of two material layers (mwf_mix / mwf_mix_smooth).
// l1/l2 point to {H, C, ORM, EM, NM}; null entries use defaults (H=0,
// C/ORM/EM=black, NM=flat normal). mode 0 selects the layer with the
// highest height per pixel; mode 1 blends with
// smoothstep((h2-h1)/width). outs receives {H=max(h1,h2), C, ORM, EM,
// NM}; entries must be Init'd by the caller to a common size.
void MMLayerMix(GenTexture *outs[5], const GenTexture *l1[5],
                const GenTexture *l2[5], sInt mode, sF32 width);

// Unpacks a material bundle into PBR maps (mwf_output). Metallic and
// roughness come from ORM.b / ORM.g. The normal output combines the
// height-derived normal with the bundle normal weighted by matNormal.
// Occlusion is a blur-based approximation from the heightmap scaled by
// occStrength (MM computes AO in a shader); depth = 1 - height.
void MMWorkflowOutput(GenTexture &albedo, GenTexture &metallic,
                      GenTexture &roughness, GenTexture &emission,
                      GenTexture &normal, GenTexture &occlusion,
                      GenTexture &depth, const GenTexture *height,
                      const GenTexture *albedoIn, const GenTexture *orm,
                      const GenTexture *emissionIn,
                      const GenTexture *normalIn, sF32 matNormal,
                      sF32 occStrength);

// Placement map for the workflow scatter (mwf_create_map.mmg):
// rgb = (height * h(uv), angle packed into [0,1], rand(seed + o(uv))).
void MMCreateMap(GenTexture &out, const GenTexture *h, const GenTexture *o,
                 sF32 height, sF32 angleDeg, sF32 seed);

// Applies a placement map to a material bundle (mwf_map.mmg): each
// pixel samples the four material channels at a rotated/offset UV
// driven by the map (y = angle, z = random offset); the normal map is
// counter-rotated. outH receives the map height (x channel).
void MMMatMap(GenTexture &outH, GenTexture &outC, GenTexture &outORM,
              GenTexture &outEM, GenTexture &outNM, const GenTexture &map,
              const GenTexture *mat1, const GenTexture *mat2,
              const GenTexture *mat3, const GenTexture *mat4);
