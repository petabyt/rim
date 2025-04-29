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

	sem_t wait_until_ready;
};

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
	case RIM_TAB:
	case RIM_WINDOW:
		if (p->make_window_a_layout) {
			uiControlDestroy(uiControlParent((uiControl *)w->os_handle));
			return 0;
		}
	case RIM_BUTTON:
	case RIM_LABEL:
	case RIM_HORIZONTAL_BOX:
	case RIM_VERTICAL_BOX:
	case RIM_ENTRY:
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

int rim_backend_create(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;
	char *string = NULL;
	switch (w->type) {
	case RIM_WINDOW: {
		check_prop(rim_get_prop_string(w, RIM_PROP_WIN_TITLE, &string));

		uint32_t win_width, win_height;
		check_prop(rim_get_prop_u32(w, RIM_PROP_WIDTH_DP, &win_width));
		check_prop(rim_get_prop_u32(w, RIM_PROP_HEIGHT_DP, &win_height));

		win_width = rim_dp_to_px(win_width);
		win_height = rim_dp_to_px(win_height);

		uiWindow *handle = uiNewWindow(string, (int)win_width, (int)win_height, 0);
		uiWindowOnClosing(handle, window_closed, (void *)(uintptr_t)w->unique_id);
		if (p->make_window_a_layout) {
			uiBox *container = uiNewVerticalBox();
			uiWindowSetChild(handle, uiControl(container));
			uiControlShow(uiControl(handle));
			w->os_handle = (uintptr_t)container;
		} else {
			w->os_handle = (uintptr_t)handle;
		}
		} return 0;
	case RIM_BUTTON: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiButton *handle = uiNewButton(string);
		uiButtonOnClicked(handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_LABEL: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiLabel *handle = uiNewLabel(string);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_ENTRY: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiEntry *handle = uiNewEntry();
		uiEntrySetText(handle, string);
		uiEntryOnChanged(handle, on_changed, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_MULTILINE_ENTRY: {
		check_prop(rim_get_prop_string(w, RIM_PROP_TEXT, &string));
		uiMultilineEntry *handle = uiNewMultilineEntry();
		uiMultilineEntrySetText(handle, string);
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
		check_prop(rim_get_prop_u32(w, RIM_PROP_SLIDER_MIN, &min));
		check_prop(rim_get_prop_u32(w, RIM_PROP_SLIDER_MAX, &max));
		uiSlider *handle = uiNewSlider((int)min, (int)max);
		uiSliderOnChanged(handle, on_slider, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	}
	return 1;
}

int rim_backend_update_id(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;
	char *string = NULL;
	switch (w->type) {
	case RIM_WINDOW: {
		uiWindowOnClosing((uiWindow *)w->os_handle, window_closed, (void *)(uintptr_t)w->unique_id);
		} return 0;
	case RIM_BUTTON: {
		uiButtonOnClicked((uiButton *)w->os_handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		} return 0;
	case RIM_ENTRY: {
		uiEntryOnChanged((uiEntry *)w->os_handle, on_changed, (void *)(uintptr_t)w->unique_id);
		} return 0;
	case RIM_MULTILINE_ENTRY: {
		uiMultilineEntryOnChanged((uiMultilineEntry *)w->os_handle, on_multiline_changed, (void *)(uintptr_t)w->unique_id);
		} return 0;
	case RIM_SLIDER: {
		uiSliderOnChanged((uiSlider *)w->os_handle, on_slider, (void *)(uintptr_t)w->unique_id);
		} return 0;
	}
	return 1;
}

int rim_backend_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	struct Priv *p = ctx->priv;
	if (parent == NULL) {
		// Handle being appended to root?
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
	}
	return 1;
}

int rim_backend_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type) {
	struct Priv *p = ctx->priv;
	uint32_t val32 = 0;
	memcpy(&val32, prop->data, 4); // assumes len>=4

	if (prop->type == RIM_PROP_DISABLED) {
		uint32_t val;
		memcpy(&val, prop->data, 4);
		if (val == 1) {
			uiControlDisable((uiControl *)w->os_handle);
		} else {
			uiControlEnable((uiControl *)w->os_handle);
		}
		return 0;
	}
	switch (w->type) {
	case RIM_WINDOW:
		if (prop->type == RIM_PROP_INNER_PADDING) {
			if (p->make_window_a_layout) {
				uiBoxSetPadded((uiBox *)w->os_handle, (int)val32);
				return 0;
			}
			return 0;
		}
		if (prop->type == RIM_PROP_WIN_TITLE) {
			if (p->make_window_a_layout) {
				uiWindowSetTitle((uiWindow *)uiControlParent((uiControl *)w->os_handle), (const char *)prop->data);
				return 0;
			}
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
		if (prop->type == RIM_PROP_TEXT) {
			uiEntrySetText((uiEntry *)w->os_handle, (const char *)prop->data);
			return 0;
		}
		break;
	case RIM_MULTILINE_ENTRY:
		if (prop->type == RIM_PROP_TEXT) {
			uiMultilineEntrySetText((uiMultilineEntry *)w->os_handle, (const char *)prop->data);
			return 0;
		}
		break;
	case RIM_SLIDER:
		if (prop->type == RIM_PROP_SLIDER_VALUE) {
			uiSliderSetValue((uiSlider *)w->os_handle, (int)val32);
			return 0;
		}
		break;
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
	uiQuit();
}

int rim_backend_init(struct RimContext *ctx) {
	ctx->priv = malloc(sizeof(struct Priv));
	struct Priv *p = ctx->priv;
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
