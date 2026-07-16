#pragma once
#include <string>

bool saveProject(const std::string& filepath);
bool loadProject(const std::string& filepath);

// Import a Material Maker .ptex project, converting it to TEXGEN nodes.
// Unmapped node types are reported on stderr and skipped.
bool importPtexProject(const std::string& filepath);
