#pragma once
#include <nlohmann/json.hpp>
#include <string>

class NodeGraph;

// Export the current node graph as a standalone C header file.
// The generated header contains a single function that reproduces
// the texture generation pipeline using libtexgen calls.
//   graph:      the node graph to export
//   name:       function/file prefix (e.g. "my_texture")
//   outPath:    output file path (e.g. "my_texture.h")
// Returns true on success.
bool exportCHeader(NodeGraph *graph, const std::string &name,
                   const std::string &outPath);

// Export from a project JSON directly (no NodeGraph/UI required).
bool exportCHeaderFromJSON(const nlohmann::json &project,
                           const std::string &name,
                           const std::string &outPath);
