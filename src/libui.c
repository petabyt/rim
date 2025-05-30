// LibUI backend for rim
// Copyright Daniel C 2025
#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include <pthread.h>
#include <rim.h>
#include <rim_internal.h>
#include <string.h>
#include <fcntl.h>

struct Priv {
	struct RimContext *ctx;
	/// @brief If 1, the root widget of a window will be a uiVerticalBox.
	/// If 0, it will be uiWindow
	/// If more than 1 widget is added to a window, things get weird.
	int make_window_a_layout;

	/// @brief Dummy handle because several widgets don't have a handle
	uintptr_t dummy;
};

int is_base_control_class(uint32_t type) {
	uint32_t base[] = {
		RIM_LABEL,
		RIM_BUTTON,
		RIM_PROGRESS_BAR,
		RIM_ENTRY,
		RIM_MULTILINE_ENTRY,
		RIM_SPINBOX,
		RIM_SLIDER,
		RIM_SPINBOX,
		RIM_COMBOBOX,
		RIM_RADIO,
		RIM_DATE_PICKER,
		RIM_TAB_BAR,
		RIM_HORIZONTAL_BOX,
		RIM_VERTICAL_BOX,
		RIM_TABLE,
	};
	for (int i = 0; i < sizeof(base) / sizeof(base[0]); i++) {
		if (base[i] == type) return 1;
	}
	return 0;
}

static int get_prop_rules(void *priv, const struct WidgetHeader *w, const struct PropHeader *p) {
	int flag = 0;
	switch (w->type) {
	case RIM_COMBOBOX:
	case RIM_RADIO:
	case RIM_TAB_BAR:
	case RIM_SPINBOX:
		// LibUI doesn't like it when you run eg uiComboboxSetSelected before appending entries
		if (p->type == RIM_PROP_NUMBER_VALUE) {
			flag |= RIM_FLAG_SET_PROP_AFTER_CHILDREN;
		}
		break;
	}
	return flag;
}

static int get_widget_rules(void *priv, const struct WidgetHeader *w, const struct WidgetHeader *parent) {
	int flag = 0;
	switch (w->type) {
	case RIM_FORM_ENTRY:
		// RIM_FORM_ENTRY is dummy node with a child
		flag |= RIM_FLAG_INIT_CHILDREN_FIRST;
		break;
	}
	return flag;
}

static int rim_backend_remove(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	if (parent == NULL) {
		if (w->type != RIM_WINDOW) {
			rim_abort("Only windows should be removed from root...\n");
		}
		// Is this even needed?
		uiControlHide((uiControl *)w->os_handle);
		return 0;
	}

	switch (parent->type) {
	case RIM_COMBOBOX: {
		int index = rim_get_child_index(w, parent);
		if (index == -1) rim_abort("child index failed\n");
		uiComboboxDelete((uiCombobox *)parent->os_handle, index);
		} return 0;
	case RIM_RADIO: {
		// WTF libui doesn't have uiRadioButtonsDelete, your app will get screwed
		} return 0;
	case RIM_TAB_BAR: {
		int index = rim_get_child_index(w, parent);
		if (index == -1) rim_abort("child index failed\n");
		uiTabDelete((uiTab *)parent->os_handle, index);
		} return 0;
	case RIM_WINDOW:
	case RIM_VERTICAL_BOX:
	case RIM_HORIZONTAL_BOX: {
		int index = rim_get_child_index(w, parent);
		if (index == -1) rim_abort("child index failed\n");
		uiBoxDelete((uiBox *)parent->os_handle, index);
		} return 0;
	}
	return 1;
}

static int rim_backend_destroy(void *priv, struct WidgetHeader *w) {
	struct Priv *p = priv;
	if (is_base_control_class(w->type)) {
		uiControlDestroy((uiControl *)w->os_handle);
		return 0;
	}
	switch (w->type) {
	// Dummy widget
	case RIM_RADIO_ITEM:
	case RIM_COMBOBOX_ITEM:
		return 0;
	case RIM_TAB:
	case RIM_WINDOW:
		if (p->make_window_a_layout) {
			uiControlDestroy(uiControlParent((uiControl *)w->os_handle));
			return 0;
		}
	}
	return 1;
}

static int window_closed(uiWindow *w, void *arg) {
	// let event handler run uiQuit()
	struct RimContext *ctx = rim_get_global_ctx();
	rim_on_widget_event(ctx, RIM_EVENT_WINDOW_CLOSE, (int)(uintptr_t)arg);
	return 1;
}

static void button_clicked(uiButton *button, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	rim_on_widget_event(ctx, RIM_EVENT_CLICK, (int)(uintptr_t)arg);
}

static void on_changed(uiEntry *entry, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	char *text = uiEntryText(entry);
	unsigned int len = strlen(text) + 1;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, RIM_PROP_TEXT, (int)(uintptr_t)arg, text, len);
	uiFreeText(text);
}

static void on_multiline_changed(uiMultilineEntry *entry, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	char *text = uiMultilineEntryText(entry);
	unsigned int len = strlen(text) + 1;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, RIM_PROP_TEXT, (int)(uintptr_t)arg, text, len);
	uiFreeText(text);
}

static void on_slider(uiSlider *handle, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	int val = uiSliderValue(handle);
	uint32_t b = (uint32_t)val;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, RIM_PROP_NUMBER_VALUE, (int)(uintptr_t)arg, &b, 4);
}

static void on_spinbox(uiSpinbox *handle, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	int val = uiSpinboxValue(handle);
	uint32_t b = (uint32_t)val;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, RIM_PROP_NUMBER_VALUE, (int)(uintptr_t)arg, &b, 4);
}

static void on_radio(uiRadioButtons *handle, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	int val = uiRadioButtonsSelected(handle);
	uint32_t b = (uint32_t)val;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, RIM_PROP_NUMBER_VALUE, (int)(uintptr_t)arg, &b, 4);
}

static void on_selected(uiCombobox *handle, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	int val = uiComboboxSelected(handle);
	uint32_t b = (uint32_t)val;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, RIM_PROP_NUMBER_VALUE, (int)(uintptr_t)arg, &b, 4);
}

static void on_menu_item_clicked(uiMenuItem *handle, uiWindow *sender, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	rim_on_widget_event(ctx, RIM_EVENT_CLICK, (int)(uintptr_t)arg);
}

// LibUI requires that all menus be inited before the uiNewWindow call. So this hack is needed.
static int init_window_menu_bar(struct WidgetHeader *bar) {
	for (int i = 0; i < bar->n_children; i++) {
		struct WidgetHeader *menu_h = rim_get_child(bar, i);
		if (menu_h->type != RIM_WINDOW_MENU) rim_abort("Menu not in menu bar\n");
		char *string;
		check_prop(rim_get_prop_string(menu_h, RIM_PROP_TEXT, &string));
		uiMenu *menu = uiNewMenu(string);
		menu_h->os_handle = (uintptr_t)menu;
		for (int y = 0; y < menu_h->n_children; y++) {
			struct WidgetHeader *item_h = rim_get_child(menu_h, y);
			if (item_h->type != RIM_WINDOW_MENU_ITEM) rim_abort("Item not in menu\n");
			check_prop(rim_get_prop_string(item_h, RIM_PROP_TEXT, &string));
			uiMenuItem *item = uiMenuAppendItem(menu, string);
			uiMenuItemOnClicked(item, on_menu_item_clicked, (void *)(uintptr_t)item_h->unique_id);
			item_h->os_handle = (uintptr_t)item;
		}
	}

	return 0;
}

static int rim_backend_create(void *priv, struct WidgetHeader *w) {
	struct Priv *p = priv;
	char *string = NULL;
	switch (w->type) {
	case RIM_WINDOW: {
		int has_menu = 0;

		struct WidgetHeader *bar = rim_get_child(w, 0);
		bar->os_handle = p->dummy;
		if (bar->type == RIM_WINDOW_MENU_BAR) {
			has_menu = 1;
			init_window_menu_bar(bar);
		}

		check_prop(rim_get_prop_string(w, RIM_PROP_TITLE, &string));

		uint32_t win_width, win_height;
		check_prop(rim_get_prop_u32(w, RIM_PROP_WIDTH_DP, &win_width));
		check_prop(rim_get_prop_u32(w, RIM_PROP_HEIGHT_DP, &win_height));

//		win_width = rim_dp_to_px(win_width);
//		win_height = rim_dp_to_px(win_height);

		uiWindow *handle = uiNewWindow(string, (int)win_width, (int)win_height, has_menu);
		uiWindowOnClosing(handle, window_closed, (void *)(uintptr_t)w->unique_id);
		if (p->make_window_a_layout) {
			uiBox *container = uiNewVerticalBox();
			uiWindowSetChild(handle, uiControl(container));
			uiControlShow(uiControl(handle));
			w->os_handle = (uintptr_t)container;
		} else {
			w->os_handle = (uintptr_t)handle;
		}
		rim_mark_prop_fulfilled(w, RIM_PROP_TITLE);
		rim_mark_prop_fulfilled(w, RIM_PROP_WIDTH_DP);
		rim_mark_prop_fulfilled(w, RIM_PROP_HEIGHT_DP);
		} return 0;
	case RIM_BUTTON: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiButton *handle = uiNewButton(string);
		uiButtonOnClicked(handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fulfilled(w, RIM_PROP_TEXT);
		} return 0;
	case RIM_LABEL: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiLabel *handle = uiNewLabel(string);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fulfilled(w, RIM_PROP_TEXT);
		} return 0;
	case RIM_ENTRY: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiEntry *handle = uiNewEntry();
		uiEntrySetText(handle, string);
		uiEntryOnChanged(handle, on_changed, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fulfilled(w, RIM_PROP_TEXT);
		} return 0;
	case RIM_MULTILINE_ENTRY: {
		uiMultilineEntry *handle = uiNewNonWrappingMultilineEntry(); // uiNewMultilineEntry
		uiMultilineEntryOnChanged(handle, on_multiline_changed, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_HORIZONTAL_BOX: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
		} return 0;
	case RIM_VERTICAL_BOX: {
		w->os_handle = (uintptr_t)uiNewVerticalBox();
		} return 0;
	case RIM_TAB_BAR: {
		w->os_handle = (uintptr_t)uiNewTab();
		} return 0;
	case RIM_TAB: {
		w->os_handle = (uintptr_t)uiNewVerticalBox();
		} return 0;
	case RIM_SLIDER: {
		uint32_t min, max;
		check_prop(rim_get_prop_u32(w, RIM_PROP_NUMBER_MIN, &min));
		check_prop(rim_get_prop_u32(w, RIM_PROP_NUMBER_MAX, &max));
		uiSlider *handle = uiNewSlider((int)min, (int)max);
		uiSliderOnChanged(handle, on_slider, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fulfilled(w, RIM_PROP_NUMBER_MIN);
		rim_mark_prop_fulfilled(w, RIM_PROP_NUMBER_MAX);
		} return 0;
	case RIM_COMBOBOX: {
		uiCombobox *handle = uiNewCombobox();
		uiComboboxOnSelected(handle, on_selected, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_PROGRESS_BAR: {
		uiProgressBar *handle = uiNewProgressBar();
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_FORM: {
		uiForm *handle = uiNewForm();
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_WINDOW_MENU_ITEM:
	case RIM_WINDOW_MENU_BAR:
	case RIM_WINDOW_MENU:
		if (w->os_handle == 0x0) {
			rim_abort("Menus can only be inited once in LibUI\n");
		}
		return 0;
	case RIM_FORM_ENTRY:
	case RIM_RADIO_ITEM:
	case RIM_COMBOBOX_ITEM: {
		w->os_handle = p->dummy;
		} return 0;
	case RIM_SPINBOX: {
		uint32_t min, max;
		check_prop(rim_get_prop_u32(w, RIM_PROP_NUMBER_MIN, &min));
		check_prop(rim_get_prop_u32(w, RIM_PROP_NUMBER_MAX, &max));
		uiSpinbox *handle = uiNewSpinbox(min, max);
		w->os_handle = (uintptr_t)handle;
		uiSpinboxOnChanged(handle, on_spinbox, (void *)(uintptr_t)w->unique_id);
		rim_mark_prop_fulfilled(w, RIM_PROP_NUMBER_MIN);
		rim_mark_prop_fulfilled(w, RIM_PROP_NUMBER_MAX);
		} return 0;
	case RIM_RADIO: {
		uiRadioButtons *handle = uiNewRadioButtons();
		w->os_handle = (uintptr_t)handle;
		uiRadioButtonsOnSelected(handle, on_radio, (void *)(uintptr_t)w->unique_id);
		} return 0;
	}
	return 1;
}

static int rim_backend_update_id(void *priv, struct WidgetHeader *w) {
	struct Priv *p = priv;
	char *string = NULL;
	switch (w->type) {
	case RIM_RADIO_ITEM:
		return 0;
	case RIM_WINDOW:
		uiWindowOnClosing((uiWindow *)w->os_handle, window_closed, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_WINDOW_MENU_ITEM:
		uiMenuItemOnClicked((uiMenuItem *)w->os_handle, on_menu_item_clicked, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_BUTTON:
		uiButtonOnClicked((uiButton *)w->os_handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_ENTRY:
		uiEntryOnChanged((uiEntry *)w->os_handle, on_changed, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_MULTILINE_ENTRY:
		uiMultilineEntryOnChanged((uiMultilineEntry *)w->os_handle, on_multiline_changed, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_SLIDER:
		uiSliderOnChanged((uiSlider *)w->os_handle, on_slider, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_SPINBOX:
		uiSpinboxOnChanged((uiSpinbox *)w->os_handle, on_spinbox, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_COMBOBOX:
		uiComboboxOnSelected((uiCombobox *)w->os_handle, on_selected, (void *)(uintptr_t)w->unique_id);
		return 0;
	case RIM_RADIO:
		uiRadioButtonsOnSelected((uiRadioButtons *)w->os_handle, on_radio, (void *)(uintptr_t)w->unique_id);
		return 0;
	}
	if (is_rim_widget(w->type)) return 0;
	return 1;
}

static int rim_backend_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	struct Priv *p = priv;
	if (parent == NULL) {
		// Handle being appended to root?
		return 0;
	}
	if (w->type == RIM_WINDOW_MENU_BAR || w->type == RIM_WINDOW_MENU || w->type == RIM_WINDOW_MENU_ITEM) {
		// See init_window_menu_bar
		return 0;
	}

	int expand = 0;
	uint32_t expand_val = 0;
	if (rim_get_prop_u32(w, RIM_PROP_EXPAND, &expand_val) == 0) {
		expand = 1;
	}

	char *title = NULL;

	switch (parent->type) {	
	case RIM_WINDOW:
		if (p->make_window_a_layout) {
			uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, expand);
		} else {
			uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
		}
		return 0;
	case RIM_HORIZONTAL_BOX:
	case RIM_VERTICAL_BOX:
	case RIM_TAB:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, expand);
		return 0;
	case RIM_TAB_BAR:
		check_prop(rim_get_prop_string(w, RIM_PROP_TITLE, &title));
		uiTabAppend((uiTab *)parent->os_handle, title, (uiControl *)w->os_handle);
		return 0;
	case RIM_COMBOBOX: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &title));
		uiComboboxAppend((uiCombobox *)parent->os_handle, title);
		} return 0;
	case RIM_RADIO: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &title));
		uiRadioButtonsAppend((uiRadioButtons *)parent->os_handle, title);
		} return 0;
	case RIM_FORM: {
		check_prop(rim_get_prop_string(w, RIM_PROP_LABEL, &title));
		struct WidgetHeader *entry_child = rim_get_child(w, 0);
		if (entry_child == NULL) rim_abort("no entry_child\n");
		uiFormAppend((uiForm *)parent->os_handle, title, (uiControl *)entry_child->os_handle, 0);
		} return 0;
	case RIM_FORM_ENTRY:
		return 0;
	}
	return 1;
}

static int rim_backend_tweak(void *priv, struct WidgetHeader *w, struct PropHeader *prop, enum RimPropTrigger type) {
	struct Priv *p = priv;
	uint32_t val32 = 0;
	memcpy(&val32, prop->data, 4); // assumes len>=4
	int libui_bool = val32 ? 1 : 0;

	if (prop->type == RIM_PROP_EXPAND) {
		// 'stretchy' can only be set in uiBoxAppend
		return 0;
	}

	switch (w->type) {
	case RIM_WINDOW:
		switch (prop->type) {
		case RIM_PROP_WIDTH_DP:
		case RIM_PROP_HEIGHT_DP:
			return 0;
		case RIM_PROP_INNER_PADDING:
			if (p->make_window_a_layout) {
				uiWindowSetMargined((uiWindow *)uiControlParent((uiControl *)w->os_handle), (int)val32);
				return 0;
			}
			return 1;
		case RIM_PROP_TITLE:
			if (p->make_window_a_layout) {
				uiWindowSetTitle((uiWindow *)uiControlParent((uiControl *)w->os_handle), (const char *)prop->data);
				return 0;
			}
			return 1;
		}
		break;
	case RIM_HORIZONTAL_BOX:
	case RIM_VERTICAL_BOX:
		if (prop->type == RIM_PROP_GAP) {
			uiBoxSetPadded((uiBox *)w->os_handle, libui_bool);
			return 0;
		}
	break;
	case RIM_LABEL:
		if (prop->type == RIM_PROP_TEXT) {
			uiLabelSetText((uiLabel *)w->os_handle, (const char *)prop->data);
			return 0;
		}
		break;
	case RIM_BUTTON:
		if (prop->type == RIM_PROP_TEXT) {
			uiButtonSetText((uiButton *)w->os_handle, (const char *)prop->data);
			return 0;
		}
		break;
	case RIM_ENTRY:
		switch (prop->type) {
		case RIM_PROP_LABEL:
			return 0;
		case RIM_PROP_TEXT:
			uiEntrySetText((uiEntry *)w->os_handle, (const char *)prop->data);
			return 0;
		case RIM_PROP_ENTRY_READ_ONLY:
			uiEntrySetReadOnly((uiEntry *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_MULTILINE_ENTRY:
		if (prop->type == RIM_PROP_TEXT) {
			uiMultilineEntrySetText((uiMultilineEntry *)w->os_handle, (const char *)prop->data);
			return 0;
		} else if (prop->type == RIM_PROP_ENTRY_READ_ONLY) {
			uiMultilineEntrySetReadOnly((uiMultilineEntry *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_SLIDER:
		switch (prop->type) {
		case RIM_PROP_NUMBER_VALUE:
			uiSliderSetValue((uiSlider *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_SPINBOX:
		switch (prop->type) {
		case RIM_PROP_NUMBER_VALUE:
			uiSpinboxSetValue((uiSpinbox *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_RADIO:
		switch (prop->type) {
		case RIM_PROP_NUMBER_VALUE:
			if (w->n_children > val32) {
				uiRadioButtonsSetSelected((uiRadioButtons *)w->os_handle, (int)val32);
			}
			return 0;
		}
		break;
	case RIM_COMBOBOX:
		switch (prop->type) {
		case RIM_PROP_LABEL:
			return 0;
		case RIM_PROP_NUMBER_VALUE:
			if (w->n_children > val32) {
				uiComboboxSetSelected((uiCombobox *)w->os_handle, (int)val32);
			}
			return 0;
		}
		break;
	case RIM_PROGRESS_BAR:
		if (prop->type == RIM_PROP_NUMBER_VALUE) {
			uiProgressBarSetValue((uiProgressBar *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_COMBOBOX_ITEM:
		if (prop->type == RIM_PROP_TEXT) {
			struct WidgetHeader *parent = (struct WidgetHeader *)(rim_get_current_tree()->buffer + w->parent_of);
			int index = rim_get_child_index(w, parent);
			if (index == -1) rim_abort("child index failed\n");
			uiComboboxDelete((uiCombobox *)parent->os_handle, index);
			uiComboboxInsertAt((uiCombobox *)parent->os_handle, index, (const char *)prop->data);
			// Just screwed with it so it needs to be updated again
			uint32_t sel;
			check_prop(rim_get_prop_u32(parent, RIM_PROP_NUMBER_VALUE, &sel));
			uiComboboxSetSelected((uiCombobox *)parent->os_handle, (int)sel);
			return 0;
		}
		break;
	case RIM_TAB:
		if (prop->type == RIM_PROP_INNER_PADDING) {
			struct WidgetHeader *parent = (struct WidgetHeader *)(rim_get_current_tree()->buffer + w->parent_of);
			int index = rim_get_child_index(w, parent);
			if (index == -1) rim_abort("child index failed\n");
			uiTabSetMargined((uiTab *)parent->os_handle, index, libui_bool);
			return 0;
		}
		if (prop->type == RIM_PROP_GAP) {
			uiBoxSetPadded((uiBox *)w->os_handle, libui_bool);
			return 0;
		}
		if (prop->type == RIM_PROP_TITLE) return 0;
		break;
	case RIM_FORM:
		if (prop->type == RIM_PROP_GAP) {
			uiFormSetPadded((uiForm *)w->os_handle, libui_bool);
			return 0;
		}
		break;
	// Ignore all properties on these widgets for now
	case RIM_FORM_ENTRY:
	case RIM_RADIO_ITEM:
	case RIM_TAB_BAR:
	case RIM_WINDOW_MENU:
	case RIM_WINDOW_MENU_ITEM:
		return 0;
	}
	if (is_base_control_class(w->type)) {
		if (prop->type == RIM_PROP_DISABLED) {
			if (val32 == 1) {
				uiControlDisable((uiControl *)w->os_handle);
			} else {
				uiControlEnable((uiControl *)w->os_handle);
			}
			return 0;
		} else if (prop->type == RIM_PROP_TOOLTIP) {
			uiControlSetTooltip((uiControl *)w->os_handle, (const char *)prop->data);
			return 0;
		}
	}
	return 1;
}

int rim_backend_run(struct RimContext *ctx, rim_on_run_callback *callback, void *arg) {
	uiQueueMain(callback, arg);
	return 0;
}

// Get the old handle for the current window
static struct WidgetHeader *get_current_window_old_tree(void) {
	struct RimTree *tree = rim_get_current_tree();
	if (tree->widget_stack_depth == 0) {
		rim_abort("get_current_window: stack too short\n");
	}
	struct WidgetHeader *w = (struct WidgetHeader *)(tree->buffer + tree->widget_stack[0]);
	if (w->type != RIM_WINDOW) {
		rim_abort("get_current_window: not a window\n");
	}

	// This this code is called in the tree-building phase, we have to get the window handle
	// from the old tree
	tree = rim_get_old_tree();
	unsigned int of = 0;
	int rc = rim_find_in_tree(tree, &of, w->unique_id);
	if (rc) rim_abort("get_current_window: failed to find window in old tree\n");
	w = (struct WidgetHeader *)(tree->buffer + of);
	return w;
}

struct DialogPriv {
	struct RimContext *ctx;
	uiWindow *win;
	int rc;
	char *buffer;
	unsigned int size;
	const char *title;
	const char *desc;
};

static void open_file(void *priv) {
	struct DialogPriv *p = (struct DialogPriv *)priv;

	char *path = uiOpenFile(p->win);
	if (path == NULL) {
		p->rc = IM_CANCELED;
	} else {
		snprintf(p->buffer, p->size, "%s", path);
		uiFreeText(path);
		p->rc = IM_SELECTED;
	}

	sem_post(p->ctx->backend_done_signal);
}

int im_open_file(char *buffer, unsigned int size) {
	struct RimContext *ctx = rim_get_global_ctx();
	struct Priv *p = ctx->backend.priv;

	struct WidgetHeader *window = get_current_window_old_tree();

	uiWindow *w;
	if (p->make_window_a_layout) {
		w = (uiWindow *)uiControlParent((uiControl *)(void *)window->os_handle);
	} else {
		w = (uiWindow *)(void *)window->os_handle;
	}

	struct DialogPriv dp = {
		.ctx = ctx,
		.win = w,
		.rc = 0,
		.buffer = buffer,
		.size = size,
	};

	rim_backend_run(ctx, open_file, (void *)&dp);
	sem_wait(ctx->backend_done_signal);

	return dp.rc;
}

static void destroy(void *arg) {
	uiQuit();
	sem_post(((struct RimContext *)arg)->backend_done_signal);
}

static void rim_backend_close(void *priv) {
	struct RimContext *ctx = rim_get_global_ctx();
	rim_backend_run(ctx, destroy, ctx);
	sem_wait(ctx->backend_done_signal);
	struct Priv *p = ctx->backend.priv;
	free(p);
}

void rim_backend_start(struct RimContext *ctx, sem_t *done) {
	struct Priv *p = malloc(sizeof(struct Priv));
	p->ctx = ctx;
	p->dummy = (uintptr_t)8008135;
	p->make_window_a_layout = 1;
	ctx->backend.priv = p;
	ctx->backend.ext_id = 1;
	ctx->backend.create = rim_backend_create;
	ctx->backend.tweak = rim_backend_tweak;
	ctx->backend.append = rim_backend_append;
	ctx->backend.remove = rim_backend_remove;
	ctx->backend.destroy = rim_backend_destroy;
	ctx->backend.close = rim_backend_close;
	ctx->backend.update_onclick = rim_backend_update_id;
	ctx->backend.get_prop_rules = get_prop_rules;
	ctx->backend.get_widget_rules = get_widget_rules;

	uiInitOptions o = { 0 };
	const char *err;

	printf("Calling uiInit\n");
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return;
	}

	if (done != NULL) {
		sem_post(done);
	}

	uiMain();
	uiUninit();	
}
