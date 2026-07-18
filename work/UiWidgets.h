#pragma once
// Shared parameter widgets with fine tuning, used by all node UIs.
// On top of the plain ImGui sliders:
//  - clicking a slider SELECTS it (thin highlight frame); Up/Down
//    arrows then step the value (Shift = 10x finer for floats), even
//    after the mouse leaves the slider. Esc clears the selection.
//  - the mouse wheel steps the value while hovering
//  - Ctrl+Click is ImGui's native slider-to-text-input: type an exact
//    value and press Enter
#include <imgui.h>

namespace uiw {

inline ImGuiID& lastSlider() {
  static ImGuiID id = 0;
  return id;
}

// Shared post-slider logic. Returns the applied delta in steps.
inline int sliderSteps(bool allowFine, float* fineFactor) {
  ImGuiID id = ImGui::GetItemID();
  if (ImGui::IsItemActivated())
    lastSlider() = id;
  const bool selected = lastSlider() == id;
  if (selected) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
      lastSlider() = 0;
  }
  ImGuiIO& io = ImGui::GetIO();
  *fineFactor = (allowFine && io.KeyShift) ? 0.1f : 1.0f;
  int steps = 0;
  // arrows apply to the hovered or selected slider, but never while a
  // text field is being edited (e.g. the Ctrl+Click input)
  if (!io.WantTextInput && (ImGui::IsItemHovered() || selected)) {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
      steps++;
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
      steps--;
  }
  if (ImGui::IsItemHovered()) {
    const float wheel = io.MouseWheel;
    if (wheel > 0.0f)
      steps += (int)(wheel + 0.5f);
    else if (wheel < 0.0f)
      steps -= (int)(-wheel + 0.5f);
  }
  return steps;
}

inline bool SliderFloatW(const char* label,
                         float* v,
                         float mn,
                         float mx,
                         float step = 0.0f) {
  bool changed = ImGui::SliderFloat(label, v, mn, mx);
  if (step <= 0.0f)
    step = (mx - mn) * 0.01f;
  float fine = 1.0f;
  const int steps = sliderSteps(true, &fine);
  if (steps != 0) {
    *v += steps * step * fine;
    *v = *v < mn ? mn : (*v > mx ? mx : *v);
    changed = true;
  }
  return changed;
}

inline bool SliderIntW(const char* label,
                       int* v,
                       int mn,
                       int mx,
                       int step = 1) {
  bool changed = ImGui::SliderInt(label, v, mn, mx);
  float fine = 1.0f;
  const int steps = sliderSteps(false, &fine);
  if (steps != 0) {
    *v += steps * step;
    *v = *v < mn ? mn : (*v > mx ? mx : *v);
    changed = true;
  }
  return changed;
}

inline void Hint(const char* text) {
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", text);
}

}  // namespace uiw

using uiw::Hint;
using uiw::SliderFloatW;
using uiw::SliderIntW;
