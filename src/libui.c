// LibUI backend for rim
// Copyright Daniel C 2025
#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include <pthread.h>
#include <rim.h>
#include <rim_internal.h>
#include <signal.h>
#include <string.h>

struct Priv {
	/// @brief If 1, the root widget of a window will be a uiVerticalBox.
	/// If 0, it will be uiWindow
	int make_window_a_layout;

	pthread_t thread;

	uintptr_t dummy;

	sem_t wait_until_ready;
};

int is_base_control_class(uint32_t type) {
	uint32_t base[] = {
		RIM_LABEL,
		RIM_BUTTON,
		RIM_PROGRESS_BAR,
//		RIM_IMAGE,
		RIM_ENTRY,
		RIM_MULTILINE_ENTRY,
		RIM_SPINBOX,
		RIM_SLIDER,
		RIM_COMBOBOX,
		RIM_RADIO,
		RIM_DATE_PICKER,
		RIM_TAB_BAR,
		RIM_HORIZONTAL_BOX,
		RIM_VERTICAL_BOX,
//		RIM_FLEX_BOX,
		RIM_TABLE,
	};
	for (int i = 0; i < sizeof(base) / sizeof(base[0]); i++) {
		if (base[i] == type) return 1;
	}
	return 0;
}

int rim_backend_remove(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
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

int rim_backend_destroy(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;

	switch (w->type) {
	// Dummy widget
	case RIM_COMBOBOX_ITEM:
		return 0;
	case RIM_TAB:
	case RIM_WINDOW:
		if (p->make_window_a_layout) {
			uiControlDestroy(uiControlParent((uiControl *)w->os_handle));
			return 0;
		}
	case RIM_COMBOBOX:
	case RIM_BUTTON:
	case RIM_LABEL:
	case RIM_HORIZONTAL_BOX:
	case RIM_VERTICAL_BOX:
	case RIM_ENTRY:
	case RIM_PROGRESS_BAR:
		uiControlDestroy((uiControl *)w->os_handle);
		return 0;
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
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, (int)(uintptr_t)arg, text, len);
	uiFreeText(text);
}

static void on_multiline_changed(uiMultilineEntry *entry, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	char *text = uiMultilineEntryText(entry);
	unsigned int len = strlen(text) + 1;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, (int)(uintptr_t)arg, text, len);
	uiFreeText(text);
}

static void on_slider(uiSlider *slider, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	int val = uiSliderValue(slider);
	uint32_t b = (uint32_t)val;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, (int)(uintptr_t)arg, &b, 4);
}

static void on_selected(uiCombobox *combo, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	int val = uiComboboxSelected(combo);
	uint32_t b = (uint32_t)val;
	rim_on_widget_event_data(ctx, RIM_EVENT_VALUE_CHANGED, (int)(uintptr_t)arg, &b, 4);
}

static void on_menu_item_clicked(uiMenuItem *item, uiWindow *sender, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	rim_on_widget_event(ctx, RIM_EVENT_CLICK, (int)(uintptr_t)arg);
}

// LibUI requires that all menus be inited before the uiNewWindow call. So this hack is needed.
static int init_window_menu_bar(struct RimContext *ctx, struct WidgetHeader *bar) {
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

int rim_backend_create(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;
	char *string = NULL;
	switch (w->type) {
	case RIM_WINDOW: {
		int has_menu = 0;

		struct Priv *p = ctx->priv;
		struct WidgetHeader *bar = rim_get_child(w, 0);
		bar->os_handle = p->dummy;
		if (bar->type == RIM_WINDOW_MENU_BAR) {
			has_menu = 1;
			init_window_menu_bar(ctx, bar);
		}

		check_prop(rim_get_prop_string(w, RIM_PROP_WIN_TITLE, &string));

		uint32_t win_width, win_height;
		check_prop(rim_get_prop_u32(w, RIM_PROP_WIDTH_DP, &win_width));
		check_prop(rim_get_prop_u32(w, RIM_PROP_HEIGHT_DP, &win_height));

		win_width = rim_dp_to_px(win_width);
		win_height = rim_dp_to_px(win_height);

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
		rim_mark_prop_fufilled(w, RIM_PROP_WIN_TITLE);
		rim_mark_prop_fufilled(w, RIM_PROP_WIDTH_DP);
		rim_mark_prop_fufilled(w, RIM_PROP_HEIGHT_DP);
		} return 0;
	case RIM_BUTTON: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiButton *handle = uiNewButton(string);
		uiButtonOnClicked(handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fufilled(w, RIM_PROP_TEXT);
		} return 0;
	case RIM_LABEL: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiLabel *handle = uiNewLabel(string);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fufilled(w, RIM_PROP_TEXT);
		} return 0;
	case RIM_ENTRY: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiEntry *handle = uiNewEntry();
		uiEntrySetText(handle, string);
		uiEntryOnChanged(handle, on_changed, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fufilled(w, RIM_PROP_TEXT);
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
		uint32_t min, max, val;
		check_prop(rim_get_prop_u32(w, RIM_PROP_SLIDER_MIN, &min));
		check_prop(rim_get_prop_u32(w, RIM_PROP_SLIDER_MAX, &max));
		uiSlider *handle = uiNewSlider((int)min, (int)max);
		uiSliderOnChanged(handle, on_slider, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		rim_mark_prop_fufilled(w, RIM_PROP_SLIDER_MIN);
		rim_mark_prop_fufilled(w, RIM_PROP_SLIDER_MAX);
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
	case RIM_WINDOW_MENU_ITEM:
	case RIM_WINDOW_MENU_BAR:
	case RIM_WINDOW_MENU:
		if (w->os_handle == 0x0) {
			rim_abort("Menus can only be inited once in LibUI\n");
		}
		return 0;
	case RIM_COMBOBOX_ITEM: {
		w->os_handle = p->dummy;
		} return 0;
	}
	return 1;
}

int rim_backend_update_id(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;
	char *string = NULL;
	switch (w->type) {
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
	case RIM_COMBOBOX:
		uiComboboxOnSelected((uiCombobox *)w->os_handle, on_selected, (void *)(uintptr_t)w->unique_id);
		return 0;
	}
	return 1;
}

int rim_backend_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	struct Priv *p = ctx->priv;
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
		check_prop(rim_get_prop_string(w, RIM_PROP_WIN_TITLE, &title));
		uiTabAppend((uiTab *)parent->os_handle, title, (uiControl *)w->os_handle);
		return 0;
	case RIM_COMBOBOX: {
		uint32_t sel;
		check_prop(rim_get_prop_u32(parent, RIM_PROP_COMBOBOX_SELECTED, &sel));
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &title));
		uiComboboxAppend((uiCombobox *)parent->os_handle, title);
		uiComboboxSetSelected((uiCombobox *)parent->os_handle, (int)sel);
		} return 0;
	}
	return 1;
}

int rim_backend_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type) {
	struct Priv *p = ctx->priv;
	uint32_t val32 = 0;
	memcpy(&val32, prop->data, 4); // assumes len>=4

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
				uiBoxSetPadded((uiBox *)w->os_handle, (int)val32);
				return 0;
			}
			return 1;
		case RIM_PROP_WIN_TITLE:
			if (p->make_window_a_layout) {
				uiWindowSetTitle((uiWindow *)uiControlParent((uiControl *)w->os_handle), (const char *)prop->data);
				return 0;
			}
			return 1;
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
		case RIM_PROP_SLIDER_MAX:
		case RIM_PROP_SLIDER_MIN:
			return 0;
		case RIM_PROP_SLIDER_VALUE:
			uiSliderSetValue((uiSlider *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_COMBOBOX:
		switch (prop->type) {
		case RIM_PROP_LABEL:
			return 0;
		case RIM_PROP_COMBOBOX_SELECTED:
			uiComboboxSetSelected((uiCombobox *)w->os_handle, (int)val32);
			return 0;
		}
		break;
	case RIM_COMBOBOX_ITEM:
		return 0;
	case RIM_PROGRESS_BAR:
		if (prop->type == RIM_PROP_PROGRESS_BAR_VALUE) {
			uiProgressBarSetValue((uiProgressBar *)w->os_handle, (int)val32);
			return 0;
		}
	case RIM_TAB:
		return 0;
	case RIM_TAB_BAR:
		return 0;
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

int rim_backend_run(struct RimContext *ctx, rim_on_run_callback *callback) {
	uiQueueMain(callback, ctx);
	return 0;
}

static void *ui_thread(void *arg) {
	rim_ctx_t *ctx = arg;
	struct Priv *p = ctx->priv;

	uiInitOptions o = { 0 };
	const char *err;

	printf("Calling uiInit\n");
	fflush(stdout);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return NULL;
	}

	sem_post(&p->wait_until_ready);
	uiMain();
	uiUninit();

	return NULL;
}

static void handle_int(int code) {
	printf("Handling sig int");
	struct Priv *p = (struct Priv *)rim_get_global_ctx()->priv;
	pthread_kill(p->thread, 9);
	exit(0);
}

void rim_backend_close(struct RimContext *ctx) {
	struct Priv *p = ctx->priv;
	uiQuit();
}

int rim_backend_init(struct RimContext *ctx) {
	ctx->priv = malloc(sizeof(struct Priv));
	struct Priv *p = ctx->priv;
	p->dummy = (uintptr_t)malloc(10);
	p->make_window_a_layout = 1;
	sem_init(&p->wait_until_ready, 0, 0);

	if (pthread_create(&p->thread, NULL, ui_thread, (void *)ctx) != 0) {
		perror("pthread_create() error");
		return 1;
	}

	signal(SIGINT, handle_int);

	sem_wait(&p->wait_until_ready);
	return 0;
}
