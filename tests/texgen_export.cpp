// CLI tool: reads a TEXGEN project .json and exports a C header.
// Usage: texgen_export <project.json> <function_name> <output.h>
// No UI dependencies — uses only nlohmann/json and CExport logic.
#include "CExport.h"
#include <fstream>
#include <cstdio>

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
  f >> j;

  if (exportCHeaderFromJSON(j, argv[2], argv[3])) {
    printf("OK: %s -> %s\n", argv[1], argv[3]);
    return 0;
  }

  fprintf(stderr, "Export failed\n");
  return 1;
}
