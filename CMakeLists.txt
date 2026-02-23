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
