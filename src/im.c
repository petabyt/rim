// im_ generic API
#include "rim_internal.h"
#include "im.h"

int im_button(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_BUTTON, 0);
	rim_add_prop_text(tree, RIM_PROP_TEXT, label);
	rim_end_widget(tree);
	return rim_last_widget_event();
}

int im_label(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_LABEL, 0);
	rim_add_prop_text(tree, RIM_PROP_TEXT, label);
	rim_end_widget(tree);
	return rim_last_widget_event();
}

int im_window(const char *name, int width, int height) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_WINDOW, -1);
	rim_add_prop_text(tree, RIM_PROP_WIN_TITLE, name);
	return 1;
}

void im_entry(const char *label, char *buffer, unsigned int size) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_LABEL, 0);
	rim_add_prop_text(tree, RIM_PROP_TEXT, label);
	rim_end_widget(tree);
	if (rim_last_widget_event()) {
		// TODO:
		// strncpy(buffer, event->buffer, size);
	}
}

static int im_end(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_end_widget(tree);
	return 0;
}
void im_end_window(void) { im_end(); }
