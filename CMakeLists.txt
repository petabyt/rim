cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(rim)

option(RIM_COMPILE_DEMOS "Compile demos" ON)
option(RIM_COMPILE_LIBUI "Compile LibUI backend" ON)
option(RIM_COMPILE_LIBUI_SCINTILLA "Compile LibUI Scintilla extension" OFF)

#set(CMAKE_C_FLAGS_DEBUG "-g")

set(RIM_CORE src/tree.c src/im.c src/main.c src/vdom.c)

add_library(rim_base STATIC ${RIM_CORE})
target_include_directories(rim_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(RIM_COMPILE_LIBUI)
	option(LIBUI_COMPILE_EXAMPLES "" OFF)
	option(LIBUI_INCLUDE_INSTALL "" OFF)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/libui-dev ${CMAKE_CURRENT_BINARY_DIR}/libui)
	if(APPLE)
		add_library(rim_libui_osx STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_osx LibUI::LibUI)
		target_include_directories(rim_libui_osx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
		add_library(Rim:RimLibUIOSX ALIAS rim_libui_gtk3)
		add_library(Rim:Rim ALIAS rim_libui_osx)
	elseif(UNIX)
		add_library(rim_libui_gtk3 STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_gtk3 LibUI::LibUI)
		target_include_directories(rim_libui_gtk3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
		add_library(Rim:RimLibUIGTK3 ALIAS rim_libui_gtk3)
		add_library(Rim:Rim ALIAS rim_libui_gtk3)
	elseif(WIN32)
		add_library(rim_libui_win STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_win LibUI::LibUI)
		target_include_directories(rim_libui_win PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
		add_library(Rim:RimLibUIWin ALIAS rim_libui_win)
		add_library(Rim:Rim ALIAS rim_libui_win)
	endif()

	if(RIM_COMPILE_LIBUI_SCINTILLA)
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/libui-scintilla ${CMAKE_CURRENT_BINARY_DIR}/scintilla)
		add_library(rim_libui_scintilla STATIC
			src/libui_scintilla.c
		)
		target_link_libraries(rim_libui_scintilla libui_scintilla Rim:Rim)
		add_library(Rim:Scintilla ALIAS rim_libui_scintilla)
	endif()
endif()

if(WIN32)
	set(LIBS -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lssp -lurlmon -luuid -lws2_32 -static-libgcc -static-libstdc++ -lpthread)
endif()

if(RIM_COMPILE_DEMOS)
	add_executable(demo test/demo.c)
	target_link_libraries(demo Rim:Rim ${LIBS})

	add_executable(ui test/ui.c)
	target_link_libraries(ui Rim:Rim ${LIBS})

	add_executable(test_tree test/test_tree.c)
	target_link_libraries(test_tree Rim:Rim ${LIBS})

	add_executable(test_differ test/test_differ.c)
	target_link_libraries(test_differ rim_base ${LIBS})

	add_executable(thread test/thread.c)
	target_link_libraries(thread Rim:Rim ${LIBS})

	add_executable(windows test/windows.c)
	target_link_libraries(windows Rim:Rim ${LIBS})

	add_executable(log test/log.c)
	target_link_libraries(log Rim:Rim ${LIBS})

	add_executable(remap test/remap.c)
	target_link_libraries(remap Rim:Rim ${LIBS})

	add_executable(minimal test/minimal.c etc/win.rc)
	target_link_libraries(minimal Rim:Rim ${LIBS})

endif()
