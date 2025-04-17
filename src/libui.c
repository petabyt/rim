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

struct Priv {
	/// @brief If 1, the root widget of a window will be a uiVerticalBox.
	/// If 0, it will be uiWindow
	int make_window_a_layout;

	pthread_t thread;

	sem_t wait_until_ready;
};

int on_destroy(struct RimContext *ctx, struct WidgetHeader *w) {
	switch (w->type) {
	case RIM_WINDOW:
	case RIM_BUTTON:
	case RIM_LAYOUT_STATIC:
		uiControlDestroy((uiControl *)w->os_handle);
	}
	return 1;
}

static int window_closed(uiWindow *w, void *arg) {
	uiQuit();
	struct RimContext *ctx = rim_get_global_ctx();
	rim_on_widget_event(ctx, RIM_EVENT_CLICK, (int)(uintptr_t)arg);
	return 1;
}

static void button_clicked(uiButton *button, void *arg) {
	struct RimContext *ctx = rim_get_global_ctx();
	rim_on_widget_event(ctx, RIM_EVENT_CLICK, (int)(uintptr_t)arg);
}

int on_create(struct RimContext *ctx, struct WidgetHeader *w) {
	struct Priv *p = ctx->priv;
	struct RimProp prop;
	switch (w->type) {
	case RIM_WINDOW: {
		assert(rim_get_prop(w, &prop, RIM_PROP_WIN_TITLE) == 0);
		uiWindow *handle = uiNewWindow(prop.value, 300, 300, 0);
		uiWindowOnClosing(handle, window_closed, (void *)(uintptr_t)w->unique_id);
		if (p->make_window_a_layout) {
			uiBox *container = uiNewVerticalBox();
			uiWindowSetChild(handle, uiControl(container));
			uiControlShow(uiControl(handle));
			w->os_handle = (uintptr_t)container;
		} else {
			w->os_handle = (uintptr_t)handle;
		}
	}
		return 0;
	case RIM_BUTTON: {
		assert(rim_get_prop(w, &prop, RIM_PROP_TEXT) == 0);
		uiButton *handle = uiNewButton(prop.value);
		uiButtonOnClicked(handle, button_clicked, (void *)(uintptr_t)w->unique_id);
		w->os_handle = (uintptr_t)handle;
	}
		return 0;
	case RIM_LABEL: {
		assert(rim_get_prop(w, &prop, RIM_PROP_TEXT) == 0);
		uiLabel *handle = uiNewLabel(prop.value);
		w->os_handle = (uintptr_t)handle;
	}
		return 0;
	case RIM_LAYOUT_STATIC: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
	}
		return 0;
	}
	return 1;
}
int on_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	struct Priv *p = ctx->priv;
	if (parent == NULL) {
		// Handle root element, being appended to nothing?
		return 0;
	}

	switch (parent->type) {
	case RIM_WINDOW:
		if (p->make_window_a_layout) {
			uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		} else {
			uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
		}
		return 0;
	case RIM_LAYOUT_STATIC:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		return 0;
	}
	return 1;
}

int on_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop) {
	switch (w->type) {
	case RIM_LABEL:
		if (prop->type == RIM_PROP_TEXT) {
			uiLabelSetText((uiLabel *)w->os_handle, (const char *)prop->data);
			return 0;
		}
	}
	return 1;
}

int on_run(struct RimContext *ctx, rim_on_run_callback *callback) {
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
	pthread_kill(p->thread, SIGKILL);
	exit(0);
}

int rim_libui_init(rim_ctx_t *ctx) {
	ctx->create = on_create;
	ctx->append = on_append;
	ctx->tweak = on_tweak;
	ctx->append = on_append;
	ctx->destroy = on_destroy;
	ctx->run = on_run;

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
