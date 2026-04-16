#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class FileDialog {
 public:
  enum Mode { Save, Load };

  // Opens the dialog. Call from your ImGui frame.
  // Returns true when the user confirms a selection.
  bool show(const char* title, Mode mode, const char* extension) {
    if (!m_open)
      return false;

    bool result = false;
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(title, &m_open)) {
      // Current directory display
      ImGui::TextWrapped("Dir: %s", m_currentDir.c_str());

      // Go up button
      if (ImGui::Button("..##up")) {
        fs::path parent = fs::path(m_currentDir).parent_path();
        if (!parent.empty()) {
          navigateTo(parent.string());
          scanDir(m_extension.empty() ? extension : m_extension.c_str());
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Refresh"))
        scanDir(m_extension.empty() ? extension : m_extension.c_str());

      ImGui::Separator();

      // File list
      ImVec2 listSize(0, -60);
      if (ImGui::BeginChild("##filelist", listSize, true)) {
        for (auto& entry : m_entries) {
          bool isDir = entry.isDir;
          std::string label =
              isDir ? "[" + entry.name + "]" : entry.name;

          if (ImGui::Selectable(label.c_str(), entry.name == m_selected)) {
            if (isDir) {
              navigateTo((fs::path(m_currentDir) / entry.name).string());
              scanDir(m_extension.empty() ? extension : m_extension.c_str());
            } else {
              m_selected = entry.name;
              strncpy(m_filename, entry.name.c_str(), sizeof(m_filename) - 1);
            }
          }
        }
      }
      ImGui::EndChild();

      // Filename input
      ImGui::PushItemWidth(-120);
      ImGui::InputText("##filename", m_filename, sizeof(m_filename));
      ImGui::PopItemWidth();
      ImGui::SameLine();

      const char* btnLabel = (mode == Save) ? "Save" : "Open";
      if (ImGui::Button(btnLabel, ImVec2(100, 0))) {
        if (strlen(m_filename) > 0) {
          m_resultPath = (fs::path(m_currentDir) / m_filename).string();
          result = true;
          m_open = false;
        }
      }
    }
    ImGui::End();
    return result;
  }

  void open(const char* defaultFilename, const char* extension = ".json") {
    m_open = true;
    m_selected.clear();
    m_extension = extension ? extension : "";
    strncpy(m_filename, defaultFilename, sizeof(m_filename) - 1);
    m_currentDir = fs::current_path().string();
    scanDir(extension);
  }

  bool isOpen() const { return m_open; }

  const std::string& getResultPath() const { return m_resultPath; }

 private:
  struct Entry {
    std::string name;
    bool isDir;
  };

  void navigateTo(const std::string& dir) {
    if (fs::is_directory(dir))
      m_currentDir = dir;
  }

  void scanDir(const char* extension) {
    m_entries.clear();
    // Build set of accepted extensions from comma-separated string
    // e.g. ".png,.jpg,.tga,.bmp" or single ".json"
    std::vector<std::string> exts;
    if (extension && strlen(extension) > 0) {
      std::string s(extension);
      size_t pos = 0;
      while (pos < s.size()) {
        size_t comma = s.find(',', pos);
        if (comma == std::string::npos) comma = s.size();
        std::string ext = s.substr(pos, comma - pos);
        // trim whitespace
        while (!ext.empty() && ext.front() == ' ') ext.erase(ext.begin());
        while (!ext.empty() && ext.back() == ' ') ext.pop_back();
        if (!ext.empty()) exts.push_back(ext);
        pos = comma + 1;
      }
    }
    try {
      for (auto& p : fs::directory_iterator(m_currentDir)) {
        Entry e;
        e.name = p.path().filename().string();
        e.isDir = p.is_directory();

        if (e.name[0] == '.' && e.name != "..")
          continue;

        if (!e.isDir && !exts.empty()) {
          std::string fileExt = p.path().extension().string();
          // case-insensitive comparison
          std::string lower;
          lower.resize(fileExt.size());
          std::transform(fileExt.begin(), fileExt.end(), lower.begin(), ::tolower);
          bool match = false;
          for (auto& ext : exts) {
            std::string extLower;
            extLower.resize(ext.size());
            std::transform(ext.begin(), ext.end(), extLower.begin(), ::tolower);
            if (lower == extLower) { match = true; break; }
          }
          if (!match) continue;
        }
        m_entries.push_back(e);
      }
    } catch (...) {
    }
    std::sort(m_entries.begin(), m_entries.end(),
              [](const Entry& a, const Entry& b) {
                if (a.isDir != b.isDir)
                  return a.isDir > b.isDir;
                return a.name < b.name;
              });
  }

  bool m_open = false;
  std::string m_currentDir;
  std::string m_selected;
  std::string m_resultPath;
  std::string m_extension;
  char m_filename[256] = {};
  std::vector<Entry> m_entries;
};

#endif  // FILEDIALOG_H
