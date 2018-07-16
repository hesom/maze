# Copyright (C) 2016
# Computer Graphics Group, University of Siegen
# Written by Martin Lambers <martin.lambers@uni-siegen.de>
#
# Copying and distribution of this file, with or without modification, are
# permitted in any medium without royalty provided the copyright notice and this
# notice are preserved. This file is offered as-is, without any warranty.

cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(maze)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Wextra")
endif()

find_package(Qt5Widgets REQUIRED)
find_package(QVR REQUIRED)

include_directories(${QVR_INCLUDE_DIRS})
link_directories(${QVR_LIBRARY_DIRS})
qt5_add_resources(RESOURCES src/maze.qrc)
add_executable(maze
    src/MazeApp.cpp src/MazeApp.hpp
    src/stb_image.h src/tiny_obj_loader.h
    ${RESOURCES})
set_target_properties(maze PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(maze ${QVR_LIBRARIES} Qt5::Widgets)

configure_file(src/maze.bmp ${CMAKE_BINARY_DIR}/maze.bmp COPYONLY)
configure_file(src/goldCoin.wavefront ${CMAKE_BINARY_DIR}/goldCoin.wavefront COPYONLY)
configure_file(src/config.qvr ${CMAKE_BINARY_DIR}/config.qvr)

install(TARGETS maze RUNTIME DESTINATION bin)
