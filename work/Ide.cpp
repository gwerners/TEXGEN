#include "Ide.h"
#include "CExport.h"
#include "Icons.h"
#include "Nodes.h"
#include "ProjectIO.h"
#include "Utils.h"
#include "texgen_utils.h"

#include <imgui_impl_raylib.h>
#include <imgui_internal.h>
#include <rlImGui.h>

#include <fmt/color.h>
#include <fmt/format.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <vector>

namespace fs = std::filesystem;

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
  g_nodeGraph->setRunner(&m_runner);

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

void Ide::refreshOutput() {
  if (!g_nodeGraph)
    return;
  GenTexture* lastOut = g_nodeGraph->getLastOutput();
  if (lastOut && lastOut->Data) {
    if (m_hasOutputTexture && m_outputTexture.id != 0)
      UnloadTexture(m_outputTexture);
    m_outputTexture = LoadTextureFromGenTexture(*lastOut);
    m_hasOutputTexture = (m_outputTexture.id != 0);
  }
}

static std::string pathStem(const std::string& path) {
  size_t slash = path.find_last_of("/\\");
  std::string base = slash == std::string::npos ? path : path.substr(slash + 1);
  size_t dot = base.find_last_of('.');
  return dot == std::string::npos ? base : base.substr(0, dot);
}

// Writes the on-disk render cache (full output + one PNG per node) from
// the graph's current in-memory outputs — mirrors the naming convention
// MaterialLibrary::buildThumbnail uses for its headless cache, so either
// path can populate the same "<path>.cache/" folder.
void Ide::writeCache(const std::string& path) {
  if (!g_nodeGraph)
    return;
  std::string dir = cacheDir(path);
  std::error_code ec;
  fs::create_directories(dir, ec);
  GenTexture* lastOut = g_nodeGraph->getLastOutput();
  if (lastOut && lastOut->Data)
    SaveImage(*lastOut, (dir + "/output.png").c_str());
  for (auto* gn : g_nodeGraph->nodes()) {
    if (gn->cachedOutputs.empty() || !gn->cachedOutputs[0].Data)
      continue;
    char fname[64];
    snprintf(fname, sizeof(fname), "/%03d_%s.png", gn->texNode()->id,
             gn->texNode()->typeName().c_str());
    SaveImage(gn->cachedOutputs[0], (dir + fname).c_str());
  }
}

// Loads the render cache written by writeCache()/buildThumbnail() into
// the bottom preview and each node's preview texture — no evaluation.
// Silently does nothing when a project has no cache yet (never built, or
// the .ptex was just imported): previews just stay empty until the user
// explicitly renders (toolbar Generate, or the Library's build buttons).
void Ide::loadCachedRender(const std::string& path) {
  std::string dir = cacheDir(path);
  if (!fs::exists(dir))
    return;

  std::string outPath = dir + "/output.png";
  if (fs::exists(outPath)) {
    if (m_hasOutputTexture && m_outputTexture.id != 0)
      UnloadTexture(m_outputTexture);
    m_outputTexture = LoadTexture(outPath.c_str());
    m_hasOutputTexture = (m_outputTexture.id != 0);
  }

  if (!g_nodeGraph)
    return;
  for (auto* gn : g_nodeGraph->nodes()) {
    char fname[64];
    snprintf(fname, sizeof(fname), "/%03d_%s.png", gn->texNode()->id,
             gn->texNode()->typeName().c_str());
    std::string nodePath = dir + fname;
    if (!fs::exists(nodePath))
      continue;
    if (gn->hasPreview && gn->previewTex.id != 0)
      UnloadTexture(gn->previewTex);
    gn->previewTex = LoadTexture(nodePath.c_str());
    gn->hasPreview = (gn->previewTex.id != 0);
  }
}

void Ide::doSave(const std::string& path) {
  if (!saveProject(path))
    return;
  m_currentName = pathStem(path);
  m_currentPath = path;
  if (g_nodeGraph) {
    GenTexture* lastOut = g_nodeGraph->getLastOutput();
    if (lastOut && lastOut->Data) {
      MaterialLibrary::saveThumbnail(*lastOut, path);
      writeCache(path);
    }
    m_library.invalidate();
  }
}

void Ide::doLoad(const std::string& path) {
  if (loadProject(path) && g_nodeGraph) {
    m_currentName = pathStem(path);
    m_currentPath = path;
    loadCachedRender(path);
  }
}

void Ide::doImport(const std::string& path) {
  if (importPtexProject(path) && g_nodeGraph) {
    m_currentName = pathStem(path);
    m_currentPath = path;
    loadCachedRender(path);
  }
}

// Toolbar icon button with tooltip (falls back to a text button)
static bool IconButton(const char* id, const char* icon, const char* tooltip) {
  Texture2D* t = uiIcon(icon);
  bool clicked = t ? ImGui::ImageButton(id, ImTextureID(t->id), ImVec2(20, 20))
                   : ImGui::Button(id);
  if (tooltip && ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", tooltip);
  return clicked;
}

void Ide::draw() {
  ImGui::PushFont(m_firaCodeRegular);

  // keep the thumbnail batch alive even when the library UI is hidden
  m_library.tick();

  // Create a full-screen docking space (minus the hint bar at the bottom)
  ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
  ImVec2 fullSize = ImGui::GetIO().DisplaySize;
  float hintBarH = ImGui::GetFontSize() + 10.0f;
  ImVec2 displaySize(fullSize.x, fullSize.y - hintBarH);
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

    ImGui::DockBuilderDockWindow("Project###Left Panel", left_id);
    ImGui::DockBuilderDockWindow("Preview###Bottom Panel", bottom_id);
    ImGui::DockBuilderDockWindow("Graph###Right Panel", right_id);

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
  ImGui::Begin("Project###Left Panel", nullptr,
               m_isLeftFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  TitleBarMaxButton("left", m_isLeftFullscreen, m_resetLayout);

  // ── Toolbar ──────────────────────────────────────────────
  if (IconButton("##tb_new", "new", "New project (clears the graph)")) {
    delete g_nodeGraph;
    g_nodeGraph = new NodeGraph();
    g_nodeGraph->setRunner(&m_runner);
    m_currentName = "untitled";
    m_currentPath.clear();
    refreshOutput();
  }
  ImGui::SameLine();
  if (IconButton("##tb_open", "open", "Load project..."))
    m_loadDialog.open(m_saveFilename);
  ImGui::SameLine();
  if (IconButton("##tb_save", "save", "Save project (writes thumbnail)"))
    doSave(m_saveFilename);
  ImGui::SameLine();
  if (IconButton("##tb_saveas", "saveas", "Save project as..."))
    m_saveDialog.open(m_saveFilename);
  ImGui::SameLine();
  ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
  ImGui::SameLine();
  if (IconButton("##tb_import", "import",
                 "Import a Material Maker .ptex project"))
    m_importDialog.open("", ".ptex");
  ImGui::SameLine();
  if (IconButton("##tb_export", "export", "Export C header")) {
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
  ImGui::SameLine();
  ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
  ImGui::SameLine();
  if (IconButton("##tb_generate", "play", "Generate (re-evaluate the graph)")) {
    if (g_nodeGraph) {
      g_nodeGraph->generate();
      refreshOutput();
    }
  }

  ImGui::Spacing();

  // ── Project ──────────────────────────────────────────────
  ImGui::Separator();
  ImGui::Text("Project: %s", m_currentName.c_str());
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", m_saveFilename);
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Project", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##projectfile", m_saveFilename, sizeof(m_saveFilename));
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Project file used by the Save/Load buttons");
  }

  // ── Export ───────────────────────────────────────────────
  if (ImGui::CollapsingHeader("Export")) {
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##exportname", m_exportName, sizeof(m_exportName));
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Base name for the exported C header");
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
  }

  // ── Library ──────────────────────────────────────────────
  if (ImGui::CollapsingHeader("Library", ImGuiTreeNodeFlags_DefaultOpen)) {
    std::string picked;
    bool pickedPtex = false;
    if (m_library.draw(picked, pickedPtex)) {
      if (pickedPtex) {
        doImport(picked);
      } else {
        strncpy(m_saveFilename, picked.c_str(), sizeof(m_saveFilename) - 1);
        m_saveFilename[sizeof(m_saveFilename) - 1] = '\0';
        doLoad(picked);
      }
    }
  }

  ImGui::End();

  // File dialogs (rendered as separate windows)
  if (m_saveDialog.show("Save Project As", FileDialog::Save, ".json")) {
    std::string path = m_saveDialog.getResultPath();
    strncpy(m_saveFilename, path.c_str(), sizeof(m_saveFilename) - 1);
    doSave(m_saveFilename);
  }

  if (m_loadDialog.show("Load Project", FileDialog::Load, ".json")) {
    std::string path = m_loadDialog.getResultPath();
    strncpy(m_saveFilename, path.c_str(), sizeof(m_saveFilename) - 1);
    doLoad(m_saveFilename);
  }

  if (m_importDialog.show("Import Material Maker Project", FileDialog::Load,
                          ".ptex")) {
    doImport(m_importDialog.getResultPath());
  }

  // ------------------------------------------------------------------
  // Bottom Panel - output image preview
  // ------------------------------------------------------------------
  if (m_isBottomFullscreen) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);
  }
  ImGui::Begin("Preview###Bottom Panel", nullptr,
               m_isBottomFullscreen ? ImGuiWindowFlags_NoDocking : 0);

  TitleBarMaxButton("bottom", m_isBottomFullscreen, m_resetLayout);

  ImGui::Checkbox("3D##previewmode", &m_preview3dOn);
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip(
        "GPU-lit 3D preview of the Material node maps\n"
        "(drag to orbit, wheel to zoom)");

  if (m_preview3dOn) {
    ImGui::SameLine();
    m_preview3d.draw(g_nodeGraph);
    ImGui::End();
  } else {
    ImGui::SameLine();
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
  }  // 2D preview branch

  // ------------------------------------------------------------------
  // Right Panel - node canvas
  // ------------------------------------------------------------------
  if (m_isRightFullscreen) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);
  }
  // the graph tab is titled after the loaded material (### keeps the
  // docking ID stable while the visible name changes)
  {
    std::string rightTitle = m_currentName + "###Right Panel";
    ImGui::Begin(rightTitle.c_str(), nullptr,
                 m_isRightFullscreen ? ImGuiWindowFlags_NoDocking : 0);
  }

  TitleBarMaxButton("right", m_isRightFullscreen, m_resetLayout);

  // Node canvas
  createNodeCanvas();

  ImGui::End();

  // A full graph run just landed (pollRunner() inside createNodeCanvas()
  // above) — refresh the on-disk render cache so the next load shows it
  // pre-rendered. Skip for a project that was never saved/loaded from a
  // real file (the default in-memory "untitled" graph).
  if (g_nodeGraph && g_nodeGraph->fullRunCount() != m_lastFullRunCount) {
    m_lastFullRunCount = g_nodeGraph->fullRunCount();
    if (!m_currentPath.empty())
      writeCache(m_currentPath);
  }

  // ------------------------------------------------------------------
  // Hint bar - contextual help at the bottom of the screen
  // ------------------------------------------------------------------
  ImGui::SetNextWindowPos(ImVec2(0, fullSize.y - hintBarH));
  ImGui::SetNextWindowSize(ImVec2(fullSize.x, hintBarH));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
  ImGui::Begin("##hintbar", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoDocking |
                   ImGuiWindowFlags_NoFocusOnAppearing |
                   ImGuiWindowFlags_NoNav);
  {
    std::string hint = g_nodeGraph ? g_nodeGraph->hintText() : std::string();
    const std::string lib = m_library.statusText();
    if (!lib.empty())
      hint += (hint.empty() ? "" : "   |   ") + lib;
    ImGui::TextUnformatted(hint.c_str());
  }
  ImGui::End();
  ImGui::PopStyleVar(2);

  ImGui::PopFont();
}
