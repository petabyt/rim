#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	rim_libui_init(ctx);
	int show_more = 1;
	int counter = 0;
	while (rim_poll(ctx)) {
		if (im_window("My Window", 200, 200)) {
			char buffer[64];
			sprintf(buffer, "Events: %04d\n", counter);
			im_label(buffer);
			if (show_more) {
				im_label("Hello, World");
			}
			if (im_button("Show More")) {
				show_more = !show_more;
			}
			counter++;
		}
	}

	return 0;
}
