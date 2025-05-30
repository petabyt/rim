cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
project(rim)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_C_FLAGS_RELEASE "-O2")
else()
	set(CMAKE_C_FLAGS_DEBUG "-g")
	set(CMAKE_CPP_FLAGS_DEBUG "-g")
endif()

option(RIM_COMPILE_DEMOS "Compile demos" ON)
option(RIM_COMPILE_LIBUI "Compile LibUI backend" ON)
option(RIM_COMPILE_LIBUI_SCINTILLA "Compile LibUI Scintilla extension" OFF)
option(RIM_PACKAGE_EDITOR "" OFF)

set(RIM_LIBUI_DEV_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../libui-dev CACHE STRING "libui dir")
set(RIM_LIBUI_SCINTILLA_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../libui-scintilla CACHE STRING "libui-scintilla dir")
set(RIM_LUA53_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../lua53 CACHE STRING "lua dir")

set(RIM_CORE src/tree.c src/im.c src/main.c src/vdom.c src/enum.c)

add_library(rim_base STATIC ${RIM_CORE})
target_include_directories(rim_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/extensions)

set(RIM_CFLAGS -Wall -Wshadow -Wcast-qual -Wpedantic -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=return-type -Wno-unused-variable -Wno-unused-function)

if(RIM_COMPILE_LIBUI)
	if(NOT EXISTS ${RIM_LIBUI_DEV_LOCATION})
		message(SEND_ERROR "libui missing from ${RIM_LIBUI_DEV_LOCATION}")
	endif()
	option(LIBUI_COMPILE_EXAMPLES "" OFF)
	option(LIBUI_INCLUDE_INSTALL "" OFF)
	add_subdirectory(${RIM_LIBUI_DEV_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/libui)
	if(APPLE)
		add_library(rim_libui_osx STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_osx LibUI::LibUI)
		set(RIM_LIB_NAME rim_libui_osx)
	elseif(UNIX)
		add_library(rim_libui_gtk3 STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_gtk3 LibUI::LibUI)
		set(RIM_LIB_NAME rim_libui_gtk3)
	elseif(WIN32)
		add_library(rim_libui_win STATIC
			${RIM_CORE}
			src/libui.c
		)
		target_link_libraries(rim_libui_win LibUI::LibUI)
		set(RIM_LIB_NAME rim_libui_win)
	endif()

	target_include_directories(${RIM_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/extensions)
	target_compile_options(${RIM_LIB_NAME} PRIVATE ${RIM_CFLAGS})
	add_library(Rim::RimLibUI ALIAS ${RIM_LIB_NAME})
	add_library(Rim::Rim ALIAS ${RIM_LIB_NAME})

	if(RIM_COMPILE_LIBUI_SCINTILLA)
		if(NOT EXISTS ${RIM_LIBUI_SCINTILLA_LOCATION})
			message(SEND_ERROR "libui-scintilla missing from ${RIM_LIBUI_SCINTILLA_LOCATION}")
		endif()
		add_subdirectory(${RIM_LIBUI_SCINTILLA_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/scintilla)
		add_library(rim_libui_scintilla STATIC
			extensions/libui_scintilla.c
		)
		target_link_libraries(rim_libui_scintilla libui_scintilla Rim::RimLibUI)
		add_library(Rim::ScintillaLibUI ALIAS rim_libui_scintilla)
	endif()
endif()

if(WIN32)
	set(WIN_LINK_LIBS -s -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lssp -lurlmon -luuid -lws2_32 -static-libgcc -static-libstdc++ -lpthread -limm32)
endif()

if(RIM_COMPILE_DEMOS)
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_C_FLAGS_RELEASE "-O2")
	else()
		set(CMAKE_C_FLAGS_DEBUG "-g")
		set(CMAKE_CPP_FLAGS_DEBUG "-g")
	endif()

	function(add_demo name src)
		if(WIN32)
			add_executable(${name} ${src} etc/win.rc)
			target_link_libraries(${name} Rim::Rim ${WIN_LINK_LIBS})
		else()
			add_executable(${name} ${src})
			target_link_libraries(${name} Rim::Rim)
		endif()
	endfunction()

	add_demo(demo examples/demo.c)
	add_demo(demo2 examples/demo2.c)
	add_demo(ui examples/ui.c)
	add_demo(thread examples/thread.c)
	add_demo(windows examples/windows.c)
	add_demo(log examples/log.c)
	add_demo(remap examples/remap.c)
	add_demo(minimal examples/minimal.c)
	add_demo(menus examples/menus.c)
	#add_demo(fudge examples/fudge.c)

	add_executable(test test/test.c)
	target_link_libraries(test rim_base ${WIN_LINK_LIBS})
endif()

if(RIM_COMPILE_LUA_EDITOR AND RIM_COMPILE_LIBUI)
	function(create_appimage)
	    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "TARGET;ICON;DESKTOP" "")

	    install(TARGETS ${ARG_TARGET} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/AppDir/usr/bin)

	    find_program(LINUXDEPLOY linuxdeploy)
	    if(NOT LINUXDEPLOY)
	        message(FATAL_ERROR "linuxdeploy not found")
	    endif()
	    find_program(APPIMAGETOOL appimagetool)
	    if(NOT APPIMAGETOOL)
	        message(FATAL_ERROR "appimagetool not found")
	    endif()

	    # Strip binary included in linuxdeploy is broken, disable stripping
	    add_custom_target(${ARG_TARGET}_AppImage ALL
	        COMMAND rm -rf AppDir
	        COMMAND NO_STRIP=true linuxdeploy --appdir=${CMAKE_CURRENT_BINARY_DIR}/AppDir --executable=$<TARGET_FILE:${ARG_TARGET}> -d ${ARG_DESKTOP} -i ${ARG_ICON} --output appimage
	        COMMAND appimagetool ${CMAKE_CURRENT_BINARY_DIR}/AppDir
	        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	        DEPENDS ${ARG_TARGET}
	    )
	endfunction()

	add_subdirectory(${RIM_LUA53_LOCATION} ${CMAKE_CURRENT_BINARY_DIR}/lua)

	add_executable(rim_editor examples/demo/main.c examples/demo/lua.c etc/win.rc)
	target_link_libraries(rim_editor Rim::ScintillaLibUI Rim::RimLibUI lua-5.3 ${WIN_LINK_LIBS})

	if(LINUX AND RIM_PACKAGE_EDITOR)
		create_appimage(
			TARGET rim_editor
			ICON ${CMAKE_CURRENT_SOURCE_DIR}/etc/logo.png
			DESKTOP ${CMAKE_CURRENT_SOURCE_DIR}/etc/rim.desktop
		)
	endif()
endif()
