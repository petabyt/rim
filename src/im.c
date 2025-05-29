// Immediate-mode 'im_' API implementation for Rim
//#include <stdio.h>
#include <string.h>
#include "rim_internal.h"
#include "im.h"

struct Props {
	int gap;
	int expand;
	int inner_padding;
	int disabled;
	int favicon;
	int begin_disabled;
	int tooltip;
	const char *tooltip_text_ptr;

	void *favicon_dest;
	unsigned int favicon_len;
}props = {0};

void im_set_next_tooltip(const char *text) {
	props.tooltip_text_ptr = text; // TODO: undefined behavior, use strcpy instead
	props.tooltip = 1;
}
void im_set_next_window_favicon(void *data, unsigned int length) {
	props.favicon_dest = data;
	props.favicon_len = length;
	props.favicon = 1;
}
void im_set_next_expand(void) {
	props.expand = 1;
}
void im_set_next_gap(int dp) {
	props.gap = dp;
}
void im_set_next_inner_padding(int dp) {
	props.inner_padding = dp;
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
void im_apply_prop(void) {
	struct RimTree *tree = rim_get_current_tree();
	if (props.expand) {
		rim_add_prop_u32(tree, RIM_PROP_EXPAND, 100);
		props.expand = 0;
	}
	if (props.gap) {
		rim_add_prop_u32(tree, RIM_PROP_GAP, props.gap);
		props.gap = 0;
	}
	if (props.disabled || props.begin_disabled) {
		rim_add_prop_u32(tree, RIM_PROP_DISABLED, 1);
		props.disabled = 0;
	}
	if (props.tooltip) {
		rim_add_prop_string(tree, RIM_PROP_TOOLTIP, props.tooltip_text_ptr);
		props.tooltip = 0;
	}
	if (props.inner_padding) {
		rim_add_prop_u32(tree, RIM_PROP_INNER_PADDING, props.inner_padding);
		props.inner_padding = 0;
	}
}

static int im_end(uint32_t expected_type) {
	struct RimTree *tree = rim_get_current_tree();
	rim_end_widget(tree, expected_type);
	return 0;
}

int im_button(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_BUTTON);
	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	im_apply_prop();
	rim_end_widget(tree, RIM_BUTTON);
	return rim_last_widget_event(0);
}

int im_label(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_LABEL);
	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	im_apply_prop();
	rim_end_widget(tree, RIM_LABEL);
	return 0;
}

int im_begin_vertical_box(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_VERTICAL_BOX);
	im_apply_prop();
	return 1;
}

void im_end_vertical_box(void) {
	im_end(RIM_VERTICAL_BOX);
}

int im_begin_horizontal_box(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_HORIZONTAL_BOX);
	im_apply_prop();
	return 1;
}

void im_end_horizontal_box(void) {
	im_end(RIM_HORIZONTAL_BOX);
}

static void window(struct RimTree *tree, const char *name, int width_dp, int height_dp) {
	rim_add_widget(tree, RIM_WINDOW);
	rim_add_prop_string(tree, RIM_PROP_TITLE, name);
	rim_add_prop_u32(tree, RIM_PROP_WIDTH_DP, (uint32_t)width_dp);
	rim_add_prop_u32(tree, RIM_PROP_HEIGHT_DP, (uint32_t)height_dp);
	im_apply_prop();
	if (props.favicon) {
		struct RimPropData dat = {
			.res0 = 0,
			.length = props.favicon_len,
			.ptr = (uintptr_t)props.favicon_dest,
		};
		rim_add_prop_data(tree, RIM_PROP_WIN_ICON_DATA, &dat, sizeof(dat));
		props.favicon = 0;
	}
}

int im_begin_window(const char *name, int width_dp, int height_dp) {
	struct RimTree *tree = rim_get_current_tree();
	window(tree, name, width_dp, height_dp);

	if (rim_last_widget_event(1) == RIM_EVENT_WINDOW_CLOSE) {
		struct RimContext *ctx = rim_get_global_ctx();
		ctx->quit_immediately = 1;
		rim_end_widget(tree, RIM_WINDOW);
		return 0;
	}

	return 1;
}

int im_begin_window_ex(const char *name, int width_dp, int height_dp, int *is_open) {
	struct RimTree *tree = rim_get_current_tree();
	if (*is_open) {
		window(tree, name, width_dp, height_dp);

		if (rim_last_widget_event(1) == RIM_EVENT_WINDOW_CLOSE) {
			rim_last_widget_detach(1);
			rim_end_widget(tree, RIM_WINDOW);
			(*is_open) = 0;
			return 0;
		}
		return 1;
	}
	return 0;
}

int im_begin_tab_bar(int *selected) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_TAB_BAR);
	im_apply_prop();
	return IM_CHILDREN_VISIBLE;
}

int im_begin_tab(const char *title) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_TAB);
	rim_add_prop_string(tree, RIM_PROP_TITLE, title);
	im_apply_prop();
	return IM_CHILDREN_VISIBLE;
}

static void copy_string_from_event(struct RimContext *ctx, char *buffer, unsigned int max) {
	if (ctx->last_event.data_length > max) {
		memcpy(buffer, ctx->last_event.data, max);
		buffer[max - 1] = '\0';
	} else {
		memcpy(buffer, ctx->last_event.data, ctx->last_event.data_length);
	}
}

void im_entry(const char *label, char *buffer, unsigned int size) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_ENTRY);
	rim_add_prop_string(tree, RIM_PROP_TEXT, buffer);
	rim_add_prop_string(tree, RIM_PROP_LABEL, label);
	im_apply_prop();
	rim_end_widget(tree, RIM_ENTRY);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		copy_string_from_event(ctx, buffer, size);
	}
}

void im_multiline_entry(char *buffer, unsigned int size) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_MULTILINE_ENTRY);
	rim_add_prop_string(tree, RIM_PROP_TEXT, buffer);
	im_apply_prop();
	rim_end_widget(tree, RIM_MULTILINE_ENTRY);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		copy_string_from_event(ctx, buffer, size);
	}
}

void im_slider(int min, int max, int *value) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_SLIDER);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_VALUE, (uint32_t)(*value));
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_MAX, (uint32_t)max);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_MIN, (uint32_t)min);
	im_apply_prop();
	rim_end_widget(tree, RIM_SLIDER);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		memcpy(value, ctx->last_event.data, 4);
	}
}

int im_begin_combo_box(const char *label, int *selected) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_COMBOBOX);
	rim_add_prop_string(tree, RIM_PROP_LABEL, label);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_VALUE, (*selected));
	if (rim_last_widget_event(1) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		memcpy(selected, ctx->last_event.data, 4);
	}
	return 1;
}
void im_combo_box_item(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_COMBOBOX_ITEM);
	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	rim_end_widget(tree, RIM_COMBOBOX_ITEM);
}
void im_end_combo_box(void) {
	im_end(RIM_COMBOBOX);
}

int im_begin_radio(int *selected) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_RADIO);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_VALUE, (*selected));
	if (rim_last_widget_event(1) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		memcpy(selected, ctx->last_event.data, 4);
	}
	return 1;
}
void im_radio_item(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_RADIO_ITEM);
	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	rim_end_widget(tree, RIM_RADIO_ITEM);
}
void im_end_radio(void) {
	im_end(RIM_RADIO);
}

void im_progress_bar(int progress) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_PROGRESS_BAR);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_VALUE, (uint32_t)progress);
	im_apply_prop();
	rim_end_widget(tree, RIM_PROGRESS_BAR);
}

int im_begin_menu_bar(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_WINDOW_MENU_BAR);
	return 1;
}

int im_begin_menu(const char *title) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_WINDOW_MENU);
	rim_add_prop_string(tree, RIM_PROP_TEXT, title);
	im_apply_prop();
	return 1;
}

int im_menu_item(const char *title) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_WINDOW_MENU_ITEM);
	rim_add_prop_string(tree, RIM_PROP_TEXT, title);
	im_apply_prop();
	rim_end_widget(tree, RIM_WINDOW_MENU_ITEM);
	return rim_last_widget_event(0);
}

void im_end_menu(void) {
	im_end(RIM_WINDOW_MENU);
}

void im_end_menu_bar(void) {
	im_end(RIM_WINDOW_MENU_BAR);
}

void im_spinbox(int min, int max, int *value) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_SPINBOX);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_MIN, (uint32_t)min);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_MAX, (uint32_t)max);
	rim_add_prop_u32(tree, RIM_PROP_NUMBER_VALUE, (uint32_t)(*value));
	im_apply_prop();
	rim_end_widget(tree, RIM_SPINBOX);
	if (rim_last_widget_event(0) == RIM_EVENT_VALUE_CHANGED) {
		struct RimContext *ctx = rim_get_global_ctx();
		memcpy(value, ctx->last_event.data, 4);
	}
}

int im_begin_form(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_FORM);
	im_apply_prop();
	return IM_CHILDREN_VISIBLE;
}
int im_begin_form_entry(const char *label) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_FORM_ENTRY);
	rim_add_prop_string(tree, RIM_PROP_LABEL, label);
	im_apply_prop();
	return IM_CHILDREN_VISIBLE;
}
void im_end_form_entry(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_end_widget(tree, RIM_FORM_ENTRY);
}
void im_end_form(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_end_widget(tree, RIM_FORM);
}

void im_end_window(void) { im_end(RIM_WINDOW); }
void im_end_tab(void) { im_end(RIM_TAB); }
void im_end_tab_bar(void) { im_end(RIM_TAB_BAR); }
