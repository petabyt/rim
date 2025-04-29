#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int x = 0;
	while (rim_poll(ctx)) {
		if (im_begin_window("My Window", 500, 500)) {
			if (im_button("asd")) {
				x++;
			}
			if (im_begin_static_layout()) {
				if (x & 1) {
					im_label("Hello");
				}
				im_end_static_layout();
			}
			if (x & 3) {
				// TODO: Need to remap onclick events
				if (im_button("dsa")) {
					printf("Hello\n");
				}
			}
			im_end_window();
		}
	}

	return 0;
}
