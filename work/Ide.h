#ifndef IDE_H
#define IDE_H
#include <string>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <raylib.h>
#include "ProjectIO.h"

class Ide {
 public:
  Ide(const int screenWidth,
      const int screenHeight,
      const std::string& fontPath);
  ~Ide() {};
  void loadTexture();
  void unLoadTextures();
  void draw();

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
  char m_outputFilename[256];
  Texture2D m_outputTexture;
  bool m_hasOutputTexture;
};

#endif  // IDE_H
