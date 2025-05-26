#include <stdio.h>
#include <stdint.h>
#include "rim_internal.h"

const char *rim_eval_widget_type(uint32_t type) {
	switch (type) {
	case RIM_NONE: return "none";
	case RIM_WINDOW: return "window";
	case RIM_POPUP: return "popup";
	case RIM_LABEL: return "label";
	case RIM_BUTTON: return "button";
	case RIM_PROGRESS_BAR: return "progress_bar";
	case RIM_IMAGE: return "image";
	case RIM_ENTRY: return "entry";
	case RIM_MULTILINE_ENTRY: return "multiline_entry";
	case RIM_PASSWORD_ENTRY: return "password_entry";
	case RIM_SEARCH_ENTRY: return "search_entry";
	case RIM_SPINBOX: return "spinbox";
	case RIM_SLIDER: return "slider";
	case RIM_COMBOBOX: return "combobox";
	case RIM_COMBOBOX_ITEM: return "combobox_item";
	case RIM_RADIO: return "radio";
	case RIM_RADIO_ITEM: return "radio_item";
	case RIM_DATE_PICKER: return "date_picker";
	case RIM_TAB_BAR: return "tab_bar";
	case RIM_TAB: return "tab";
	case RIM_HORIZONTAL_BOX: return "horizontal_box";
	case RIM_VERTICAL_BOX: return "vertical_box";
	case RIM_FLEX_BOX: return "flex_box";
	case RIM_WINDOW_MENU_BAR: return "window_menu_bar";
	case RIM_WINDOW_MENU: return "window_menu";
	case RIM_WINDOW_MENU_ITEM: return "window_menu_item";
	case RIM_TABLE: return "table";
	case RIM_FORM: return "form";
	case RIM_FORM_ENTRY: return "form_entry";
	default: {
		printf("rim_eval_widget_type: unknown %d\n", type);
		return "???";
	}
	}
}

const char *rim_eval_prop_type(uint32_t type) {
	switch (type) {
	case RIM_PROP_NONE: return "none";
	case RIM_PROP_TITLE: return "title";
	case RIM_PROP_WIN_ICON_PATH: return "win_icon_path";
	case RIM_PROP_WIN_ICON_DATA: return "win_icon_data";
	case RIM_PROP_WIDTH_DP: return "width_dp";
	case RIM_PROP_HEIGHT_DP: return "height_dp";
	case RIM_PROP_TEXT: return "text";
	case RIM_PROP_LABEL: return "label";
	case RIM_PROP_EXPAND: return "expand";
	case RIM_PROP_INNER_PADDING: return "inner_padding";
	case RIM_PROP_GAP: return "gap";
	case RIM_PROP_DISABLED: return "disabled";
	case RIM_PROP_TOOLTIP: return "tooltip";
	case RIM_PROP_NUMBER_VALUE: return "number_value";
	case RIM_PROP_NUMBER_MIN: return "number_min";
	case RIM_PROP_NUMBER_MAX: return "number_max";
	case RIM_PROP_ENTRY_READ_ONLY: return "entry_read_only";
	case RIM_PROP_SECONDARY_ID: return "secondary_id";
	case RIM_PROP_META: return "meta";
	default: {
		printf("rim_eval_prop_type: unknown %d\n", type);
		return "???";
	}
	}
}
