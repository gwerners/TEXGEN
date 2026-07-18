#ifndef IDE_H
#define IDE_H
#include <string>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <raylib.h>
#include "FileDialog.h"
#include "GraphRunner.h"
#include "Library.h"
#include "Preview3D.h"
#include "ProjectIO.h"

class Ide {
 public:
  Ide(const int screenWidth,
      const int screenHeight,
      const std::string& fontPath);
  ~Ide() {};
  void loadTexture();
  void unLoadTextures();
  // project helpers (public so harnesses/scripts can drive the Ide)
  void doSave(const std::string& path);
  void doLoad(const std::string& path);
  void doImport(const std::string& path);
  void draw();

 private:
  // refresh the bottom-panel texture from the graph output
  void refreshOutput();
  // write the on-disk render cache (full output + one PNG per node) from
  // the graph's current in-memory outputs, next to the project file
  void writeCache(const std::string& path);
  // load the on-disk render cache into the bottom preview and node
  // previews, without evaluating anything (used on project load)
  void loadCachedRender(const std::string& path);

 private:
  ImFont* LoadCustomFonts(const std::string& fontPath);
  bool m_firstTime;
  bool m_isLeftFullscreen;
  bool m_isBottomFullscreen;
  bool m_isRightFullscreen;
  bool m_resetLayout;
  float m_zoom;
  const float m_minZoom;
  const float m_maxZoom;
  Texture2D m_texture;
  ImFont* m_firaCodeRegular;

  char m_saveFilename[256];
  std::string m_currentName = "untitled";  // stem of the loaded project
  // Full path backing the currently loaded/saved project (independent of
  // m_saveFilename, which only tracks the toolbar Save/Save-As target —
  // doImport() never syncs that one). Empty for the in-memory default
  // graph that was never saved/loaded from a real file.
  std::string m_currentPath;
  char m_outputFilename[256];
  char m_exportName[128];
  Texture2D m_outputTexture;
  bool m_hasOutputTexture;
  FileDialog m_saveDialog;
  FileDialog m_loadDialog;
  FileDialog m_importDialog;
  int m_lastChangeCount = -1;
  int m_lastFullRunCount = -1;
  Preview3D m_preview3d;
  bool m_preview3dOn = false;
  MaterialLibrary m_library;
  GraphRunner m_runner;
};

#endif  // IDE_H
