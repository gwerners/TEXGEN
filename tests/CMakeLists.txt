# CLI tools — no UI dependencies (no raylib, imgui, ImNodes).

# texgen_export: JSON -> C header
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

# texgen_render: JSON -> TGA (headless evaluation)
add_executable(texgen_render
  texgen_render.cpp
)
target_link_libraries(texgen_render PRIVATE texgen)
target_compile_options(texgen_render PRIVATE -w)
