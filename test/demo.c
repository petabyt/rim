#include <stdio.h>
#include <nim.h>
#include <im.h>

int main() {
	struct NimContext *ctx = nim_init();
	nim_libui_init(ctx);
	int show_more = 0;
	int counter = 0;
	while (nim_poll(ctx)) {
		if (im_window("My Window", 640, 480)) {
			char buffer[64];
			sprintf(buffer, "Events: %d\n", counter);
			im_label(buffer);

			if (im_button("Show More")) {
				show_more = 1;
			}
			if (show_more) {
				im_label("Hello, World");
			}
			counter++;
		}
	}

	return 0;	
}
