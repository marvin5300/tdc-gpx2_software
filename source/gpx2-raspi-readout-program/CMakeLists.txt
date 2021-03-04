cmake_minimum_required(VERSION 3.10)
project(readout LANGUAGES CXX C)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
set(LIBRARY_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../gpx2-raspi-lib/include/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(${BUILD_TIDY})
    set(CMAKE_CXX_CLANG_TIDY
      clang-tidy;
      -header-filter=^global;
      -checks=-*,readability-*,bugprone-*,performace-*,clang-analyzer-*,modernize-*,hicpp-*;
    )
endif(${BUILD_TIDY})

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

set(READOUT_HEADER_FILES
    "${PROJECT_HEADER_DIR}/gpio.h"
)
set(READOUT_SOURCE_FILES
    "${PROJECT_SRC_DIR}/main.cpp"
    "${PROJECT_SRC_DIR}/gpio.cpp"
)

add_executable(readout ${READOUT_SOURCE_FILES} ${READOUT_HEADER_FILES})

target_include_directories(readout PUBLIC
    ${PROJECT_HEADER_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../spi/include/"
)

target_link_directories(readout PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/../lib/")
target_link_libraries(readout
    spi_static
    gpiod
)

#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
