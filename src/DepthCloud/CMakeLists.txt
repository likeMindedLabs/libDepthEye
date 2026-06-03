cmake_minimum_required(VERSION 3.21)
project(DepthCloud)
add_executable(DepthCloud DepthCloud.cpp)
target_link_libraries(DepthCloud PRIVATE PointCloud)
