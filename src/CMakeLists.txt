

find_package(Qt5Widgets REQUIRED)
find_package(QVR REQUIRED)

qt5_add_resources(RESOURCES resources.qrc)

add_executable(maze
  maze.cpp maze.h
  mesh.cpp mesh.h
  entity.cpp entity.h
  cell.cpp cell.h
  builder.cpp builder.h
  ${RESOURCES}
)
set_target_properties(maze PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
)

include_directories(${QVR_INCLUDE_DIRS})
target_link_libraries(maze ${QVR_LIBRARIES} Qt5::Widgets)
install(TARGETS maze
  RUNTIME DESTINATION bin
)
