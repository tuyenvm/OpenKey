# CMake configuration for Fcitx5 OpenKey Addon
project(fcitx5-openkey VERSION ${OpenKey_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core)

# The Fcitx5 Addon Component
add_library(fcitx5-openkey MODULE
    openkey_engine.cpp
    # Include engine sources for static linking into the module, 
    # or link against a shared lib openkey-engine later if preferred.
    ${ENGINE_SOURCES}
)

# Fcitx5 expects the addon shared library to have no "lib" prefix
set_target_properties(fcitx5-openkey PROPERTIES PREFIX "")

target_compile_definitions(fcitx5-openkey PRIVATE LINUX QT_NO_KEYWORDS)
target_include_directories(fcitx5-openkey PRIVATE
    ${ENGINE_DIR}
    ${ENGINE_DIR}/platforms
)

target_link_libraries(fcitx5-openkey
    Fcitx5::Core
    Qt5::Core
)

include(GNUInstallDirs)

# Install shared library plugin to relative lib directory for packaging (multiarch aware)
install(TARGETS fcitx5-openkey DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")

# Configure and install the Fcitx5 Addon Config file so Fcitx5 can load the plugin
configure_file(openkey.conf.in ${CMAKE_CURRENT_BINARY_DIR}/openkey.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openkey.conf DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/addon")

# Configure and install the Fcitx5 InputMethod Config file so OpenKey appears in the GUI
configure_file(openkey-im.conf.in ${CMAKE_CURRENT_BINARY_DIR}/openkey-im.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openkey-im.conf DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod")
