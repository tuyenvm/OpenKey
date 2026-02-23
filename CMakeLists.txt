cmake_minimum_required(VERSION 3.10)
project(OpenKey VERSION 2.0.0 LANGUAGES CXX)

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
set(CPACK_PACKAGE_NAME "openkey-cli")
set(CPACK_PACKAGE_VENDOR "OpenKey Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenKey Engine CLI for Linux")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Tam Le <tamld@example.com>") # Placeholder contact
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_GENERATOR "DEB;TGZ") # Generate .deb and .tar.gz
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x86_64")

include(CPack)
