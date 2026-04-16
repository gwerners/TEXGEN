# libtexgen — procedural texture generation library (no UI dependencies).

set(TEXGEN_WORK "${CMAKE_SOURCE_DIR}/work")

set(TEXGEN_LIB_SOURCES
  ${TEXGEN_WORK}/ktg/gentexture.cpp
  ${TEXGEN_WORK}/extra_generators.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/texgen_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/HeadlessEval.cpp
)

set(TEXGEN_LIB_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TEXGEN_WORK}
  ${TEXGEN_WORK}/ktg
  ${CMAKE_SOURCE_DIR}/json/include
)

# Static library
add_library(texgen STATIC ${TEXGEN_LIB_SOURCES})
target_include_directories(texgen PUBLIC ${TEXGEN_LIB_INCLUDES})
target_link_libraries(texgen PUBLIC agg)
target_compile_options(texgen PRIVATE -w)

# Shared library
add_library(texgen_shared SHARED ${TEXGEN_LIB_SOURCES})
target_include_directories(texgen_shared PUBLIC ${TEXGEN_LIB_INCLUDES})
target_link_libraries(texgen_shared PUBLIC agg_shared)
target_compile_options(texgen_shared PRIVATE -w)
set_target_properties(texgen_shared PROPERTIES
  OUTPUT_NAME texgen
  POSITION_INDEPENDENT_CODE ON
)
