#include <stdio.h>
#define REPLACE_MAIN
#include <rim.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int x = 0;
	while (rim_poll(ctx)) {
		if (im_begin_window("My Window", 500, 500)) {
			if (im_button("asd")) {
				x++;
			}
			if (im_begin_vbox()) {
				if (x & 1) {
					im_label("Hello");
				}
				im_end_vbox();
			}
			if (x & 3) {
				// Test remapping button events
				if (im_button("dsa")) {
					printf("Hello\n");
				}
			}
			im_end_window();
		}
	}

	return 0;
}
