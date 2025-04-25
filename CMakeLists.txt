cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(rim)

set(CMAKE_C_FLAGS_DEBUG "-g")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libui-dev ${CMAKE_CURRENT_BINARY_DIR}/libui)

set(RIM_CORE src/tree.c src/im.c src/main.c src/vdom.c)

add_library(rim_base STATIC ${RIM_CORE})
target_include_directories(rim_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(UNIX)
	add_library(rim_libui_gtk3 STATIC
		${RIM_CORE}
		src/libui.c
	)
	target_link_libraries(rim_libui_gtk3 LibUI::LibUI)
	target_include_directories(rim_libui_gtk3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
	add_library(Rim:RimLibUIGTK3 ALIAS rim_libui_gtk3)
	add_library(Rim:Rim ALIAS rim_libui_gtk3)
endif()
if(WIN32)
	add_library(rim_libui_win STATIC
		${RIM_CORE}
		src/libui.c
	)
	target_link_libraries(rim_libui_win LibUI::LibUI)
	target_include_directories(rim_libui_win PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
	add_library(Rim:RimLibUIWin ALIAS rim_libui_win)
	add_library(Rim:Rim ALIAS rim_libui_win)
endif()

add_executable(demo test/demo.c)
target_link_libraries(demo Rim:Rim)

add_executable(ui test/ui.c)
target_link_libraries(ui Rim:Rim)

add_executable(test_tree test/test_tree.c)
target_link_libraries(test_tree Rim:Rim)

add_executable(test_differ test/test_differ.c)
target_link_libraries(test_differ rim_base)

add_executable(thread test/thread.c)
target_link_libraries(thread Rim:Rim)

add_executable(windows test/windows.c)
target_link_libraries(windows Rim:Rim)
