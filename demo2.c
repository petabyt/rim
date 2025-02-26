#include <stdio.h>
#include "nim.h"
#include "im.h"

int main() {
	int show_more = 0;

	struct NimContext *ctx = nim_imgui_init();

	while (nim_poll(ctx)) {
		puts("Hello");
		if (im_window("My Window", 640, 480, 0)) {
			if (im_button("Show More")) {
				show_more = 1;
			}
			if (show_more) {
				im_label("Hello, World");
			}
		}
	}

	return 0;	
}
