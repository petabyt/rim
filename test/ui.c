#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	rim_libui_init(ctx);
	int show_more = 1;
	int counter = 0;
	int selected = 0;
	while (rim_poll(ctx)) {
		if (im_window("My Window", 500, 500)) {
			if (im_tab_bar(&selected)) {
				if (im_tab("Tab thing")) {
					if (im_button("Hello")) {
						printf("Thing\n");
					}
					im_end_tab();
				}
				if (im_tab("Tab #2")) {
					if (im_button("Hello 2")) {
						printf("Thing 2\n");
					}
					im_end_tab();
				}
				im_end_tab_bar();
			}
		}
	}

	return 0;
}
