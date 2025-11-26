# Copyright(C) 2019-2025 Adrian Mansell
#
# This program is free software : you can redistribute it and /or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.If not, see < https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.16)

include(CMakePrintHelpers)

project(ACAD 
    VERSION 3.7.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(Qt6_DIR ${CMAKE_SOURCE_DIR}/qt5_debug/lib/cmake/Qt6)
else()
    set(Qt6_DIR ${CMAKE_SOURCE_DIR}/qt5_release/lib/cmake/Qt6)
endif()
cmake_print_variables(Qt6_DIR)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()


qt_add_executable(ACAD
    app/app.cpp
    app/build.cpp
    app/main.cpp
    hpgl/hpgl.cpp
    neutralpoint/neutralpoint.cpp
    tabs/tabs.cpp
    utils/ascii.cpp
    utils/bezier.cpp
    utils/debug.cpp
    utils/former.cpp
    utils/object_oo.cpp
    wing/airfoil.cpp
    wing/element.cpp
    wing/le_template.cpp
    wing/part.cpp
    wing/planform.cpp
    wing/rib.cpp
    wing/spar.cpp
    wing/wing.cpp
    application.qrc
)

qt_add_resources(helloworld imageresources
    PREFIX "/images"
    FILES add.png del.png export_dxf.png export hpgl.png mum.ico new.png open.png refresh.png save.png save_as.png
)

target_include_directories(ACAD PUBLIC
    app
    dxf
    hpgl
    neutralpoint
    tabs
    thirdparty
    thirdparty/json
    utils
    wing
)

target_include_directories(ACAD INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ACAD PRIVATE
    Qt6::Widgets
)

target_compile_options(ACAD PUBLIC 
    /Zc:preprocessor
)

add_compile_definitions(VERSION="${PROJECT_VERSION}")

set_target_properties(ACAD PROPERTIES
    WIN32_EXECUTABLE ON
    AUTOMOC TRUE
    AUTORCC TRUE
)

# Use windeployqt to get all of the libraries etc. into the binary directory for development work
add_custom_command(
  TARGET ACAD POST_BUILD
  COMMAND
    CD ${CMAKE_CURRENT_BINARY_DIR} &&
    ${WINDEPLOYQT_EXECUTABLE} --pdb ACAD.exe
       --verbose 1 --dir . --plugindir plugins --compiler-runtime
       COMMENT "Custom: windeployqt"
    )

# Copy supporting files to binary directory
add_custom_command(TARGET ACAD POST_BUILD
    COMMAND 
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/copyfiles ${CMAKE_CURRENT_BINARY_DIR}
)
