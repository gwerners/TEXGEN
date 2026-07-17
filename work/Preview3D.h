#pragma once
// Preview3D — GPU-lit 3D preview of the Material node's PBR maps.
// Renders a sphere/cube/plane with a tangent-space PBR shader (normal
// mapping, roughness/metallic specular, AO, emission, fake environment
// reflection and height displacement) into a RenderTexture shown in
// the Bottom Panel. Editor-only eye candy: the deterministic CPU lib
// remains the reference for runtime output.

#include <raylib.h>

class NodeGraph;

class Preview3D {
 public:
  // Draws the 3D preview ImGui content (viewport + controls).
  void draw(NodeGraph* graph);
  void unload();

 private:
  void ensureInit();
  void updateTextures(NodeGraph* graph);
  void setTexture(int slot, Texture2D tex);

  bool m_init = false;
  RenderTexture2D m_rt{};
  Model m_models[3]{};
  Shader m_shader{};

  // maps: 0 albedo, 1 normal, 2 roughness, 3 metallic, 4 height,
  //       5 ao, 6 emission
  Texture2D m_tex[7]{};
  bool m_has[7] = {};

  int m_shape = 0;  // 0 sphere, 1 cube, 2 plane
  float m_yaw = 0.7f, m_pitch = 0.35f, m_dist = 2.4f;
  bool m_autoRotate = true;
  float m_displace = 0.06f;
  float m_tiling = 1.0f;
  double m_lastUpload = -1.0;
  int m_lastChangeCount = -1;

  // shader uniform locations
  int m_locHas[7] = {};
  int m_locLightDir = -1, m_locViewPos = -1, m_locDisplace = -1,
      m_locTiling = -1;
};
