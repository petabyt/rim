#include "imgui.h"

int main(void) {
	void *ctx = imgui_init();

	char buf[64];

	// show a simple window that we created ourselves.
	while (imgui_while_open(ctx)) {
		nim_text("Hello, World %d", 123);
		if (nim_button("Save"))
			printf("Hello\n");
		nim_input_text("string", buf, sizeof(buf));
		
		imgui_end(ctx);
	}

	return 0;
}
