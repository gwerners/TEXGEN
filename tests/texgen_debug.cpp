// CLI tool: evaluates a TEXGEN project (or a Material Maker .ptex,
// converted on the fly) and dumps EVERY node's first output as a PNG
// into a directory — for auditing a graph chain node by node.
// Usage: texgen_debug <project.json|project.ptex> <out_dir>
// Files are named <id>_<typeName>.png in evaluation order.
#include "GraphEval.h"
#include "texgen.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <project.json|.ptex> <out_dir>\n", argv[0]);
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
  } catch (const std::exception &e) {
    fprintf(stderr, "Error: invalid JSON in %s: %s\n", argv[1], e.what());
    return 1;
  }

  std::string inFile = argv[1];
  if (inFile.size() > 5 &&
      inFile.compare(inFile.size() - 5, 5, ".ptex") == 0) {
    std::vector<std::string> skipped;
    j = ptexToTexgen(j, "debug", &skipped);
    if (!skipped.empty())
      fprintf(stderr, "note: %zu unsupported node(s) skipped\n",
              skipped.size());
  }

  // expose subgraph inner nodes too
  j = flattenSubgraphs(applyRemotes(j));

  mkdir(argv[2], 0755);

  {
    std::ofstream cj(std::string(argv[2]) + "/converted.json");
    cj << j.dump(1);
  }

  GraphEval ev;
  if (!ev.load(j)) {
    fprintf(stderr, "load failed\n");
    return 1;
  }
  if (!ev.run()) {
    fprintf(stderr, "run failed\n");
    return 1;
  }

  int saved = dumpNodePreviews(j, ev, argv[2]);
  printf("OK: %d node previews -> %s\n", saved, argv[2]);
  return 0;
}
