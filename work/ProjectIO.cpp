#include "ProjectIO.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include "Nodes.h"
#include "PtexImport.h"

bool saveProject(const std::string& filepath) {
  if (!g_nodeGraph)
    return false;
  nlohmann::json j = g_nodeGraph->save();
  std::ofstream f(filepath);
  if (!f.is_open())
    return false;
  f << j.dump(2);
  return true;
}

bool loadProject(const std::string& filepath) {
  if (!g_nodeGraph)
    return false;
  std::ifstream f(filepath);
  if (!f.is_open())
    return false;
  try {
    nlohmann::json j;
    f >> j;
    g_nodeGraph->load(j);
  } catch (const std::exception& e) {
    fprintf(stderr, "loadProject: failed to load '%s': %s\n", filepath.c_str(),
            e.what());
    return false;
  }
  return true;
}

bool importPtexProject(const std::string& filepath) {
  if (!g_nodeGraph)
    return false;
  std::ifstream f(filepath);
  if (!f.is_open())
    return false;

  // base name from the file stem (drives Material/Output filenames)
  std::string base = filepath;
  size_t slash = base.find_last_of("/\\");
  if (slash != std::string::npos)
    base = base.substr(slash + 1);
  size_t dot = base.find_last_of('.');
  if (dot != std::string::npos)
    base = base.substr(0, dot);

  try {
    nlohmann::json ptex;
    f >> ptex;
    std::vector<std::string> skipped;
    nlohmann::json project = ptexToTexgen(ptex, base, &skipped);
    g_nodeGraph->load(project);
    if (!skipped.empty()) {
      std::map<std::string, int> counts;
      for (auto& s : skipped)
        counts[s]++;
      fprintf(stderr, "importPtex: '%s' — unsupported node types skipped:",
              base.c_str());
      for (auto& [type, n] : counts)
        fprintf(stderr, " %s(x%d)", type.c_str(), n);
      fprintf(stderr, "\n");
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "importPtex: failed to import '%s': %s\n", filepath.c_str(),
            e.what());
    return false;
  }
  return true;
}
