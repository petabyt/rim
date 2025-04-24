#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 1;
	int counter = 0;
	while (rim_poll(ctx)) {
		if (im_window("My Window", 500, 500)) {
			char buffer[64];
			sprintf(buffer, "Events: %04d\n", counter);
			im_label(buffer);
			if (im_button("Show More")) {
				show_more = !show_more;
			}
			if (show_more) {
				im_label("Hello, World");
			}

			im_label("Two labels with the same text!");
			im_label("Two labels with the same text!");

			counter++;
		}
	}

	return 0;
}
