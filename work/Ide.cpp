#include "Ide.h"
#include "Nodes.h"
#include "ProjectIO.h"
#include "Utils.h"

#include <imgui_impl_raylib.h>
#include <imgui_internal.h>
#include <rlImGui.h>

#include <fmt/color.h>
#include <fmt/format.h>

#include <cstring>
#include <map>
#include <vector>

//************************************************************
//************************************************************
//************************************************************
Ide::Ide(const int screenWidth,
         const int screenHeight,
         const std::string& fontPath)
    : m_width(screenWidth),
      m_height(screenHeight),
      m_firstTime(true),
      m_isLeftFullscreen(false),
      m_isBottomFullscreen(false),
      m_isRightFullscreen(false),
      m_resetLayout(false),
      m_zoom(1.0f),
      m_minZoom(0.5f),
      m_maxZoom(3.0f),
      m_hasOutputTexture(false) {
  strncpy(m_saveFilename, "project.json", sizeof(m_saveFilename));
  strncpy(m_outputFilename, "output.tga", sizeof(m_outputFilename));
  m_outputTexture.id = 0;
  m_texture.id = 0;

  m_firaCodeRegular = LoadCustomFonts(fontPath);

  // Initialize the node graph
  if (!g_nodeGraph) {
    g_nodeGraph = new NodeGraph();
  }

  loadTexture();
}

ImFont* Ide::LoadCustomFonts(const std::string& fontPath) {
  ImGuiIO& io = ImGui::GetIO();
  io.Fonts->AddFontDefault();
  ImFont* font = nullptr;
  if (exists(fontPath)) {
    font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
    IM_ASSERT(font != nullptr);
  } else {
    fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
               "Unable to find font {}", fontPath);
    return nullptr;
  }

  return font;
}

void Ide::loadTexture() {
  // Create a simple 1x1 placeholder texture
  m_texture.id = 0;
  m_texture.width = 1;
  m_texture.height = 1;
}

void Ide::unLoadTextures() {
  if (m_texture.id != 0)
    UnloadTexture(m_texture);
  if (m_outputTexture.id != 0)
    UnloadTexture(m_outputTexture);
}

void Ide::draw() {
  ImGui::PushFont(m_firaCodeRegular);  // Push the custom font for ImNodes

  // Create a full-screen docking space
  ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
  ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(displaySize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("DockSpace", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                   ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::End();
  ImGui::PopStyleVar(2);

  // Programmatically dock the windows (only done once or when resetting)
  if (m_resetLayout || m_firstTime) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id,
                                  ImVec2((float)m_width, (float)m_height));

    ImGuiID left_id, right_id;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, &left_id,
                                &right_id);

    ImGuiID bottom_id;
    ImGui::DockBuilderSplitNode(right_id, ImGuiDir_Down, 0.3f, &bottom_id,
                                &right_id);

    ImGui::DockBuilderDockWindow("Left Panel", left_id);
    ImGui::DockBuilderDockWindow("Bottom Panel", bottom_id);
    ImGui::DockBuilderDockWindow("Right Panel", right_id);

    ImGui::DockBuilderFinish(dockspace_id);

    m_isLeftFullscreen = false;
    m_isBottomFullscreen = false;
    m_isRightFullscreen = false;
    m_resetLayout = false;
    m_firstTime = false;
  }

  // ------------------------------------------------------------------
  // Left Panel
  // ------------------------------------------------------------------
  if (m_isLeftFullscreen) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_width, (float)m_height));
  }
  ImGui::Begin("Left Panel", nullptr,
               m_isLeftFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  ImGui::Text("Texture Node Graph");
  ImGui::Separator();

  // Generate button
  if (ImGui::Button("Generate")) {
    if (g_nodeGraph) {
      g_nodeGraph->generate();
      GenTexture* lastOut = g_nodeGraph->getLastOutput();
      if (lastOut && lastOut->Data) {
        if (m_hasOutputTexture && m_outputTexture.id != 0) {
          UnloadTexture(m_outputTexture);
        }
        m_outputTexture = LoadTextureFromGenTexture(*lastOut);
        m_hasOutputTexture = (m_outputTexture.id != 0);
      }
    }
  }

  ImGui::Separator();
  ImGui::Text("Project I/O:");

  ImGui::PushItemWidth(160);
  ImGui::InputText("##savefile", m_saveFilename, sizeof(m_saveFilename));
  ImGui::PopItemWidth();

  ImGui::SameLine();
  if (ImGui::Button("Save")) {
    saveProject(m_saveFilename);
  }

  ImGui::PushItemWidth(160);
  ImGui::InputText("##loadfile", m_saveFilename, sizeof(m_saveFilename));
  ImGui::PopItemWidth();

  ImGui::SameLine();
  if (ImGui::Button("Load")) {
    if (loadProject(m_saveFilename) && g_nodeGraph) {
      g_nodeGraph->generate();
      GenTexture* lastOut = g_nodeGraph->getLastOutput();
      if (lastOut && lastOut->Data) {
        if (m_hasOutputTexture && m_outputTexture.id != 0) {
          UnloadTexture(m_outputTexture);
        }
        m_outputTexture = LoadTextureFromGenTexture(*lastOut);
        m_hasOutputTexture = (m_outputTexture.id != 0);
      }
    }
  }

  ImGui::Separator();

  if (ImGui::Button("Toggle Fullscreen##left")) {
    m_isLeftFullscreen = !m_isLeftFullscreen;
  }
  if (ImGui::Button("Reset Layout##left")) {
    m_resetLayout = true;
  }

  ImGui::End();

  // ------------------------------------------------------------------
  // Bottom Panel - output image preview
  // ------------------------------------------------------------------
  if (m_isBottomFullscreen) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_width, (float)m_height));
  }
  ImGui::Begin("Bottom Panel", nullptr,
               m_isBottomFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  ImGui::Text("Output Preview");
  if (ImGui::Button("Toggle Fullscreen##bottom")) {
    m_isBottomFullscreen = !m_isBottomFullscreen;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset Layout##bottom")) {
    m_resetLayout = true;
  }

  // Zoom controls
  if (ImGui::Button("Zoom In")) {
    m_zoom += 0.1f;
    if (m_zoom > m_maxZoom)
      m_zoom = m_maxZoom;
  }
  ImGui::SameLine();
  if (ImGui::Button("Zoom Out")) {
    m_zoom -= 0.1f;
    if (m_zoom < m_minZoom)
      m_zoom = m_minZoom;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset Zoom")) {
    m_zoom = 1.0f;
  }

  if (m_hasOutputTexture && m_outputTexture.id != 0) {
    ImVec2 imageSize = ImVec2(float(m_outputTexture.width * m_zoom),
                              float(m_outputTexture.height * m_zoom));
    ImGui::Image(ImTextureID(m_outputTexture.id), imageSize);
  } else {
    ImGui::TextDisabled("(no output yet - click Generate)");
  }

  ImGui::End();

  // ------------------------------------------------------------------
  // Right Panel - node canvas
  // ------------------------------------------------------------------
  if (m_isRightFullscreen) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_width, (float)m_height));
  }
  ImGui::Begin("Right Panel", nullptr,
               m_isRightFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  if (ImGui::Button("Toggle Fullscreen##right")) {
    m_isRightFullscreen = !m_isRightFullscreen;
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset Layout##right")) {
    m_resetLayout = true;
  }

  // Node canvas
  createNodeCanvas();

  ImGui::End();

  ImGui::PopFont();
}
