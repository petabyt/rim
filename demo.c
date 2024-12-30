#undef NDEBUG
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "imgui.h"

int main(void) {
	void *ctx = imgui_init();

	bool showDemoWindow = true;
	int counter = 0;

	// show a simple window that we created ourselves.
	while (imgui_while_open(ctx)) {
		igBegin("Hello, world!", NULL, 0);
		igText("This is some useful text");
		igCheckbox("Demo window", &showDemoWindow);

		ImVec2 buttonSize;
		buttonSize.x = 0;
		buttonSize.y = 0;
		if (igButton("Button", buttonSize))
			counter++;
		igSameLine(0.0f, -1.0f);
		igText("counter = %d", counter);
		igEnd();
		imgui_end(ctx);
	}

	return 0;
}
