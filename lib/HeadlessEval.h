#pragma once
// Headless texture pipeline evaluator.
// Reads a TEXGEN project JSON, evaluates the node graph, and returns
// the output texture. No UI dependencies (no raylib, imgui, ImNodes).

#include <nlohmann/json.hpp>
#include <string>
#include "gentexture.hpp"

// Evaluate a TEXGEN project JSON and return the final output texture.
// Returns true if a valid output was produced.
bool headlessEvaluate(const nlohmann::json &project, GenTexture &output);
