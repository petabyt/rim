cmake_minimum_required(VERSION 3.10)
project(rim_demo)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/rim)

add_executable(rim_demo main.c)
target_link_libraries(rim_demo Rim:Rim)
