#include <stdio.h>
#include <rim.h>
#include <im.h>

int main() {
	struct RimContext *ctx = rim_init();
	rim_libui_init(ctx);
	int show_more = 0;
	int counter = 0;
	while (rim_poll(ctx)) {
		if (im_window("My Window", 640, 480)) {
			char buffer[64];
			sprintf(buffer, "Events: %d\n", counter);
			im_label(buffer);

			if (im_button("Show More")) {
				show_more = 1;
			}
			if (counter & 1) {
				im_label("Hello, World");
			}
			counter++;
		}
	}

	return 0;
}
