// Immediate-mode 'im_' API implementation for Rim
#include <stdio.h>
#include <string.h>
#include "rim_internal.h"
#include "im.h"

struct Props {
	int expand;
	int disabled;
	int begin_disabled;
}props = {0};

void im_set_next_expand(void) {
	props.expand = 1;
}
void im_set_next_disabled(int opt) {
	props.disabled = 1;
}
void im_begin_disabled(void) {
	props.begin_disabled = 1;
}
void im_end_disabled(void) {
	props.begin_disabled = 0;
}
void im_apply_prop(struct RimTree *tree) {
	if (props.expand) {
		rim_add_prop_u32(tree, RIM_PROP_EXPAND, 100);
		props.expand = 0;
	}
	if (props.disabled || props.begin_disabled) {
		rim_add_prop_u32(tree, RIM_PROP_DISABLED, 1);
		props.disabled = 0;
	}
}

static int im_end(int expected_type) {
	struct RimTree *tree = rim_get_current_tree();
	rim_end_widget(tree);
	return 0;
}

int im_button(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_BUTTON, 0);
	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	im_apply_prop(tree);
	rim_end_widget(tree);
	return rim_last_widget_event(0);
}

int im_label(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_LABEL, 0);
	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	im_apply_prop(tree);
	rim_end_widget(tree);
	return 0;
}

int im_begin_vertical_box(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_VERTICAL_BOX, -1);
	return 1;
}

void im_end_vertical_box(void) {
	im_end(RIM_VERTICAL_BOX);
}

int im_begin_horizontal_box(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_HORIZONTAL_BOX, -1);
	return 1;
}

void im_end_horizontal_box(void) {
	im_end(RIM_HORIZONTAL_BOX);
}

int im_begin_window(const char *name, int width_dp, int height_dp) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_WINDOW, -1);
	rim_add_prop_string(tree, RIM_PROP_WIN_TITLE, name);
	rim_add_prop_u32(tree, RIM_PROP_WIDTH_DP, (uint32_t)width_dp);
	rim_add_prop_u32(tree, RIM_PROP_HEIGHT_DP, (uint32_t)height_dp);
	im_apply_prop(tree);

	if (rim_last_widget_event(1) == RIM_EVENT_WINDOW_CLOSE) {
		struct RimContext *ctx = rim_get_global_ctx();
		ctx->quit_immediately = 1;
		rim_end_widget(tree);
		return 0;
	}

	return 1;
}

int im_begin_window_ex(const char *name, int width_dp, int height_dp, int *is_open) {
	struct RimTree *tree = rim_get_current_tree();
	if (*is_open) {
		rim_add_widget(tree, RIM_WINDOW, -1);
		rim_add_prop_string(tree, RIM_PROP_WIN_TITLE, name);
		rim_add_prop_u32(tree, RIM_PROP_WIDTH_DP, (uint32_t)width_dp);
		rim_add_prop_u32(tree, RIM_PROP_HEIGHT_DP, (uint32_t)height_dp);
		im_apply_prop(tree);

		if (rim_last_widget_event(1) == RIM_EVENT_WINDOW_CLOSE) {
			struct RimContext *ctx = rim_get_global_ctx();
			rim_last_widget_detach(1);
			rim_end_widget(tree);
			(*is_open) = 0;
			return 0;
		}
		return 1;
	}
	return 0;
}

int im_begin_tab_bar(int *selected) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_TAB_BAR, -1);
	im_apply_prop(tree);
	return IM_CHILDREN_VISIBLE;
}

int im_begin_tab(const char *title) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_TAB, -1);
	rim_add_prop_string(tree, RIM_PROP_WIN_TITLE, title);
	im_apply_prop(tree);
	return IM_CHILDREN_VISIBLE;
}

void im_entry(const char *label, char *buffer, unsigned int size) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_ENTRY, 0);
	rim_add_prop_string(tree, RIM_PROP_TEXT, buffer);
	rim_add_prop_string(tree, RIM_PROP_LABEL, label);
	im_apply_prop(tree);
	rim_end_widget(tree);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		snprintf(buffer, size, "%s", (char *)ctx->last_event.data);
	}
}

void im_multiline_entry(char *buffer, unsigned int size) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_MULTILINE_ENTRY, 0);
	rim_add_prop_string(tree, RIM_PROP_TEXT, buffer);
	im_apply_prop(tree);
	rim_end_widget(tree);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		snprintf(buffer, size, "%s", (char *)ctx->last_event.data);
	}
}

void im_slider(int min, int max, int *value) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_SLIDER, 0);
	rim_add_prop_u32(tree, RIM_PROP_SLIDER_VALUE, (uint32_t)(*value));
	rim_add_prop_u32(tree, RIM_PROP_SLIDER_MAX, (uint32_t)max);
	rim_add_prop_u32(tree, RIM_PROP_SLIDER_MIN, (uint32_t)min);
	im_apply_prop(tree);
	rim_end_widget(tree);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		memcpy(value, ctx->last_event.data, 4);
	}
}

void im_end_window(void) { im_end(RIM_WINDOW); }
void im_end_tab(void) { im_end(RIM_TAB); }
void im_end_tab_bar(void) { im_end(RIM_TAB_BAR); }
