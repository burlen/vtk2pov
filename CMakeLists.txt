cmake_minimum_required(VERSION 3.0)
project(vtk2pov)

find_package(VTK)
include(${VTK_USE_FILE})
if (NOT VTK_FOUND)
message(FETAL_ERROR
"
Error:" ;
"VTK was not found. Set VTK_DIR in your cache."
)
endif()

#find_package(Boost COMPONENTS program_options)
#if (NOT Boost_FOUND)
#message(FATAL_ERROR
#"
#Error:" ;
#"Boost was not found. Set BOOST_ROOT in your cache."
#)
#endif()

include_directories(${Boost_INCLUDE_DIRS} ${VTK_INCLUDE_DIRS})
add_executable(vtk2pov vtk2pov.cpp)
target_link_libraries(vtk2pov ${VTK_LIBRARIES} ${Boost_LIBRARIES})
