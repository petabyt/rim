// im_ generic API
#include "nim_internal.h"
#include "im.h"

int im_button(const char *label) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, NIM_BUTTON, 0);
	nim_add_prop_text(tree, NIM_PROP_TEXT, label);
	nim_end_widget(tree);
	return nim_last_widget_event();
}

int im_label(const char *label) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, NIM_LABEL, 0);
	nim_add_prop_text(tree, NIM_PROP_TEXT, label);
	nim_end_widget(tree);
	return nim_last_widget_event();
}

int im_window(const char *name, int width, int height) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, NIM_WINDOW, -1);
	nim_add_prop_text(tree, NIM_PROP_WIN_TITLE, name);
	return 1;
}

static int im_end(void) {
	struct NimTree *tree = nim_get_current_tree();
	nim_end_widget(tree);
	return 0;
}
void im_end_window(void) { im_end(); }
