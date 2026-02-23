# CMake configuration for OpenKey Native UI (Qt5)
cmake_minimum_required(VERSION 3.10)
project(openkey-linux-ui VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5 core modules
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# Create the UI executable
add_executable(openkey-linux-ui
    main.cpp
)

# Link Qt5 libraries
target_link_libraries(openkey-linux-ui PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

# Installation rules
install(TARGETS openkey-linux-ui DESTINATION bin)
