cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE RelWithDebInfo)

rosbuild_init()

set(ROS_BUILD_TYPE Release)


#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


rosbuild_add_library(${PROJECT_NAME} src/shape_operations.cpp
				     src/bodies.cpp)


# Unit tests
rosbuild_add_gtest(test_point_inclusion test/test_point_inclusion.cpp)
target_link_libraries(test_point_inclusion ${PROJECT_NAME})