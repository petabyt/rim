#include <stdio.h>
#include <rim.h>

int mymain(struct RimContext *ctx, void *arg) {
	int show_more = 0;
	int counter = 0;
	int value = 50;
	int selected = 0;
	while (rim_poll(ctx)) {
		im_set_next_inner_padding(1);
		if (im_begin_window("My Window", 500, 500)) {
			if (im_button("Show more")) {
				show_more = !show_more;
			}
			if (show_more) {
				im_label("Select an item");
			}
			im_spinbox(0, 10, &selected);
			if (im_begin_combo_box("Combo", &selected)) {
				if (show_more) {
					im_combo_box_item("hello");
				} else {
					im_combo_box_item("Item 1");
				}
				im_combo_box_item("Item 2");
				im_combo_box_item("Item 3");
				im_end_combo_box();
			}

			if (im_begin_radio(&selected)) {
				im_radio_item("Hello 1");
				im_radio_item("Hello 2");
				im_radio_item("Hello 3");
				im_end_radio();
			}

			counter++;
			im_end_window();
		}
	}

	return 0;
}
int main(void) {
    return rim_start(mymain, NULL);
}
