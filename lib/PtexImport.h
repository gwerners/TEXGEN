#pragma once
// PtexImport — converts Material Maker .ptex projects to TEXGEN graphs.
// C++ port of tools/ptex2texgen.py (this implementation is canonical).
// Pure JSON transform, no UI dependencies.

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Convert a parsed .ptex project into TEXGEN project JSON.
// baseName is used for the Material node's output files and the preview
// Output node. Unmapped Material Maker node types are skipped and their
// names appended to skippedTypes (if given) — the rest of the graph is
// still converted.
nlohmann::json ptexToTexgen(const nlohmann::json &ptex,
                            const std::string &baseName,
                            std::vector<std::string> *skippedTypes = nullptr);
