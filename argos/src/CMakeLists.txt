# Minimum version
cmake_minimum_required(VERSION 2.8.12)

# Project name
project(foraging)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Disable MacOSX rpath
set(CMAKE_MACOSX_RPATH 0)

# Check for ARGoS3
find_package(PkgConfig)
pkg_check_modules(ARGOS REQUIRED argos3_simulator)

# Add ARGoS paths
include_directories(${ARGOS_INCLUDE_DIRS})
link_directories(${ARGOS_LIBRARY_DIRS})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ARGOS_PREFIX}/share/argos3/cmake)

# Check for Lua and add include path
find_package(Lua52 REQUIRED)
include_directories(${LUA_INCLUDE_DIR})

# Check for QT and OpenGL
include(ARGoSCheckQTOpenGL)

# Create the loop function library
add_library(foraging_loop_functions MODULE
    foraging_loop_functions.h
    foraging_loop_functions.cpp
    foraging_qt_user_functions.h
    foraging_qt_user_functions.cpp)

target_link_libraries(foraging_loop_functions
    argos3core_simulator
    argos3plugin_simulator_dynamics2d
    argos3plugin_simulator_entities
    argos3plugin_simulator_footbot
    argos3plugin_simulator_genericrobot
    argos3plugin_simulator_media
    argos3plugin_simulator_qtopengl
    ${ARGOS_QTOPENGL_LIBRARIES}
    ${LUA_LIBRARIES})
