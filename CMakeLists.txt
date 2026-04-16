cmake_minimum_required(VERSION 3.16)
project(all-stuff)


set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(WORK_PATH "${PROJECT_SOURCE_DIR}/work")

################################
# AGG 2.4 (library only, no examples)
################################
set(AGG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(agg/agg_lib)

################################
# libtexgen (core library, no UI)
################################
add_subdirectory(lib)

################################
# tests (CLI export tool, no UI)
################################
add_subdirectory(tests)

################################
# work (editor application)
################################
add_subdirectory(work)
