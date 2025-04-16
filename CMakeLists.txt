cmake_minimum_required(VERSION 3.10)

project(nim)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libui-dev ${CMAKE_CURRENT_BINARY_DIR}/libui)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../imgui ${CMAKE_CURRENT_BINARY_DIR}/imgui)

set(CORE tree.c im.c)

add_library(nim_libui_gtk3 SHARED
	${CORE}
	vdom.c
	libui.c
)
target_link_libraries(nim_libui_gtk3 LibUI::LibUI)
target_include_directories(nim_libui_gtk3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../imgui/)


add_executable(nim_demo demo.c)
target_link_libraries(nim_demo nim_libui_gtk3)
add_executable(test_tree test_tree.c)
target_link_libraries(test_tree nim_libui_gtk3)