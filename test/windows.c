#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 1;
	int counter = 0;
	while (rim_poll(ctx)) {
		if (im_window("Big Window", 600, 600)) {
			im_label("Hello 123");
			im_end_window();
		}
		if (im_window("Small window", 400, 300)) {
			im_button("Hello 123");
			im_end_window();
		}
	}

	return 0;
}
