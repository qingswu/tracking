cmake_minimum_required(VERSION 2.8)
project( tracker )
SET (CMAKE_CXX_COMPILER             "/usr/bin/g++")
SET (CMAKE_CXX_FLAGS                "-Wall -std=c++11")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g -traceback")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
find_package( OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
#include_directories( "/usr/include/eigen3/" )
include_directories( "/usr/local/include/eigen3/" )

add_executable( pmmh run_pmmh_old.cpp pmmh.cpp particle_filter.cpp hist.cpp dirichlet.cpp utils.cpp gaussian.cpp multinomial.cpp poisson.cpp)
target_link_libraries( pmmh ${OpenCV_LIBS} )
add_executable( tracker main_old.cpp particle_filter.cpp hist.cpp dirichlet.cpp utils.cpp gaussian.cpp multinomial.cpp poisson.cpp)
target_link_libraries( tracker ${OpenCV_LIBS} )
