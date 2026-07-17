// CLI tool: reads a TEXGEN project .json (or a Material Maker .ptex,
// converted on the fly) and exports a C header.
// Usage: texgen_export <project.json|project.ptex> <function_name> <output.h>
// No UI dependencies — uses only nlohmann/json and CExport logic.
#include "CExport.h"
#include "PtexImport.h"
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <project.json> <func_name> <output.h>\n",
            argv[0]);
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
    try {
      j = ptexToTexgen(j, base, &skipped);
    } catch (const std::exception& e) {
      fprintf(stderr, "Error converting %s: %s\n", argv[1], e.what());
      return 1;
    }
    if (!skipped.empty()) {
      fprintf(stderr, "note: %zu unsupported node(s) skipped\n",
              skipped.size());
    }
  }

  if (exportCHeaderFromJSON(j, argv[2], argv[3])) {
    printf("OK: %s -> %s\n", argv[1], argv[3]);
    return 0;
  }

  fprintf(stderr, "Export failed\n");
  return 1;
}
