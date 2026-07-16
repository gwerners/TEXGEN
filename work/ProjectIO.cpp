#include "ProjectIO.h"
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Nodes.h"

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
    fprintf(stderr, "loadProject: failed to load '%s': %s\n",
            filepath.c_str(), e.what());
    return false;
  }
  return true;
}
