cmake_minimum_required(VERSION 3.10)

project(rim)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libui-dev ${CMAKE_CURRENT_BINARY_DIR}/libui)

set(CORE src/tree.c src/im.c)

add_library(rim_libui_gtk3 SHARED
	${CORE}
	src/vdom.c
	src/libui.c
)
target_link_libraries(rim_libui_gtk3 LibUI::LibUI)
target_include_directories(rim_libui_gtk3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(rim_demo test/demo.c)
target_link_libraries(rim_demo rim_libui_gtk3)

add_executable(test_tree test/test_tree.c)
target_link_libraries(test_tree rim_libui_gtk3)

add_executable(test_differ test/test_differ.c)
target_link_libraries(test_differ rim_libui_gtk3)
