#include "Ide.h"
#include "CExport.h"
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

// Helper: render a maximize/restore button right-aligned in the title bar area.
// Returns true if fullscreen state changed. Toggles fullscreen on first click,
// resets layout if already fullscreen.
static bool TitleBarMaxButton(const char* id,
                              bool& isFullscreen,
                              bool& resetLayout) {
  // Right-align a small button on the same line as the tab/title
  float btnW = ImGui::CalcTextSize(isFullscreen ? "[=]" : "[+]").x +
               ImGui::GetStyle().FramePadding.x * 2;
  ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - btnW);

  bool changed = false;
  if (isFullscreen) {
    if (ImGui::SmallButton((std::string("[=]##restore_") + id).c_str())) {
      isFullscreen = false;
      resetLayout = true;
      changed = true;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Reset Layout");
  } else {
    if (ImGui::SmallButton((std::string("[+]##max_") + id).c_str())) {
      isFullscreen = true;
      changed = true;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Maximize");
  }
  return changed;
}

//************************************************************
//************************************************************
//************************************************************
Ide::Ide(const int /*screenWidth*/,
         const int /*screenHeight*/,
         const std::string& fontPath)
    : m_firstTime(true),
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
  strncpy(m_exportName, "my_texture", sizeof(m_exportName));
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
  ImGui::PushFont(m_firaCodeRegular);

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
    ImGui::DockBuilderSetNodeSize(dockspace_id, displaySize);

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
    ImGui::SetNextWindowSize(displaySize);
  }
  ImGui::Begin("Left Panel", nullptr,
               m_isLeftFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  TitleBarMaxButton("left", m_isLeftFullscreen, m_resetLayout);

  // ── Generate ──────────────────────────────────────────────
  if (ImGui::Button("Generate", ImVec2(-1, 30))) {
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

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // ── Project ──────────────────────────────────────────────
  ImGui::Text("Project");
  ImGui::PushItemWidth(-1);
  ImGui::InputText("##projectfile", m_saveFilename, sizeof(m_saveFilename));
  ImGui::PopItemWidth();

  ImGui::Spacing();

  float btnW =
      (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) *
      0.5f;
  if (ImGui::Button("Save", ImVec2(btnW, 0))) {
    saveProject(m_saveFilename);
  }
  ImGui::SameLine();
  if (ImGui::Button("Save As..", ImVec2(btnW, 0))) {
    m_saveDialog.open(m_saveFilename);
  }

  if (ImGui::Button("Load", ImVec2(btnW, 0))) {
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
  ImGui::SameLine();
  if (ImGui::Button("Load..", ImVec2(btnW, 0))) {
    m_loadDialog.open(m_saveFilename);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // ── Export ───────────────────────────────────────────────
  ImGui::Text("Export");
  ImGui::PushItemWidth(-1);
  ImGui::InputText("##exportname", m_exportName, sizeof(m_exportName));
  ImGui::PopItemWidth();

  ImGui::Spacing();

  if (ImGui::Button("Export C Header", ImVec2(-1, 0))) {
    if (g_nodeGraph) {
      std::string hName(m_exportName);
      std::string hPath = hName + ".h";
      if (exportCHeader(g_nodeGraph, hName, hPath)) {
        fmt::print(fg(fmt::color::green), "Exported: {}\n", hPath);
      } else {
        fmt::print(fg(fmt::color::red), "Export failed: {}\n", hPath);
      }
    }
  }

  ImGui::End();

  // File dialogs (rendered as separate windows)
  if (m_saveDialog.show("Save Project As", FileDialog::Save, ".json")) {
    std::string path = m_saveDialog.getResultPath();
    strncpy(m_saveFilename, path.c_str(), sizeof(m_saveFilename) - 1);
    saveProject(m_saveFilename);
  }

  if (m_loadDialog.show("Load Project", FileDialog::Load, ".json")) {
    std::string path = m_loadDialog.getResultPath();
    strncpy(m_saveFilename, path.c_str(), sizeof(m_saveFilename) - 1);
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

  // ------------------------------------------------------------------
  // Bottom Panel - output image preview
  // ------------------------------------------------------------------
  if (m_isBottomFullscreen) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);
  }
  ImGui::Begin("Bottom Panel", nullptr,
               m_isBottomFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  TitleBarMaxButton("bottom", m_isBottomFullscreen, m_resetLayout);

  // Zoom controls
  if (ImGui::Button("+##zin")) {
    m_zoom += 0.1f;
    if (m_zoom > m_maxZoom)
      m_zoom = m_maxZoom;
  }
  ImGui::SameLine();
  if (ImGui::Button("-##zout")) {
    m_zoom -= 0.1f;
    if (m_zoom < m_minZoom)
      m_zoom = m_minZoom;
  }
  ImGui::SameLine();
  if (ImGui::Button("1:1##zreset")) {
    m_zoom = 1.0f;
  }
  ImGui::SameLine();
  ImGui::Text("%.0f%%", m_zoom * 100.0f);

  // Auto-update output when graph changes
  if (g_nodeGraph && g_nodeGraph->changeCount() != m_lastChangeCount) {
    m_lastChangeCount = g_nodeGraph->changeCount();
    GenTexture* lastOut = g_nodeGraph->getLastOutput();
    if (lastOut && lastOut->Data) {
      if (m_hasOutputTexture && m_outputTexture.id != 0)
        UnloadTexture(m_outputTexture);
      m_outputTexture = LoadTextureFromGenTexture(*lastOut);
      m_hasOutputTexture = (m_outputTexture.id != 0);
    }
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
    ImGui::SetNextWindowSize(displaySize);
  }
  ImGui::Begin("Right Panel", nullptr,
               m_isRightFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  TitleBarMaxButton("right", m_isRightFullscreen, m_resetLayout);

  // Node canvas
  createNodeCanvas();

  ImGui::End();

  ImGui::PopFont();
}
