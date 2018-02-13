#
#   This file is part of Magnum.
#
#   Original authors — credit is appreciated but not required:
#
#       2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
#           Vladimír Vondruš <mosra@centrum.cz>
#       2013 — Jan Dupal <dupal.j@gmail.com>
#
#   This is free and unencumbered software released into the public domain.
#
#   Anyone is free to copy, modify, publish, use, compile, sell, or distribute
#   this software, either in source code form or as a compiled binary, for any
#   purpose, commercial or non-commercial, and by any means.
#
#   In jurisdictions that recognize copyright laws, the author or authors of
#   this software dedicate any and all copyright interest in the software to
#   the public domain. We make this dedication for the benefit of the public
#   at large and to the detriment of our heirs and successors. We intend this
#   dedication to be an overt act of relinquishment in perpetuity of all
#   present and future rights to this software under copyright law.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

cmake_minimum_required(VERSION 2.8.12)

# CMake policies. 0025 needs to be before project(), so putting all there.
# Use AppleClang instead of Clang on Apple
if(POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif()
# Don't treat imported targets with :: as files
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()
# Enable MACOSX_RPATH by default
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()
# Quoted variables should not be dereferenced
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()

project(MagnumDartExample)

if(CORRADE_TARGET_EMSCRIPTEN OR CORRADE_TARGET_ANDROID)
    set(DARTEXAMPLE_DIR ".")
else()
    set(DARTEXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# We need the URDF loader of DART for this example
find_package(DART 6.0.0 CONFIG REQUIRED OPTIONAL_COMPONENTS io-urdf utils-urdf)

if(NOT (DART_utils-urdf_FOUND OR DART_io-urdf_FOUND) OR (DART_VERSION STRLESS 6.0.0))
    message(FATAL_ERROR "DART version >= 6 and DART urdf library are required for this example!")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

find_package(Magnum REQUIRED
    MeshTools
    SceneGraph
    Primitives
    Shaders
    Sdl2Application)
find_package(MagnumIntegration REQUIRED
    Dart)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

add_executable(magnum-dart DartExample.cpp)
target_include_directories(magnum-dart PUBLIC ${DART_INCLUDE_DIRS} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(magnum-dart
    Magnum::Application
    Magnum::MeshTools
    Magnum::Magnum
    Magnum::SceneGraph
    Magnum::Primitives
    Magnum::Shaders
    MagnumIntegration::Dart
    ${DART_LIBRARIES})

install(TARGETS magnum-dart DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})
