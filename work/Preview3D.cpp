#include "Preview3D.h"
#include "Nodes.h"
#include "Utils.h"

#include <imgui.h>
#include <rlImGui.h>

#include <cmath>

namespace {

const int RT_SIZE = 512;

const char* kVertexShader = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform sampler2D heightMap;
uniform int hasHeight;
uniform float displace;
uniform vec2 tiling;
out vec2 fragUV;
out vec3 fragPos;
out vec3 fragNormal;
out vec4 fragTangent;
void main() {
    fragUV = vertexTexCoord;
    vec3 pos = vertexPosition;
    if (hasHeight == 1) {
        // depth map (MM convention): white pushes the surface in
        float d = texture(heightMap, vertexTexCoord * tiling).r;
        pos += vertexNormal * (0.5 - d) * displace;
    }
    fragPos = vec3(matModel * vec4(pos, 1.0));
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragTangent = vec4(normalize(vec3(matModel * vec4(vertexTangent.xyz, 0.0))),
                       vertexTangent.w);
    gl_Position = mvp * vec4(pos, 1.0);
}
)GLSL";

const char* kFragmentShader = R"GLSL(
#version 330
in vec2 fragUV;
in vec3 fragPos;
in vec3 fragNormal;
in vec4 fragTangent;
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D roughMap;
uniform sampler2D metalMap;
uniform sampler2D aoMap;
uniform sampler2D emissionMap;
uniform int hasAlbedo;
uniform int hasNormal;
uniform int hasRough;
uniform int hasMetal;
uniform int hasAO;
uniform int hasEmission;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec2 tiling;
out vec4 finalColor;

void main() {
    vec2 uv = fragUV * tiling;
    vec3 albedo = hasAlbedo == 1 ? texture(albedoMap, uv).rgb : vec3(1.0);
    float rough = hasRough == 1 ? texture(roughMap, uv).r : 1.0;
    float metal = hasMetal == 1 ? texture(metalMap, uv).r : 0.0;
    float ao    = hasAO == 1 ? texture(aoMap, uv).r : 1.0;

    vec3 N = normalize(fragNormal);
    if (hasNormal == 1) {
        vec3 T = normalize(fragTangent.xyz - N * dot(fragTangent.xyz, N));
        vec3 B = cross(N, T) * fragTangent.w;
        vec3 nm = texture(normalMap, uv).rgb * 2.0 - 1.0;
        N = normalize(mat3(T, B, N) * nm);
    }
    vec3 V = normalize(viewPos - fragPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);
    float ndl = max(dot(N, L), 0.0);
    float ndh = max(dot(N, H), 0.0);
    float ndv = max(dot(N, V), 0.0);

    // Blinn-Phong with roughness-driven exponent, metallic F0 tint
    float shininess = 4.0 + 508.0 * (1.0 - rough) * (1.0 - rough);
    float specPow = pow(ndh, shininess) * (8.0 + shininess) / 25.0;
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    // Schlick fresnel boosts grazing reflections
    vec3 F = F0 + (vec3(1.0) - F0) * pow(1.0 - ndv, 5.0);

    // fake sky/ground environment: reflection color from the reflected
    // vector, ambient from the normal
    vec3 R = reflect(-V, N);
    vec3 skyTop = vec3(0.75, 0.80, 0.90);
    vec3 skyBottom = vec3(0.18, 0.16, 0.14);
    vec3 envRefl = mix(skyBottom, skyTop, clamp(R.y * 0.5 + 0.5, 0.0, 1.0));
    vec3 ambient = mix(skyBottom, skyTop, clamp(N.y * 0.5 + 0.5, 0.0, 1.0));

    vec3 diffuse = albedo * (1.0 - metal);
    vec3 lightCol = vec3(1.05, 1.0, 0.95);

    vec3 col = diffuse * (0.35 * ambient + lightCol * ndl) * ao;
    col += F * envRefl * (1.0 - rough) * (0.25 + 0.75 * metal) * ao;
    col += F0 * specPow * lightCol * ndl;
    if (hasEmission == 1)
        col += texture(emissionMap, uv).rgb;

    // tonemap + gamma
    col = col / (col + vec3(1.0));
    col = pow(col, vec3(1.0 / 2.2));
    finalColor = vec4(col, 1.0);
}
)GLSL";

}  // namespace

void Preview3D::ensureInit() {
  if (m_init)
    return;
  m_init = true;

  m_rt = LoadRenderTexture(RT_SIZE, RT_SIZE);

  Mesh sphere = GenMeshSphere(1.0f, 64, 96);
  Mesh cube = GenMeshCube(1.5f, 1.5f, 1.5f);
  Mesh plane = GenMeshPlane(2.2f, 2.2f, 64, 64);
  GenMeshTangents(&sphere);
  GenMeshTangents(&cube);
  GenMeshTangents(&plane);
  m_models[0] = LoadModelFromMesh(sphere);
  m_models[1] = LoadModelFromMesh(cube);
  m_models[2] = LoadModelFromMesh(plane);

  m_shader = LoadShaderFromMemory(kVertexShader, kFragmentShader);
  static const char* hasNames[7] = {"hasAlbedo",  "hasNormal", "hasRough",
                                    "hasMetal",   "hasHeight", "hasAO",
                                    "hasEmission"};
  for (int i = 0; i < 7; i++)
    m_locHas[i] = GetShaderLocation(m_shader, hasNames[i]);
  m_locLightDir = GetShaderLocation(m_shader, "lightDir");
  m_locViewPos = GetShaderLocation(m_shader, "viewPos");
  m_locDisplace = GetShaderLocation(m_shader, "displace");
  m_locTiling = GetShaderLocation(m_shader, "tiling");

  // sampler binding through raylib material maps
  m_shader.locs[SHADER_LOC_MAP_ALBEDO] =
      GetShaderLocation(m_shader, "albedoMap");
  m_shader.locs[SHADER_LOC_MAP_NORMAL] =
      GetShaderLocation(m_shader, "normalMap");
  m_shader.locs[SHADER_LOC_MAP_ROUGHNESS] =
      GetShaderLocation(m_shader, "roughMap");
  m_shader.locs[SHADER_LOC_MAP_METALNESS] =
      GetShaderLocation(m_shader, "metalMap");
  m_shader.locs[SHADER_LOC_MAP_OCCLUSION] =
      GetShaderLocation(m_shader, "aoMap");
  m_shader.locs[SHADER_LOC_MAP_EMISSION] =
      GetShaderLocation(m_shader, "emissionMap");
  m_shader.locs[SHADER_LOC_MAP_HEIGHT] =
      GetShaderLocation(m_shader, "heightMap");

  for (int i = 0; i < 3; i++)
    m_models[i].materials[0].shader = m_shader;
}

void Preview3D::setTexture(int slot, Texture2D tex) {
  if (m_has[slot] && m_tex[slot].id != 0)
    UnloadTexture(m_tex[slot]);
  m_tex[slot] = tex;
  m_has[slot] = tex.id != 0;
  if (m_has[slot]) {
    SetTextureWrap(m_tex[slot], TEXTURE_WRAP_REPEAT);
    SetTextureFilter(m_tex[slot], TEXTURE_FILTER_BILINEAR);
  }
}

void Preview3D::updateTextures(NodeGraph* graph) {
  // Material core input slots: 0 Albedo, 1 Normal, 2 Roughness,
  // 3 Metallic, 4 Depth, 5 AO, 6 Emission
  GraphNode* mat = nullptr;
  for (auto* gn : graph->nodes())
    if (gn->texNode()->typeName() == "Material") {
      mat = gn;
      break;
    }

  if (!mat) {
    // no Material node: show the graph output as albedo
    for (int i = 1; i < 7; i++)
      setTexture(i, Texture2D{});
    GenTexture* out = graph->getLastOutput();
    setTexture(
        0, (out && out->Data) ? LoadTextureFromGenTexture(*out) : Texture2D{});
    return;
  }

  for (int i = 0; i < 7; i++) {
    GenTexture* in = graph->getInputImageForSlot(mat, i);
    setTexture(i,
               (in && in->Data) ? LoadTextureFromGenTexture(*in) : Texture2D{});
  }
}

void Preview3D::draw(NodeGraph* graph) {
  ensureInit();

  // refresh textures when the graph changes (with a small time floor)
  double now = GetTime();
  if (graph &&
      (graph->changeCount() != m_lastChangeCount || now - m_lastUpload > 1.0)) {
    m_lastChangeCount = graph->changeCount();
    m_lastUpload = now;
    updateTextures(graph);
  }

  // controls
  static const char* shapes = "Sphere\0Cube\0Plane\0";
  ImGui::PushItemWidth(90);
  ImGui::Combo("##p3dshape", &m_shape, shapes);
  ImGui::SameLine();
  ImGui::Checkbox("Spin##p3d", &m_autoRotate);
  ImGui::SameLine();
  ImGui::PushItemWidth(110);
  ImGui::SliderFloat("Height##p3d", &m_displace, 0.0f, 0.3f, "%.2f");
  ImGui::SameLine();
  ImGui::SliderFloat("Tile##p3d", &m_tiling, 1.0f, 4.0f, "%.0f");
  ImGui::PopItemWidth();
  ImGui::PopItemWidth();

  if (m_autoRotate)
    m_yaw += GetFrameTime() * 0.4f;

  // camera orbit
  Camera3D cam{};
  cam.position =
      Vector3{m_dist * cosf(m_pitch) * cosf(m_yaw), m_dist * sinf(m_pitch),
              m_dist * cosf(m_pitch) * sinf(m_yaw)};
  cam.target = Vector3{0, 0, 0};
  cam.up = Vector3{0, 1, 0};
  cam.fovy = 45.0f;
  cam.projection = CAMERA_PERSPECTIVE;

  // light and uniforms
  float lightDir[3] = {-0.45f, -0.75f, -0.5f};
  SetShaderValue(m_shader, m_locLightDir, lightDir, SHADER_UNIFORM_VEC3);
  float viewPos[3] = {cam.position.x, cam.position.y, cam.position.z};
  SetShaderValue(m_shader, m_locViewPos, viewPos, SHADER_UNIFORM_VEC3);
  float disp = m_shape == 2 ? m_displace * 2.0f : m_displace;
  SetShaderValue(m_shader, m_locDisplace, &disp, SHADER_UNIFORM_FLOAT);
  // the sphere's equirect UVs cover twice the arc along U as along V;
  // doubling the U repeat keeps the texels square
  float tiling[2] = {m_shape == 0 ? m_tiling * 2.0f : m_tiling, m_tiling};
  SetShaderValue(m_shader, m_locTiling, tiling, SHADER_UNIFORM_VEC2);
  static const int mapSlot[7] = {MATERIAL_MAP_ALBEDO,    MATERIAL_MAP_NORMAL,
                                 MATERIAL_MAP_ROUGHNESS, MATERIAL_MAP_METALNESS,
                                 MATERIAL_MAP_HEIGHT,    MATERIAL_MAP_OCCLUSION,
                                 MATERIAL_MAP_EMISSION};
  Model& model = m_models[m_shape];
  for (int i = 0; i < 7; i++) {
    int has = m_has[i] ? 1 : 0;
    SetShaderValue(m_shader, m_locHas[i], &has, SHADER_UNIFORM_INT);
    model.materials[0].maps[mapSlot[i]].texture =
        m_has[i] ? m_tex[i] : Texture2D{};
  }

  BeginTextureMode(m_rt);
  ClearBackground(Color{26, 26, 30, 255});
  BeginMode3D(cam);
  // the generated sphere has its poles on the Z axis; stand it upright
  // so the default view faces the equator, not a pole
  if (m_shape == 0)
    DrawModelEx(model, Vector3{0, 0, 0}, Vector3{1, 0, 0}, -90.0f,
                Vector3{1, 1, 1}, WHITE);
  else
    DrawModel(model, Vector3{0, 0, 0}, 1.0f, WHITE);
  EndMode3D();
  EndTextureMode();

  rlImGuiImageRenderTextureFit(&m_rt, true);

  // mouse orbit / zoom over the viewport
  if (ImGui::IsItemHovered()) {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      m_yaw += delta.x * 0.01f;
      m_pitch += delta.y * 0.01f;
      if (m_pitch > 1.5f)
        m_pitch = 1.5f;
      if (m_pitch < -1.5f)
        m_pitch = -1.5f;
      m_autoRotate = false;
    }
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      m_dist -= wheel * 0.2f;
      if (m_dist < 1.4f)
        m_dist = 1.4f;
      if (m_dist > 6.0f)
        m_dist = 6.0f;
    }
  }
}

void Preview3D::unload() {
  // Models share one shader and reference m_tex through their material
  // maps, so UnloadModel/UnloadShader here would double-free; this is
  // only called at shutdown where the GL context teardown reclaims
  // everything anyway.
  m_init = false;
}
