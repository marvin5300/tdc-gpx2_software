cmake_minimum_required (VERSION 3.10)
project (gpx2)

option(BUILD_TIDY "Use clang tidy to build" OFF)
option(BUILD_TOOLS "Build additional tools (like readout program) in addition to the library" ON)
option(BUILD_DOCS "Build doxygen files" OFF)

set(CMAKE_BUILD_TYPE Release)
message("${CMAKE_HOST_SYSTEM_PROCESSOR}")
add_subdirectory(spi)
if (BUILD_TOOLS)
add_subdirectory(gpx2-raspi-readout-program)
endif (BUILD_TOOLS)

