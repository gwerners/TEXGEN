#include "Core.h"
#include <fmt/color.h>
#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include "Const.h"

#include <imgui_impl_raylib.h>
#include <imgui_internal.h>
#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include "Ide.h"
#include "Utils.h"

using json = nlohmann::json;

Core::Core() {}

void Core::configure() {
  std::string filename(CONFIG_JSON_FILENAME);
  json data;

  if (exists(filename)) {
    data = json::parse(readFile(true, filename));
  } else {
    data = json::parse(CONFIG_JSON);
  }
  _priv._debug = data["debug"];
  _priv._width = data["width"];
  _priv._height = data["height"];
  _priv._font = data["font"];
}

void Core::run() {
  // force imediate flush
  setbuf(stdout, nullptr);
  fmt::print(fmt::emphasis::bold | fg(fmt::color::green), MSG_START);
  configure();

  const int screenWidth = _priv._width;
  const int screenHeight = _priv._height;
  InitWindow(screenWidth, screenHeight, "Raylib + ImGui Docking Example");

  // Set up ImGui with rlImGui
  rlImGuiSetup(true);

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable docking

  // Setup Platform/Renderer backends
  ImGui_ImplRaylib_Init();

  bool done = false;

  Ide ide(screenWidth, screenHeight, _priv._font);
  while (!done) {
    // Start ImGui frame
    rlImGuiBegin();

    ide.draw();
    rlImGuiEnd();

    BeginDrawing();
    ClearBackground(RAYWHITE);
    ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
    // DrawTexture(texture, screenWidth/2 - texture.width/2, screenHeight/2 -
    // texture.height/2, WHITE);
    EndDrawing();
    done = WindowShouldClose();
  }
  rlImGuiShutdown();
  ide.unLoadTextures();
  CloseWindow();
  return;
}
