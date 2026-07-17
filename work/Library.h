#pragma once
// MaterialLibrary — folder browser with material thumbnails.
// Each project file (.json or .ptex) can have a "<file>.thumb.png"
// next to it; the grid shows it so materials can be recognized without
// loading and rendering them. Thumbnails are written on every project
// save and can be batch-generated for a folder (one per frame, so the
// UI keeps breathing and the build can be stopped).

#include <raylib.h>
#include <deque>
#include <future>
#include <string>
#include <utility>
#include <vector>

struct GenTexture;

class MaterialLibrary {
 public:
  MaterialLibrary();

  // Draws the library UI. Returns true when an entry was activated;
  // outPath receives the project path, outIsPtex its kind.
  bool draw(std::string& outPath, bool& outIsPtex);

  // Advances the background thumbnail batch. Call once per frame from
  // the Ide so the build keeps running even while the library section
  // is collapsed or hidden.
  void tick();

  // "" when idle; "thumbnails 3/47" while building, or a sticky
  // "thumbnails stopped (12 left)" message after an interruption.
  std::string statusText() const;

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
  void startBuild(bool rebuildAll);
  void stopBuild();

  char m_dir[512];
  std::vector<Entry> m_entries;
  bool m_scanned = false;

  // thumbnail batch: a path queue drained by a small pool of worker
  // futures; in-flight renders are never cancelled, only abandoned
  std::deque<std::string> m_queue;
  std::vector<std::pair<std::string, std::future<bool>>> m_inflight;
  int m_total = 0, m_done = 0;
  bool m_building = false;
  std::string m_stopMsg;
};
