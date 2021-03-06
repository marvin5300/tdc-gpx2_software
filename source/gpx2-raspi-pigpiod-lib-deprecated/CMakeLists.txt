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

# check if Doxygen is installed (from https://vicrucann.github.io/tutorials/quick-cmake-doxygen/)
find_package(Doxygen)
if (DOXYGEN_FOUND AND BUILD_DOCS)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
elseif (BUILD_DOCS)
    message("doxygen need to be installed to generate the doxygen documentation. for generating tree views graphviz is needed.")
else ()
    message("Selected option doxygen build OFF. Doxygen will not be build.")
endif ()

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

set_target_properties(
gpx2
PROPERTIES
SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)

include(GNUInstallDirs)
set(GPX2_PACKAGE_NAME "gpx2")
add_custom_target(changelog ALL COMMAND gzip -cn9 "${PROJECT_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
install(TARGETS gpx2 DESTINATION lib COMPONENT "${GPX2_PACKAGE_NAME}")
install(FILES ${GPX2_HEADER_FILES} DESTINATION include COMPONENT "${GPX2_PACKAGE_NAME}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT "${GPX2_PACKAGE_NAME}")
install(FILES "${PROJECT_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT "${GPX2_PACKAGE_NAME}")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_CONFIG_DIR}/copyright")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "pigpiod")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_CONFIG_DIR}/triggers")
set(CPACK_PACKAGE_VENDOR "Marvin Peter")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
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
set(CPACK_STRIP_FILES "${CMAKE_CURRENT_BINARY_DIR}/../lib/${GPX2_PACKAGE_NAME}")
set(CPACK_PACKAGE_NAME "${GPX2_PACKAGE_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${GPX2_PACKAGE_NAME}")
set(CPACK_SYSTEM_NAME "Linux-armv7l")
set(CPACK_TOPLEVEL_TAG  "Linux-armv7l")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

include(CPack)