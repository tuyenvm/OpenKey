cmake_minimum_required(VERSION 3.10)

# Dynamic version extraction from Git tag (Zero Hardcode Policy)
# Strips 'v' prefix and '-linux' suffix: v2.0.3-linux -> 2.0.3
execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_TAG_RESULT
)

if(GIT_TAG_RESULT EQUAL 0 AND GIT_TAG)
    # Strip 'v' prefix
    string(REGEX REPLACE "^v" "" GIT_VERSION "${GIT_TAG}")
    # Strip '-linux' or any other suffix after the version digits
    string(REGEX REPLACE "-.*$" "" GIT_VERSION "${GIT_VERSION}")
else()
    set(GIT_VERSION "0.0.0")
    message(WARNING "No Git tag found. Using fallback version: ${GIT_VERSION}")
endif()

message(STATUS "OpenKey version from Git: ${GIT_VERSION}")
project(OpenKey VERSION ${GIT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source directories
set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Sources/OpenKey/engine)
set(LINUX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Sources/OpenKey/linux)

# Include paths
include_directories(${ENGINE_DIR})
include_directories(${ENGINE_DIR}/platforms)

# Engine source files (shared across platforms)
set(ENGINE_SOURCES
    ${ENGINE_DIR}/Engine.cpp
    ${ENGINE_DIR}/Vietnamese.cpp
    ${ENGINE_DIR}/Macro.cpp
    ${ENGINE_DIR}/SmartSwitchKey.cpp
    ${ENGINE_DIR}/ConvertTool.cpp
)

# Linux CLI executable
add_executable(openkey-cli
    ${LINUX_DIR}/main.cpp
    ${ENGINE_SOURCES}
)

# LINUX macro activates platforms/linux.h via DataType.h
target_compile_definitions(openkey-cli PRIVATE LINUX)

# Fcitx5 Integration Module
add_subdirectory(${LINUX_DIR}/fcitx5)

# Qt5 Native UI Control Center
add_subdirectory(${LINUX_DIR}/ui)

# Installation rules
install(TARGETS openkey-cli DESTINATION bin)

# Packaging (CPack)
set(CPACK_PACKAGE_NAME "openkey")
set(CPACK_PACKAGE_VENDOR "OpenKey Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenKey Vietnamese IME for Linux (Wayland/X11)")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Tam Le <tamld@example.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_RPM_PACKAGE_REQUIRES "fcitx5, qt5-qtbase, qt5-qtsvg")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "fcitx5, libqt5core5a, libqt5gui5, libqt5widgets5, libqt5svg5")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Sources/OpenKey/linux/pkg/postinst.sh")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Sources/OpenKey/linux/pkg/postinst.sh")

set(CPACK_GENERATOR "DEB;RPM")

# Dynamic architecture detection for packaging
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    set(PACKAGE_ARCH "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(PACKAGE_ARCH "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
else()
    set(PACKAGE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-${PACKAGE_ARCH}")

include(CPack)
