#include "Library.h"
#include "Icons.h"

#include "GraphEval.h"
#include "PtexImport.h"
#include "texgen_utils.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;

std::string thumbPath(const std::string& file) {
  return file + ".thumb.png";
}

std::string cacheDir(const std::string& file) {
  return file + ".cache";
}

namespace {

const int THUMB_SIZE = 128;

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
  if (fs::is_directory("Material"))
    strncpy(m_dir, "Material", sizeof(m_dir) - 1);
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
  if (m_building)
    stopBuild();
}

void MaterialLibrary::startBuild(bool rebuildAll) {
  if (!m_scanned)
    scan();
  if (rebuildAll)
    for (auto& e : m_entries) {
      std::error_code ec;
      fs::remove(thumbPath(e.path), ec);
      if (e.texTried && e.tex.id != 0)
        UnloadTexture(e.tex);
      e.tex = Texture2D{};
      e.texTried = false;
    }
  m_queue.clear();
  for (auto& e : m_entries)
    if (!fs::exists(thumbPath(e.path)))
      m_queue.push_back(e.path);
  m_total = (int)m_queue.size();
  m_done = 0;
  m_building = m_total > 0;
  m_stopMsg.clear();
}

void MaterialLibrary::stopBuild() {
  const int left = (int)(m_queue.size() + m_inflight.size());
  m_queue.clear();
  m_building = false;
  if (left > 0)
    m_stopMsg = "thumbnails stopped (" + std::to_string(left) + " left)";
}

void MaterialLibrary::tick() {
  if (!m_building && m_inflight.empty())
    return;
  // reap finished workers
  for (auto it = m_inflight.begin(); it != m_inflight.end();) {
    if (it->second.wait_for(std::chrono::seconds(0)) ==
        std::future_status::ready) {
      if (!it->second.get()) {
        // write a placeholder so failures aren't retried forever
        GenTexture black;
        black.Init(4, 4);
        SaveImage(black, thumbPath(it->first).c_str());
      }
      m_done++;
      for (auto& e : m_entries)
        if (e.path == it->first) {
          if (e.texTried && e.tex.id != 0)
            UnloadTexture(e.tex);
          e.tex = Texture2D{};
          e.texTried = false;  // reload on the next grid pass
        }
      it = m_inflight.erase(it);
    } else {
      ++it;
    }
  }
  // keep the worker pool full
  const unsigned hw = std::thread::hardware_concurrency();
  const int workers = hw > 4 ? 4 : (hw > 1 ? (int)hw - 1 : 1);
  while (m_building && (int)m_inflight.size() < workers && !m_queue.empty()) {
    std::string path = m_queue.front();
    m_queue.pop_front();
    if (fs::exists(thumbPath(path))) {
      m_done++;
      continue;
    }
    m_inflight.emplace_back(
        path, std::async(std::launch::async, buildThumbnail, path));
  }
  if (m_building && m_queue.empty() && m_inflight.empty()) {
    m_building = false;
    m_stopMsg.clear();
  }
}

std::string MaterialLibrary::statusText() const {
  if (m_building || !m_inflight.empty())
    return "thumbnails " + std::to_string(m_done) + "/" +
           std::to_string(m_total);
  return m_stopMsg;
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
  GraphEval ev;
  if (!ev.load(j) || !ev.run())
    return false;
  GenTexture* fin = ev.finalOutput();
  if (!fin || !fin->Data)
    return false;
  if (!saveThumbnail(*fin, file))
    return false;

  // full-res render cache: the final output plus one PNG per node, so
  // loading this project can show everything pre-rendered instead of
  // re-evaluating (ids match NodeGraph::save() — j is NOT flattened).
  std::string dir = cacheDir(file);
  std::error_code ec;
  fs::create_directories(dir, ec);
  SaveImage(*fin, (dir + "/output.png").c_str());
  dumpNodePreviews(j, ev, dir);
  return true;
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

  // batch thumbnail build controls (the batch itself advances in
  // tick(), which the Ide calls every frame — collapsing this section
  // no longer pauses it)
  if (m_building || !m_inflight.empty()) {
    char overlay[64];
    snprintf(overlay, sizeof(overlay), "%d / %d", m_done, m_total);
    ImGui::ProgressBar(m_total > 0 ? (float)m_done / m_total : 0.0f,
                       ImVec2(-56.0f, 0.0f), overlay);
    ImGui::SameLine();
    if (ImGui::SmallButton("Stop##libbuild"))
      stopBuild();
    if (!m_inflight.empty()) {
      std::string names;
      for (auto& f : m_inflight) {
        std::string n = fs::path(f.first).stem().string();
        names += (names.empty() ? "" : ", ") + n;
      }
      ImGui::TextDisabled("rendering: %s", names.c_str());
    }
  } else {
    int missing = 0;
    for (auto& e : m_entries)
      if (!fs::exists(thumbPath(e.path)))
        missing++;
    if (missing > 0) {
      char label[64];
      snprintf(label, sizeof(label), "Build %d missing thumbnail%s", missing,
               missing == 1 ? "" : "s");
      if (ImGui::SmallButton(label))
        startBuild(false);
      ImGui::SameLine();
    }
    // full rebuild: for when the engine's rendering changed and every
    // cached thumbnail is stale
    if (ImGui::SmallButton("Rebuild all##libthumbs"))
      startBuild(true);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Delete every thumbnail and re-render them all");
    if (!m_stopMsg.empty()) {
      ImGui::TextDisabled("%s", m_stopMsg.c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("Resume##libbuild"))
        startBuild(false);
    }
  }

  // thumbnail grid — its own scrolling region so a long material list
  // doesn't push the outer window's toolbar out of view when scrolled
  ImGui::BeginChild("##libgrid", ImVec2(0, 0), true);
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
  ImGui::EndChild();
  return activated;
}
