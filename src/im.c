// im_ generic API
#include <stdio.h>
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
	rim_add_prop_u32(tree, RIM_PROP_WIN_WIDTH, (uint32_t)width);
	rim_add_prop_u32(tree, RIM_PROP_WIN_HEIGHT, (uint32_t)height);
	return 1;
}

int im_tab_bar(int *selected) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_TAB_BAR, -1);
	return IM_CHILDREN_VISIBLE;
}

int im_tab(const char *title) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_TAB, -1);
	rim_add_prop_text(tree, RIM_PROP_WIN_TITLE, title);
	return IM_CHILDREN_VISIBLE;
}

void im_entry(const char *label, char *buffer, unsigned int size) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_ENTRY, 0);
	rim_add_prop_text(tree, RIM_PROP_TEXT, buffer);
	rim_add_prop_text(tree, RIM_PROP_LABEL, label);
	rim_end_widget(tree);
	if (rim_last_widget_event() == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		snprintf(buffer, size, "%s", (char *)ctx->last_event.data);
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
void im_end_tab(void) { im_end(); }
void im_end_tab_bar(void) { im_end(); }
