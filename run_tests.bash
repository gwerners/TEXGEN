#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
ROOT=$(pwd)

BUILD_DIR="build"
TESTS_DIR="tests"
EXAMPLES_DIR="examples"
EXPORT_TOOL="$BUILD_DIR/tests/texgen_export"
GEN_DIR="$BUILD_DIR/tests/generated"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "=== TEXGEN Export C Test Suite ==="
echo ""

# Step 1: Build the project (if not already built)
if [ ! -f "$BUILD_DIR/lib/libtexgen.a" ] || [ ! -f "$BUILD_DIR/agg/agg_lib/libagg.a" ]; then
  echo -e "${YELLOW}Building project...${NC}"
  mkdir -p "$BUILD_DIR"
  pushd "$BUILD_DIR" > /dev/null
    cmake .. > /dev/null 2>&1
    make -j$(nproc) texgen texgen_export agg 2>&1 | tail -3
  popd > /dev/null
fi

# Step 2: Build the export tool if needed
if [ ! -f "$EXPORT_TOOL" ]; then
  echo -e "${YELLOW}Building export tool...${NC}"
  pushd "$BUILD_DIR" > /dev/null
    make -j$(nproc) texgen_export 2>&1 | tail -3
  popd > /dev/null
fi

# Step 3: Generate headers and test mains
mkdir -p "$GEN_DIR"
TEMPLATE="$TESTS_DIR/test_template.cpp.in"

EXAMPLES=(
  "01_basic_shapes:basic_shapes"
  "02_star_polygon:star_polygon"
  "03_text_on_noise:text_on_noise"
  "04_procedural_brick:procedural_brick"
  "05_crystal_glow:crystal_glow"
  "06_badge_composition:badge_composition"
  "07_bezier_curves:bezier_curves"
  "08_dashed_arc:dashed_arc"
  "09_gradient_fill:gradient_fill"
)

PASS=0
FAIL=0
TOTAL=${#EXAMPLES[@]}

for entry in "${EXAMPLES[@]}"; do
  JSON_BASE="${entry%%:*}"
  FUNC_NAME="${entry##*:}"
  JSON_FILE="$EXAMPLES_DIR/${JSON_BASE}.json"
  HEADER="$GEN_DIR/${FUNC_NAME}.h"
  TEST_CPP="$GEN_DIR/test_${FUNC_NAME}.cpp"
  TEST_BIN="$GEN_DIR/test_${FUNC_NAME}"

  echo -n "  ${JSON_BASE}... "

  # Generate .h from .json
  if ! "$EXPORT_TOOL" "$JSON_FILE" "$FUNC_NAME" "$HEADER" > /dev/null 2>&1; then
    echo -e "${RED}FAIL (export)${NC}"
    FAIL=$((FAIL + 1))
    continue
  fi

  # Generate test main from template
  sed "s/@NAME@/${FUNC_NAME}/g" "$TEMPLATE" > "$TEST_CPP"

  # Compile test (link with libtexgen + libagg, no raylib)
  if ! g++ -std=c++17 \
      -I "$GEN_DIR" -I lib -I work -I work/ktg -I agg/agg_lib/include \
      -o "$TEST_BIN" "$TEST_CPP" \
      "$BUILD_DIR/lib/libtexgen.a" "$BUILD_DIR/agg/agg_lib/libagg.a" \
      -lm 2> "$GEN_DIR/${FUNC_NAME}_compile.log"; then
    echo -e "${RED}FAIL (compile)${NC}"
    cat "$GEN_DIR/${FUNC_NAME}_compile.log"
    FAIL=$((FAIL + 1))
    continue
  fi

  # Run test
  pushd "$GEN_DIR" > /dev/null
  if OUTPUT=$(./test_${FUNC_NAME} 2>&1); then
    echo -e "${GREEN}${OUTPUT}${NC}"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}FAIL (runtime): ${OUTPUT}${NC}"
    FAIL=$((FAIL + 1))
  fi
  popd > /dev/null
done

echo ""
echo "=== Results: ${PASS}/${TOTAL} passed, ${FAIL} failed ==="

if [ $FAIL -ne 0 ]; then
  exit 1
fi
