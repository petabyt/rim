cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(rim_demo)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_C_FLAGS_DEBUG "-O2")
else()
	set(CMAKE_C_FLAGS_DEBUG "-g")
endif()

option(RIM_COMPILE_DEMOS "" OFF)
option(RIM_COMPILE_LIBUI_SCINTILLA "" ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/rim)

include(../libs/lua.cmake)

add_executable(rim_demo main.c lua.c)
target_link_libraries(rim_demo Rim:Scintilla Rim:Rim lua)
