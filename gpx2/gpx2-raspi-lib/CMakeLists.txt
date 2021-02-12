cmake_minimum_required(VERSION 3.10)
project(gpx2_objlib LANGUAGES CXX C)

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

add_definitions(-Dgpx2_objlib_LIBRARY_EXPORT)

add_library(gpx2_objlib OBJECT ${GPX2_SOURCE_FILES} ${GPX2_HEADER_FILES})

add_library(gpx2 SHARED $<TARGET_OBJECTS:gpx2_objlib>)
add_library(gpx2_static STATIC $<TARGET_OBJECTS:gpx2_objlib>)

target_include_directories(gpx2_objlib PUBLIC
    ${PROJECT_HEADER_DIR}
)

set_property(TARGET gpx2_objlib PROPERTY POSITION_INDEPENDENT_CODE 1)

target_link_libraries(gpx2_objlib
    pigpiod_if2
)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

include(GNUInstallDirs)
add_custom_target(changelog ALL COMMAND gzip -cn9 "${PROJECT_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
install(TARGETS gpx2 DESTINATION lib COMPONENT gpx2)
install(FILES ${GPX2_HEADER_FILES} DESTINATION include COMPONENT gpx2)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT gpx2)
install(FILES "${PROJECT_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT gpx2)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "pigpiod")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_CONFIG_DIR}/triggers")
set(CPACK_PACKAGE_VENDOR "Marvin Peter")
set(CPACK_DEBIAN_PACKAGE_SECTION "libraries")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/marvin5300/GPX2-TDC")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for GPX2 TDC control")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION " It makes connecting to TDC board possible.
 It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Marvin Peter <marvin.peter@physik.uni-giessen.de>")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../packages/")

include(CPack)