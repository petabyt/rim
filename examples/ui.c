#include <stdio.h>
#define REPLACE_MAIN
#include <rim.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 1;
	int counter = 0;
	int selected = 0;
	char buffer[64] = "asd";
	while (rim_poll(ctx)) {
		if (im_begin_window("My Window", 500, 500)) {
			if (im_begin_tab_bar(&selected)) {
				if (im_begin_tab("Tab thing")) {
					im_entry("lbl", buffer, sizeof(buffer));
					im_label(buffer);
					if (im_button("Hello")) {
						snprintf(buffer, sizeof(buffer), "changed %04d", counter++);
					}
					im_end_tab();
				}
				if (im_begin_tab("Tab #2")) {
					if (im_button("Hello 2")) {
						printf("Thing 2\n");
					}
					im_end_tab();
				}
				im_end_tab_bar();
			}
			im_end_window();
		}
	}

	return 0;
}
