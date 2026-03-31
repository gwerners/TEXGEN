#include "Gradient.h"
#include <fmt/color.h>
#include <fmt/format.h>
#include <imgui.h>

Gradient::Gradient() : m_gradientType(GradientType::White) {}

void Gradient::setRange(sU32 start, sU32 end) {
  m_start = start;
  m_end = end;
}

void Gradient::setGradient(GradientType gt) {
  m_gradientType = gt;
}

void Gradient::refresh() {
  switch (m_gradientType) {
    case GradientType::BlackAndWhite:
      m_tex = LinearGradient(0xff000000, 0xffffffff);
      break;
    case GradientType::WhiteAndBlack:
      m_tex = LinearGradient(0xffffffff, 0xff000000);
      break;
    case GradientType::White:
      m_tex = LinearGradient(0xffffffff, 0xffffffff);
      break;
    default:
      m_tex = LinearGradient(m_start, m_end);
      break;
  }
}

GenTexture Gradient::texture() {
  return m_tex;
}

GeneratorType Gradient::type() {
  return GeneratorType::Gradient;
}

bool Gradient::accept(Generator* other) {
  return true;
}

void Gradient::render() {
  static const char* items[]{"Black and White", "White and Black", "White"};
  static int selectedGradient = 0;
  ImVec2 textSize = ImGui::CalcTextSize(items[0]);
  ImGui::SetNextItemWidth(textSize.x + 50);

  if (ImGui::Combo("##hidden", &selectedGradient, items, IM_ARRAYSIZE(items))) {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::green),
               "gradiente selected");
  }
}

std::string Gradient::title() {
  return "Gradient";
}
