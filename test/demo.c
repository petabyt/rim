#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 0;
	int counter = 0;
	int value = 50;
	while (rim_poll(ctx)) {
		if (im_begin_window("My Window", 500, 500)) {
			char buffer[64];
			sprintf(buffer, "Events: %04d", counter);
			im_label(buffer);

			im_slider(0, 100, &value);
			sprintf(buffer, "Slider value: %d", value);
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
			im_end_window();
		}
	}

	return 0;
}
