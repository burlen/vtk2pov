cmake_minimum_required(VERSION 2.8)
project(vtk2pov)

add_executable(vtk2pov vtk2pov.cpp)

find_package(VTK)
include(${VTK_USE_FILE})
if (NOT VTK_FOUND)
message(FETAL_ERROR
"
Error:" ;
"VTK was not found. Set VTK_DIR in your cache."
)
endif()
include_directories(${VTK_INCLUDE_DIRS})
target_link_libraries(vtk2pov ${VTK_LIBRARIES})

set(ENABLE_BOOST ON CACHE BOOL "Enable boost support")
if (ENABLE_BOOST)
find_package(Boost 1.50.0 COMPONENTS program_options)
if (NOT Boost_FOUND)
message(FATAL_ERROR
"
Error:" ;
"Boost was not found. Set BOOST_ROOT in your cache."
)
endif()
add_definitions(-DENABLE_BOOST)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(vtk2pov ${Boost_LIBRARIES})
endif()

