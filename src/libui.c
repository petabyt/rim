// LibUI backend for rim
// Copyright Daniel C 2025
#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include <assert.h>
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
	case RIM_LAYOUT_STATIC: {
		int index = rim_get_child_index(w, parent);
		if (index == -1) rim_abort("child index failed\n");
		uiBoxDelete((uiBox *)parent->os_handle, index);
		} return 0;
	}
	return 1;
}

int rim_backend_destroy(struct RimContext *ctx, struct WidgetHeader *w) {
	switch (w->type) {
	case RIM_TAB:
	case RIM_WINDOW:
	case RIM_BUTTON:
	case RIM_LABEL:
	case RIM_LAYOUT_STATIC:
	case RIM_ENTRY:
		uiControlDestroy((uiControl *)w->os_handle);
		return 0;
	}
	return 1;
}

static int window_closed(uiWindow *w, void *arg) {
	uiQuit();
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

	pthread_mutex_lock(&ctx->event_mutex);

	char *text = uiEntryText(entry);
	ctx->last_event.data_length = strlen(text) + 1;
	ctx->last_event.data = malloc(ctx->last_event.data_length);
	strcpy(ctx->last_event.data, text);
	rim_on_widget_event(ctx, RIM_EVENT_VALUE_CHANGED, (int)(uintptr_t)arg);
	pthread_mutex_unlock(&ctx->event_mutex);
}

int rim_backend_create(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;
	struct RimProp prop;
	switch (w->type) {
	case RIM_WINDOW: {
		assert(rim_get_prop(w, &prop, RIM_PROP_WIN_TITLE) == 0);

		uint32_t win_width, win_height;
		assert(rim_get_prop_u32(w, RIM_PROP_WIN_WIDTH, &win_width) == 0);
		assert(rim_get_prop_u32(w, RIM_PROP_WIN_HEIGHT, &win_height) == 0);

		win_width = rim_dp_to_px(win_width);
		win_height = rim_dp_to_px(win_height);

		uiWindow *handle = uiNewWindow(prop.value, (int)win_width, (int)win_height, 0);
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
		assert(rim_get_prop(w, &prop, RIM_PROP_TEXT) == 0);
		uiButton *handle = uiNewButton(prop.value);
		uiButtonOnClicked(handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_LABEL: {
		assert(rim_get_prop(w, &prop, RIM_PROP_TEXT) == 0);
		uiLabel *handle = uiNewLabel(prop.value);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_ENTRY: {
		assert(rim_get_prop(w, &prop, RIM_PROP_TEXT) == 0);
		uiEntry *handle = uiNewEntry();
		uiEntrySetText(handle, prop.value);
		uiEntryOnChanged(handle, on_changed, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case RIM_LAYOUT_STATIC: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
		} return 0;
	case RIM_TAB_BAR: {
		w->os_handle = (uintptr_t)uiNewTab();
		} return 0;
	case RIM_TAB: {
		w->os_handle = (uintptr_t)uiNewVerticalBox();
		} return 0;
	}
	return 1;
}
int rim_backend_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	struct Priv *p = ctx->priv;
	if (parent == NULL) {
		// Handle root element, being appended to nothing?
		return 0;
	}

	struct RimProp prop;

	switch (parent->type) {
	case RIM_WINDOW:
		if (p->make_window_a_layout) {
			uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		} else {
			uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
		}
		return 0;
	case RIM_LAYOUT_STATIC:
	case RIM_TAB:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		return 0;
	case RIM_TAB_BAR:
		assert(rim_get_prop(w, &prop, RIM_PROP_WIN_TITLE) == 0);
		uiTabAppend((uiTab *)parent->os_handle, (const char *)prop.value, (uiControl *)w->os_handle);
		return 0;
	}
	return 1;
}

int rim_backend_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type) {
	switch (w->type) {
	case RIM_LABEL:
		if (prop->type == RIM_PROP_TEXT) {
			uiLabelSetText((uiLabel *)w->os_handle, (const char *)prop->data);
			return 0;
		}
	case RIM_ENTRY:
		if (prop->type == RIM_PROP_TEXT) {
			uiEntrySetText((uiEntry *)w->os_handle, (const char *)prop->data);
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
