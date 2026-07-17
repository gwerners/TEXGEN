#include "Library.h"
#include "Icons.h"

#include "HeadlessEval.h"
#include "PtexImport.h"
#include "texgen_utils.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

const int THUMB_SIZE = 128;

std::string thumbPath(const std::string& file) {
  return file + ".thumb.png";
}

// Box-downsample an evaluated output into a square thumbnail.
void downsample(const GenTexture& src, GenTexture& dst, int side) {
  dst.Init(side, side);
  const int bw = src.XRes / side > 0 ? src.XRes / side : 1;
  const int bh = src.YRes / side > 0 ? src.YRes / side : 1;
  for (int y = 0; y < side; y++) {
    for (int x = 0; x < side; x++) {
      const int sx = x * src.XRes / side;
      const int sy = y * src.YRes / side;
      unsigned r = 0, g = 0, b = 0, a = 0, n = 0;
      for (int j = 0; j < bh; j++) {
        for (int i = 0; i < bw; i++) {
          const int px = (sx + i) % src.XRes;
          const int py = (sy + j) % src.YRes;
          const Pixel& p = src.Data[py * src.XRes + px];
          r += p.r;
          g += p.g;
          b += p.b;
          a += p.a;
          n++;
        }
      }
      Pixel& o = dst.Data[y * side + x];
      o.r = (sU16)(r / n);
      o.g = (sU16)(g / n);
      o.b = (sU16)(b / n);
      o.a = (sU16)(a / n);
    }
  }
}

}  // namespace

MaterialLibrary::MaterialLibrary() {
  if (fs::is_directory("MaterialMaker"))
    strncpy(m_dir, "MaterialMaker", sizeof(m_dir) - 1);
  else
    strncpy(m_dir, ".", sizeof(m_dir) - 1);
  m_dir[sizeof(m_dir) - 1] = '\0';
}

void MaterialLibrary::invalidate() {
  for (auto& e : m_entries)
    if (e.texTried && e.tex.id != 0)
      UnloadTexture(e.tex);
  m_entries.clear();
  m_scanned = false;
  m_buildPos = -1;
}

void MaterialLibrary::scan() {
  m_scanned = true;
  m_entries.clear();
  try {
    for (auto& de : fs::directory_iterator(m_dir)) {
      if (!de.is_regular_file())
        continue;
      std::string ext = de.path().extension().string();
      std::string name = de.path().stem().string();
      if (ext != ".json" && ext != ".ptex")
        continue;
      // skip helper files like foo.thumb.png (stem foo.thumb)
      if (name.size() > 6 && name.compare(name.size() - 6, 6, ".thumb") == 0)
        continue;
      Entry e;
      e.path = de.path().string();
      e.name = name;
      e.isPtex = ext == ".ptex";
      m_entries.push_back(std::move(e));
    }
  } catch (...) {
  }
  std::sort(m_entries.begin(), m_entries.end(),
            [](const Entry& a, const Entry& b) { return a.name < b.name; });
}

bool MaterialLibrary::saveThumbnail(const GenTexture& src,
                                    const std::string& file) {
  if (!src.Data)
    return false;
  GenTexture thumb;
  downsample(src, thumb, THUMB_SIZE);
  return SaveImage(thumb, thumbPath(file).c_str());
}

bool MaterialLibrary::buildThumbnail(const std::string& file) {
  std::ifstream f(file);
  if (!f.is_open())
    return false;
  nlohmann::json j;
  try {
    f >> j;
  } catch (...) {
    return false;
  }
  if (file.size() > 5 && file.compare(file.size() - 5, 5, ".ptex") == 0) {
    std::vector<std::string> skipped;
    j = ptexToTexgen(j, "thumb", &skipped);
  }
  GenTexture out;
  if (!headlessEvaluate(j, out) || !out.Data)
    return false;
  return saveThumbnail(out, file);
}

bool MaterialLibrary::draw(std::string& outPath, bool& outIsPtex) {
  if (!m_scanned)
    scan();

  // folder row
  ImGui::PushItemWidth(-58.0f);
  if (ImGui::InputText("##libdir", m_dir, sizeof(m_dir),
                       ImGuiInputTextFlags_EnterReturnsTrue))
    invalidate();
  ImGui::PopItemWidth();
  ImGui::SameLine();
  {
    Texture2D* ic = uiIcon("refresh");
    bool clicked = ic ? ImGui::ImageButton("##librescan", ImTextureID(ic->id),
                                           ImVec2(16, 16))
                      : ImGui::SmallButton("Rescan");
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Rescan the folder");
    if (clicked)
      invalidate();
  }

  if (m_entries.empty()) {
    ImGui::TextDisabled("(no .json / .ptex files here)");
    return false;
  }

  // batch thumbnail build: one file per frame so the UI stays alive
  int missing = 0;
  for (auto& e : m_entries)
    if (!fs::exists(thumbPath(e.path)))
      missing++;
  if (m_buildPos < 0) {
    if (missing > 0) {
      char label[64];
      snprintf(label, sizeof(label), "Build %d missing thumbnail%s", missing,
               missing == 1 ? "" : "s");
      if (ImGui::SmallButton(label))
        m_buildPos = 0;
    }
  } else {
    // advance to the next entry without a thumbnail
    while (m_buildPos < (int)m_entries.size() &&
           fs::exists(thumbPath(m_entries[m_buildPos].path)))
      m_buildPos++;
    if (m_buildPos >= (int)m_entries.size()) {
      if (!m_buildInFlight)
        m_buildPos = -1;
    }
    if (m_buildPos >= 0 && m_buildPos < (int)m_entries.size()) {
      Entry& e = m_entries[m_buildPos];
      ImGui::Text("Rendering %s...", e.name.c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("Stop##libbuild") && !m_buildInFlight) {
        m_buildPos = -1;
      } else if (!m_buildInFlight) {
        // evaluate on a worker so the UI never blocks
        std::string path = e.path;
        m_buildFuture = std::async(std::launch::async, buildThumbnail, path);
        m_buildInFlight = true;
      } else if (m_buildFuture.wait_for(std::chrono::seconds(0)) ==
                 std::future_status::ready) {
        if (!m_buildFuture.get()) {
          // write a placeholder so failures aren't retried forever
          GenTexture black;
          black.Init(4, 4);
          SaveImage(black, thumbPath(e.path).c_str());
        }
        e.texTried = false;  // reload the texture on the next pass
        m_buildInFlight = false;
        m_buildPos++;
      }
    }
  }

  // thumbnail grid
  const float cell = 86.0f;
  int cols = (int)(ImGui::GetContentRegionAvail().x / (cell + 8.0f));
  if (cols < 1)
    cols = 1;
  bool activated = false;
  int col = 0;
  for (auto& e : m_entries) {
    if (!e.texTried) {
      e.texTried = true;
      std::string tp = thumbPath(e.path);
      if (fs::exists(tp)) {
        e.tex = LoadTexture(tp.c_str());
        if (e.tex.id != 0)
          SetTextureFilter(e.tex, TEXTURE_FILTER_BILINEAR);
      }
    }
    if (col > 0)
      ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::PushID(e.path.c_str());
    bool clicked;
    if (e.tex.id != 0) {
      clicked = ImGui::ImageButton("##thumb", ImTextureID(e.tex.id),
                                   ImVec2(cell - 8, cell - 8));
    } else {
      clicked = ImGui::Button("?", ImVec2(cell - 8, cell - 8));
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("%s%s", e.name.c_str(),
                        e.isPtex ? "  (Material Maker)" : "");
    // truncated label under the thumbnail
    std::string label =
        e.name.size() > 11 ? e.name.substr(0, 10) + "…" : e.name;
    ImGui::TextUnformatted(label.c_str());
    ImGui::PopID();
    ImGui::EndGroup();
    if (clicked) {
      outPath = e.path;
      outIsPtex = e.isPtex;
      activated = true;
    }
    col = (col + 1) % cols;
  }
  return activated;
}
