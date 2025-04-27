#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 1;
	int counter = 0;
	const char *title1 = "Big window";
	const char *title2 = "Small window";
	int open1 = 1;
	int open2 = 1;
	while (rim_poll(ctx)) {
		if (im_begin_window_ex(title1, 600, 600, &open1)) {
			if (im_button("Change other window text")) title2 = "123";
			if (im_button("Show other window")) open2 = !open2;
			im_end_window();
		}
		if (im_begin_window_ex(title2, 400, 300, &open2)) {
			if (im_button("Change window text")) title1 = "cool";
			im_end_window();
		}
	}

	return 0;
}
