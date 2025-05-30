#include <stdio.h>
#include <rim.h>

int mymain(struct RimContext *ctx, void *arg) {
	int show_more = 0;
	int counter = 0;
	int value = 50;
	int selected = 0;
	while (rim_poll(ctx)) {
		im_set_next_inner_padding(1);
		im_set_next_gap(1);
		if (im_begin_window("My Window", 500, 500)) {
			char buffer[64];
			sprintf(buffer, "Events: %04d", counter);
			im_label(buffer);

			im_slider(0, 100, &value);
			sprintf(buffer, "Slider value: %d", value);
			im_label(buffer);

			im_set_next_tooltip("Tooltip");
			if (im_button("Show More")) show_more = !show_more;
			if (show_more) im_label("Hello, World");

			im_label("Two labels with the same text!");
			im_label("Two labels with the same text!");

			sprintf(buffer, "Currently selected item %d", selected);
			im_label(buffer);
			if (im_begin_combo_box("Label", &selected)) {
				im_combo_box_item("Item 1");
				im_combo_box_item("Item 2");
				im_combo_box_item("Item 3");
				im_end_combo_box();
			}

			im_spinbox(0, 100, &selected);

			counter++;
			im_end_window();
		}
	}

	return 0;
}
int main(void) {
    return rim_start(mymain, NULL);
}
