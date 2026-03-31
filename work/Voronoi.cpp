#include "Voronoi.h"
#include <fmt/color.h>
#include <fmt/format.h>
#include <imgui.h>
#include <iostream>
float g_zoom = 1.0f;
float g_minZoom = 0.5f;
float g_maxZoom = 3.0f;

#ifdef ORIGINAL
void proportionalInput() {
  static int dirty = 0;
  static int width = 256;   // Initial width
  static int height = 256;  // Initial height
  // Lambda to round up to the nearest power of 2 with overflow handling
  auto RoundUpToPowerOfTwo = [](int value) -> int {
    if (value <= 1)
      return 1;
    int power = static_cast<int>(std::ceil(std::log2(value)));
    if (power >= 31)
      return 1 << 30;
    return 1 << power;
  };

  // Lambda to round down to the nearest power of 2 with overflow handling
  auto RoundDownToPowerOfTwo = [](int value) -> int {
    if (value <= 1)
      return 1;
    int power = static_cast<int>(std::floor(std::log2(value)));
    if (power >= 31)
      return 1 << 30;
    return 1 << power;
  };
  ImGui::SetNextItemWidth(50);  // Set a fixed width for the input field
  ImGui::InputInt("##Width", &width, 0, 0, ImGuiInputTextFlags_CharsDecimal);
  ImGui::SameLine();
  if (ImGui::Button("+##WidthUp")) {
    width += 2;  // Increment by 2
    dirty = 1;
    width = RoundUpToPowerOfTwo(width);
  }
  ImGui::SameLine();
  if (ImGui::Button("-##WidthDown")) {
    width -= 2;
    if (width < 2)
      width = 2;
    dirty = 1;
    width = RoundDownToPowerOfTwo(width);
  }
  ImGui::SameLine();
  ImGui::Text("Width:");

  ImGui::SetNextItemWidth(50);  // Set a fixed width for the input field
  ImGui::InputInt("##Height", &height, 0, 0, ImGuiInputTextFlags_CharsDecimal);

  ImGui::SameLine();
  if (ImGui::Button("+##HeightUp")) {
    height += 2;  // Increment by 2
    dirty = 2;
    height = RoundUpToPowerOfTwo(height);
  }
  ImGui::SameLine();
  if (ImGui::Button("-##HeightDown")) {
    height -= 2;  // Decrement by 2, but enforce minimum of 2
    if (height < 2)
      height = 2;
    dirty = 2;
    height = RoundDownToPowerOfTwo(height);
  }
  ImGui::SameLine();
  ImGui::Text("Height:");
  switch (dirty) {
    case 1:
      height = width;
      dirty = 0;
      break;
    case 2:
      width = height;
      dirty = 0;
      break;
  }
}
#endif
// Rounding lambdas
static auto RoundUpToPowerOfTwo = [](int value) -> int {
  if (value <= 1)
    return 1;
  int power = static_cast<int>(std::ceil(std::log2(value)));
  if (power >= 31)
    return 1 << 30;
  return 1 << power;
};

static auto RoundDownToPowerOfTwo = [](int value) -> int {
  if (value <= 1)
    return 1;
  int power = static_cast<int>(std::floor(std::log2(value)));
  if (power >= 31)
    return 1 << 30;
  return 1 << power;
};

void HandleDimension(const char* label, int& dimension, bool increment) {
  if (increment) {
    dimension += 2;
    dimension = RoundUpToPowerOfTwo(dimension);
  } else {
    dimension -= 2;
    dimension = std::max(2, RoundDownToPowerOfTwo(dimension));
  }
}

void addTexturePreview(Texture2D texture) {
  // Buttons for zoom control
  if (ImGui::Button("Zoom In")) {
    g_zoom += 0.1f;
    if (g_zoom > g_maxZoom)
      g_zoom = g_maxZoom;
  }
  ImGui::SameLine();
  if (ImGui::Button("Zoom Out")) {
    g_zoom -= 0.1f;
    if (g_zoom < g_minZoom)
      g_zoom = g_minZoom;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset Zoom")) {
    g_zoom = 1.0f;
  }
  std::cout << " id " << texture.id << std::endl;

  static Texture2D tex;
  static bool init = false;
  if (!init) {
    GenTexture voro;
    static sInt voroIntens = {255};
    static sInt voroCount = {90};
    static sF32 voroDist = {0.125f};
    GenTexture gradWhite = LinearGradient(0xffffffff, 0xffffffff);

    voro.Init(256, 256);
    // use gradWhite to get a colored voronoi
    // use gradBW to get a black and white voronoi
    randomVoronoi(voro, gradWhite, voroIntens, voroCount, voroDist);
    // instance.colorize(voro, 0xff747d8e, 0xfff1feff);
    tex = LoadTextureFromGenTexture(voro);
    init = true;
  }
  if (texture.id != INT_MAX) {
    tex = texture;
  }

  if (init) {
    // Display the texture with zoom
    ImVec2 imageSize =
        ImVec2(float(tex.width * g_zoom), float(tex.height * g_zoom));

    // Ensure the texture is bound before passing it to ImGui
    ImGui::Image((ImTextureID)&tex, imageSize);
  }
}

Voronoi::Voronoi()
    : m_side(256), m_intensity(255), m_count(90), m_distance(0.125f) {
  m_gradient = LinearGradient(0xffffffff, 0xffffffff);
  m_texture.Init(m_side, m_side);
  randomVoronoi(m_texture, m_gradient, m_intensity, m_count, m_distance);
  texture2D = LoadTextureFromGenTexture(m_texture);
}

Voronoi::~Voronoi() {
  UnloadTexture(texture2D);
}

void Voronoi::setGradient(GenTexture tex) {
  m_gradient = tex;
}

GenTexture Voronoi::gradient() {
  return m_gradient;
}

void Voronoi::refresh() {
  UnloadTexture(texture2D);
  randomVoronoi(m_texture, m_gradient, m_intensity, m_count, m_distance);
  texture2D = LoadTextureFromGenTexture(m_texture);
}

GenTexture Voronoi::texture() {
  return m_texture;
}

GeneratorType Voronoi::type() {
  return GeneratorType::Voronoi;
}

bool Voronoi::accept(Generator* other) {
  if (dynamic_cast<class Gradient*>(other)) {
    return true;
  }
  return false;
}
void Voronoi::render() {
  static bool dirty = false;
  static enum GradientType gradient = GradientType::White;
  static const char* items[]{"Black and White", "White and Black", "White"};
  static int selectedGradient = 0;
  ImVec2 textSize = ImGui::CalcTextSize(items[0]);
  ImGui::SetNextItemWidth(textSize.x + 50);

  if (ImGui::Combo("##hidden", &selectedGradient, items, IM_ARRAYSIZE(items))) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               "gradiente selected");
    switch (selectedGradient) {
      case 0:
        gradient = GradientType::BlackAndWhite;
        break;
      case 1:
        gradient = GradientType::WhiteAndBlack;
        break;
      case 2:
        gradient = GradientType::White;
        break;
    }
    dirty = true;
  }
  // Width controls
  ImGui::SetNextItemWidth(50);
  if (ImGui::InputInt("##Side", &m_side, 0, 0,
                      ImGuiInputTextFlags_CharsDecimal)) {
    m_side = RoundUpToPowerOfTwo(m_side);
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("+##SideUp")) {
    HandleDimension("Side", m_side, true);
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("-##SideDown")) {
    HandleDimension("Side", m_side, false);
    dirty = true;
  }
  ImGui::SameLine();
  ImGui::Text("Side:");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Texture Side must be a power of two.");
  }

  ImGui::SetNextItemWidth(50);
  ImGui::InputInt("##Intensity", &m_intensity, 0, 0,
                  ImGuiInputTextFlags_CharsDecimal);
  ImGui::SameLine();
  if (ImGui::Button("+##IntensityUp")) {
    ++m_intensity;
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("-##IntensityDown")) {
    --m_intensity;
    dirty = true;
  }
  ImGui::SameLine();
  ImGui::Text("Intensity:");

  ImGui::SetNextItemWidth(50);
  ImGui::InputInt("##Count", &m_count, 0, 0, ImGuiInputTextFlags_CharsDecimal);
  ImGui::SameLine();
  if (ImGui::Button("+##CountUp")) {
    ++m_count;
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("-##CountDown")) {
    --m_count;
    dirty = true;
  }
  ImGui::SameLine();
  ImGui::Text("Count:");

  ImGui::SetNextItemWidth(50);
  ImGui::InputFloat("##Distance", &m_distance, 0, 0, "%f",
                    ImGuiInputTextFlags_CharsDecimal);
  ImGui::SameLine();
  if (ImGui::Button("+##DistanceUp")) {
    m_distance += 0.001f;
    dirty = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("-##DistanceDown")) {
    m_distance -= 0.001f;
    dirty = true;
  }
  ImGui::SameLine();
  ImGui::Text("Distance:");

  if (ImGui::Button("Generate")) {
    dirty = true;
  }
  ImGui::SameLine();
  static bool show = true;
  ImGui::Checkbox("Show texture", &show);
  if (show) {
    if (dirty) {
      GenTexture grad;
      switch (gradient) {
        case GradientType::BlackAndWhite:
          grad = LinearGradient(0xff000000, 0xffffffff);
          break;
        case GradientType::WhiteAndBlack:
          grad = LinearGradient(0xffffffff, 0xff000000);
          break;
        case GradientType::White:
          grad = LinearGradient(0xffffffff, 0xffffffff);
          break;
      }
      GenTexture voro;

      voro.Init(m_side, m_side);
      // use gradWhite to get a colored voronoi
      // use gradBW to get a black and white voronoi
      randomVoronoi(voro, grad, m_intensity, m_count, m_distance);
      // instance.colorize(voro, 0xff747d8e, 0xfff1feff);
      UnloadTexture(texture2D);

      texture2D = LoadTextureFromGenTexture(voro);
      dirty = false;
    }
    ImVec2 imageSize = ImVec2(float(m_side * g_zoom), float(m_side * g_zoom));
    ImGui::Image((ImTextureID)&texture2D, imageSize);

    /*if (dirty){
    refresh();
    texture2D = LoadTextureFromGenTexture(m_texture);
    dirty =false;
    }
    // Display the texture with zoom
    ImVec2 imageSize =
        ImVec2(float(m_side * g_zoom), float(m_side * g_zoom));

    // Ensure the texture is bound before passing it to ImGui
    ImGui::Image((ImTextureID)&texture2D, imageSize);*/
  }
}

std::string Voronoi::title() {
  return "Voronoi";
}
