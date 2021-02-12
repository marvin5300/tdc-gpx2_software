cmake_minimum_required(VERSION 3.10)
project(gpx2 LANGUAGES CXX C)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_library(PIGPIOD_IF2 pigpiod_if2 REQUIRED)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -O3
)

configure_file(
    "${PROJECT_CONFIG_DIR}/version.h"
    "${PROJECT_HEADER_DIR}/version.h"
)

set(GPX2_SOURCE_FILES
    "${PROJECT_SRC_DIR}/gpx2.cpp"
    "${PROJECT_SRC_DIR}/config.cpp"
)

set(GPX2_HEADER_FILES
    "${PROJECT_HEADER_DIR}/gpx2.h"
    "${PROJECT_HEADER_DIR}/config.h"
)

add_definitions(-DGPX2_LIBRARY_EXPORT)

add_library(gpx2 STATIC ${GPX2_SOURCE_FILES} ${GPX2_HEADER_FILES})
add_library(gpx2_shared SHARED ${GPX2_SOURCE_FILES} ${GPX2_HEADER_FILES})

target_include_directories(gpx2 PUBLIC
    ${PROJECT_HEADER_DIR}
)

target_include_directories(gpx2_shared PUBLIC
    ${PROJECT_HEADER_DIR}
)
target_link_libraries(gpx2
    pigpiod_if2
)

target_link_libraries(gpx2_shared
    pigpiod_if2
)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

install(TARGETS gpx2 DESTINATION lib COMPONENT gpx2)
install(TARGETS gpx2_shared DESTINATION lib COMPONENT gpx2)
install(FILES ${PROJECT_HEADER_FILES} DESTINATION include COMPONENT gpx2)