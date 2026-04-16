# texgen_export CLI tool — generates C headers from project JSON files.
# No UI dependencies (no raylib, imgui, ImNodes).

add_executable(texgen_export
  texgen_export.cpp
  ${CMAKE_SOURCE_DIR}/work/CExport.cpp
)
target_include_directories(texgen_export PRIVATE
  ${CMAKE_SOURCE_DIR}/work
  ${CMAKE_SOURCE_DIR}/work/ktg
  ${CMAKE_SOURCE_DIR}/json/include
)
target_compile_definitions(texgen_export PRIVATE TEXGEN_NO_UI)
target_compile_options(texgen_export PRIVATE -w)
