// HeadlessEval — JSON-project evaluation via GraphEval/CoreNodeRegistry.
// Every registered node type is supported; the logic executed here is the
// exact same CoreNode::execute() code the editor runs.
#include "HeadlessEval.h"

#include "GraphEval.h"

bool headlessEvaluate(const nlohmann::json &project, GenTexture &output) {
  GraphEval ge;
  if (!ge.load(project))
    return false;
  ge.run();
  GenTexture *result = ge.finalOutput();
  if (result && result->Data) {
    output = *result;
    return true;
  }
  return false;
}
