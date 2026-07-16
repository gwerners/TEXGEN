// CLI tool: reads a TEXGEN project .json (or a Material Maker .ptex,
// converted on the fly) and renders to TGA/PNG.
// Usage: texgen_render <project.json|project.ptex> [output.tga]
// No UI dependencies.
#include "HeadlessEval.h"
#include "texgen.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <project.json> [output.tga]\n", argv[0]);
    return 1;
  }

  std::ifstream f(argv[1]);
  if (!f.is_open()) {
    fprintf(stderr, "Cannot open: %s\n", argv[1]);
    return 1;
  }

  nlohmann::json j;
  try {
    f >> j;
  } catch (const std::exception& e) {
    fprintf(stderr, "Error: invalid JSON in %s: %s\n", argv[1], e.what());
    return 1;
  }

  // Material Maker projects are converted to TEXGEN graphs on the fly
  std::string inFile = argv[1];
  if (inFile.size() > 5 &&
      inFile.compare(inFile.size() - 5, 5, ".ptex") == 0) {
    std::string base = inFile;
    size_t slash = base.find_last_of("/\\");
    if (slash != std::string::npos)
      base = base.substr(slash + 1);
    base = base.substr(0, base.size() - 5);
    std::vector<std::string> skipped;
    j = ptexToTexgen(j, base, &skipped);
    if (!skipped.empty()) {
      fprintf(stderr, "note: %zu unsupported node(s) skipped\n",
              skipped.size());
    }
  }

  GenTexture output;
  if (!headlessEvaluate(j, output)) {
    fprintf(stderr, "Evaluation failed (no Output node or empty result)\n");
    return 1;
  }

  // Determine output filename
  std::string outFile;
  if (argc >= 3) {
    outFile = argv[2];
  } else {
    // Default: replace .json with .tga
    outFile = argv[1];
    size_t dot = outFile.rfind('.');
    if (dot != std::string::npos)
      outFile = outFile.substr(0, dot);
    outFile += ".tga";
  }

  if (SaveImage(output, outFile.c_str())) {
    printf("OK: %s -> %s (%dx%d)\n", argv[1], outFile.c_str(), output.XRes,
           output.YRes);
    return 0;
  }

  fprintf(stderr, "Failed to save: %s\n", outFile.c_str());
  return 1;
}
