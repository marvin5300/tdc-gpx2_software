cmake_minimum_required(VERSION 3.10)
project(spi LANGUAGES CXX C)

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
    add_custom_target( spi_doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
elseif (BUILD_DOCS)
    message("doxygen need to be installed to generate the doxygen documentation. for generating tree views graphviz is needed.")
else ()
    message("Selected option doxygen build OFF. Doxygen will not be build.")
endif ()

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

set(PROJECT_SOURCE_FILES
    "${PROJECT_SRC_DIR}/spidevices/spidevice.cpp"
    "${PROJECT_SRC_DIR}/spidevices/gpx2/gpx2.cpp"
)

set(PROJECT_HEADER_FILES
    "${PROJECT_HEADER_DIR}/spidevices/spidevice.h"
    "${PROJECT_HEADER_DIR}/spidevices/gpx2/gpx2.h"
)

add_definitions(-Dspi_objlib_LIBRARY_EXPORT)

add_library(spi_objlib OBJECT ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})

add_library(spi SHARED $<TARGET_OBJECTS:spi_objlib>)
add_library(spi_static STATIC $<TARGET_OBJECTS:spi_objlib>)

target_include_directories(spi_objlib PUBLIC
    ${PROJECT_HEADER_DIR}
)

set_property(TARGET spi_objlib PROPERTY POSITION_INDEPENDENT_CODE 1)

#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set_target_properties(
spi
PROPERTIES
SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)

include(GNUInstallDirs)
set(SPI_PACKAGE_NAME "spi")
add_custom_target(changelog ALL COMMAND gzip -cn9 "${PROJECT_CONFIG_DIR}/changelog" > "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz")
install(TARGETS spi DESTINATION lib COMPONENT "${SPI_PACKAGE_NAME}")
install(FILES ${PROJECT_HEADER_FILES} DESTINATION include COMPONENT "${SPI_PACKAGE_NAME}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/changelog.gz" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT "${SPI_PACKAGE_NAME}")
install(FILES "${PROJECT_CONFIG_DIR}/copyright" DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT "${SPI_PACKAGE_NAME}")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_CONFIG_DIR}/copyright")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_CONFIG_DIR}/license")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_CONFIG_DIR}/triggers")
set(CPACK_PACKAGE_VENDOR "Marvin Peter")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/marvin5300/SPI-TDC")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for SPI control")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION " It makes connecting to SPI devices possible.
 It uses the linux spidev c api.
 It is licensed under the GNU Lesser General Public License version 3 (LGPL v3).")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Marvin Peter <marvin.peter@physik.uni-giessen.de>")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../packages/")
set(CPACK_STRIP_FILES "${CMAKE_CURRENT_BINARY_DIR}/../lib/${SPI_PACKAGE_NAME}")
set(CPACK_PACKAGE_NAME "${SPI_PACKAGE_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${SPI_PACKAGE_NAME}")
set(CPACK_SYSTEM_NAME "Linux-armv7l")
set(CPACK_TOPLEVEL_TAG  "Linux-armv7l")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

include(CPack)