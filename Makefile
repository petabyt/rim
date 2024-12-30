all:
	tcc -run tree.c


CFLAGS := -Icimgui/imgui `pkg-config --cflags sdl2` -g
CXXFLAGS := $(CFLAGS) -g
DEMO_FILES := demo.o cimgui.o ./cimgui/imgui/backends/imgui_impl_sdl2.o
demo.out: $(DEMO_FILES)
	g++ $(DEMO_FILES) `pkg-config --libs sdl2` cimgui/cimgui.so cimgui/backend_test/example_sdl_opengl3/libcimgui_sdl.so -lGL -ldl -o demo.out
