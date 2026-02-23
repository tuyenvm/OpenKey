# CMake configuration for Fcitx5 OpenKey Addon
project(fcitx5-openkey VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core REQUIRED)

# The Fcitx5 Addon Component
add_library(fcitx5-openkey MODULE
    openkey_engine.cpp
    # Include engine sources for static linking into the module, 
    # or link against a shared lib openkey-engine later if preferred.
    ${ENGINE_SOURCES}
)

target_compile_definitions(fcitx5-openkey PRIVATE LINUX)
target_include_directories(fcitx5-openkey PRIVATE
    ${ENGINE_DIR}
    ${ENGINE_DIR}/platforms
)

target_link_libraries(fcitx5-openkey
    Fcitx5::Core
)

# Standard installation path for Fcitx5 addons
install(TARGETS fcitx5-openkey DESTINATION "${FCITX_INSTALL_ADDONDIR}")
