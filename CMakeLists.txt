cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(rim)

option(RIM_COMPILE_DEMOS "Compile demos" ON)
option(RIM_COMPILE_LIBUI "" ON)
option(RIM_COMPILE_LIBUI_SCINTILLA "" ON)

set(CMAKE_C_FLAGS_DEBUG "-g")

set(RIM_CORE src/tree.c src/im.c src/main.c src/vdom.c)

add_library(rim_base STATIC ${RIM_CORE})
target_include_directories(rim_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(RIM_COMPILE_LIBUI)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libui-dev ${CMAKE_CURRENT_BINARY_DIR}/libui)
	if(RIM_COMPILE_LIBUI_SCINTILLA)
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../libui-scintilla ${CMAKE_CURRENT_BINARY_DIR}/scintilla)
	endif()

	if(UNIX)
		add_library(rim_libui_gtk3 STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_gtk3 LibUI::LibUI)
		target_include_directories(rim_libui_gtk3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
		add_library(Rim:RimLibUIGTK3 ALIAS rim_libui_gtk3)
		add_library(Rim:Rim ALIAS rim_libui_gtk3)

		if(RIM_COMPILE_LIBUI_SCINTILLA)
			add_library(rim_libui_scintilla_gtk3 STATIC
				src/libui_scintilla.c
			)
			target_link_libraries(rim_libui_scintilla_gtk3 libui_scintilla rim_libui_gtk3)
			add_library(Rim:Scintilla ALIAS rim_libui_scintilla_gtk3)
		endif()
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
endif()

if(RIM_COMPILE_DEMOS)
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
endif()
