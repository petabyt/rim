#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int open = 1;
	while (rim_poll(ctx)) {
		if (im_begin_window_ex("Hello", 400, 300, &open)) {
			if (im_begin_menu_bar()) {
				if (im_begin_menu("File")) {
					if (im_menu_item("Open")) {
						puts("Open");
					}
					if (im_menu_item("Save")) {
						puts("Save");
					}
					im_end_menu();
				}
				if (im_begin_menu("Settings")) {
					if (im_menu_item("Do this")) {
						puts("Wow");
					}
					if (im_menu_item("Do it")) {
						puts("Cool");
					}
					im_end_menu();
				}
				im_end_menu_bar();
			}
			im_button("There's menus above this");
			im_end_window();
		}
	}

	return 0;
}
