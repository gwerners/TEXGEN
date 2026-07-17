#pragma once
// MaterialLibrary — folder browser with material thumbnails.
// Each project file (.json or .ptex) can have a "<file>.thumb.png"
// next to it; the grid shows it so materials can be recognized without
// loading and rendering them. Thumbnails are written on every project
// save and can be batch-generated for a folder (one per frame, so the
// UI keeps breathing and the build can be stopped).

#include <raylib.h>
#include <string>
#include <vector>

struct GenTexture;

class MaterialLibrary {
 public:
  MaterialLibrary();

  // Draws the library UI. Returns true when an entry was activated;
  // outPath receives the project path, outIsPtex its kind.
  bool draw(std::string& outPath, bool& outIsPtex);

  // Evaluates a project file headless and writes its thumbnail.
  static bool buildThumbnail(const std::string& file);

  // Writes a thumbnail for an already-evaluated output (used on save).
  static bool saveThumbnail(const GenTexture& src, const std::string& file);

  // Forget cached textures/entries so the next draw rescans.
  void invalidate();

 private:
  struct Entry {
    std::string path;
    std::string name;
    bool isPtex = false;
    Texture2D tex{};
    bool texTried = false;
  };
  void scan();

  char m_dir[512];
  std::vector<Entry> m_entries;
  bool m_scanned = false;
  int m_buildPos = -1;  // >= 0 while batch-building thumbnails
};
