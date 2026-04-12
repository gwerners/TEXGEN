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
        if (!parent.empty())
          navigateTo(parent.string());
      }
      ImGui::SameLine();
      if (ImGui::Button("Refresh"))
        scanDir(extension);

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
              scanDir(extension);
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

  void open(const char* defaultFilename) {
    m_open = true;
    m_selected.clear();
    strncpy(m_filename, defaultFilename, sizeof(m_filename) - 1);
    m_currentDir = fs::current_path().string();
    scanDir(".json");
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
    try {
      for (auto& p : fs::directory_iterator(m_currentDir)) {
        Entry e;
        e.name = p.path().filename().string();
        e.isDir = p.is_directory();

        if (e.name[0] == '.' && e.name != "..")
          continue;

        if (!e.isDir && extension && strlen(extension) > 0) {
          if (p.path().extension().string() != extension)
            continue;
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
  char m_filename[256] = {};
  std::vector<Entry> m_entries;
};

#endif  // FILEDIALOG_H
