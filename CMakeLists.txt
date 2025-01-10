cmake_minimum_required(VERSION 3.0)

project(hello_world)

find_package(libcamera)

add_executable(hello_world src/hello_world.cpp)

add_executable(cam_main src/cam_main.cpp)
target_link_libraries(cam_main libcamera)