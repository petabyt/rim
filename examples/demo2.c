#include <stdio.h>
#include <rim.h>

int mymain(struct RimContext *ctx, void *arg) {
	int show_more = 0;
	int selected = 0;
	int n_options = 4;
	while (rim_poll(ctx)) {
		im_set_next_inner_padding(1);
		if (im_begin_window("My Window", 500, 500)) {
			if (im_button("Show more")) {
				show_more = !show_more;
			}
			if (show_more) {
				im_label("Inserted a node");
			}
			im_spinbox(0, 10, &n_options);
			if (im_begin_combo_box("Combo", &selected)) {
				for (int i = 0; i < n_options; i++) {
					im_combo_box_item("Item 1");
				}
				im_end_combo_box();
			}

			if (im_begin_radio(&selected)) {
				for (int i = 0; i < n_options; i++) {
					im_radio_item("Item 1");
				}
				im_end_radio();
			}

			im_spinbox(0, 10, &selected);

			im_end_window();
		}
	}

	return 0;
}
int main(void) {
    return rim_start(mymain, NULL);
}
