#include "nim.h"
#include "im.h"

int main() {
	struct NimContext *ctx = nim_init();
	int show_more = 0;

	while (nim_poll(ctx)) {
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
