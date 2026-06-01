cmake_minimum_required(VERSION 2.8.12)
project(DepthCloud)
add_executable(DepthCloud DepthCloud.cpp)
target_link_libraries(DepthCloud PRIVATE PointCloud)
