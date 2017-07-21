

cmake_minimum_required(VERSION 3.4)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(maze)

find_package(Qt5Widgets REQUIRED)
find_package(QVR REQUIRED)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(maze)
set_target_properties(maze PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
)
