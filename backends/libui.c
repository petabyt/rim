// LibUI backend for nim
// Copyright Daniel C 2025
#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include <assert.h>
#include <pthread.h>
#include <nim.h>
#include <nim_internal.h>

int onClosing(uiWindow *w, void *data) {
	uiQuit();
	return 1;
}

int on_destroy(void *priv, struct WidgetHeader *w) {
	(void)priv;
	switch (w->type) {
	case NIM_WINDOW:
		return 0;
	case NIM_BUTTON:
		return 0;
	case NIM_LAYOUT_STATIC:
		return 0;
	}
	return 1;
}

int on_create(void *priv, struct WidgetHeader *w) {
	(void)priv;
	struct NimProp prop;
	switch (w->type) {
	case NIM_WINDOW: {
		assert(nim_get_prop(w, &prop, NIM_PROP_WIN_TITLE) == 0);
		uiWindow *handle = uiNewWindow(prop.value, 300, 300, 0);
		uiBox *container = uiNewVerticalBox();
		uiWindowSetChild(handle, uiControl(container));
		uiControlShow(uiControl(handle));
		uiWindowOnClosing(handle, onClosing, NULL);
		w->os_handle = (uintptr_t)container;
		} return 0;
	case NIM_BUTTON: {
		assert(nim_get_prop(w, &prop, NIM_PROP_TEXT) == 0);
		uiButton *handle = uiNewButton(prop.value);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case NIM_LAYOUT_STATIC: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
		} return 0;
	}
	return 1;
}
int on_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	(void)priv;
	if (parent == NULL) {
		// Handle root element
		return 0;
	}

	switch (parent->type) {
	case NIM_WINDOW:
// uiWindow is a container currently
//		uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
//		break;
	case NIM_LAYOUT_STATIC:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		return 0;
	}
	return 1;
}

static void *ui_thread(void *arg) {
	struct NimContext *backend = arg;
	//nim_demo_window1(&backend->tree_old, 0);

	uiInitOptions o = {0};
	const char *err;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return NULL;
	}

	nim_init_tree_widgets(backend, backend->tree_old, 0, NULL, 0);

	uiMain();
	uiUninit();

	return NULL;
}

int nim_libui_init(nim_ctx_t *ctx) {
	ctx->create = on_create;
	ctx->append = on_append;
}

#if 0
int nim_libNIM_start(void) {
	struct NimContext backend;
	nim_init_backend(&backend);
	backend.create = on_create;
	backend.append = on_append;

//	sem_t event_signal;
	pthread_t thid;
	void *ret;

	if (pthread_create(&thid, NULL, ui_thread, &backend) != 0) {
		perror("pthread_create() error");
		return 1;
	}

//	sem_init(&event_signal, 0, 1);

	if (pthread_join(thid, &ret) != 0) {
		perror("pthread_create() error");
		return -1;
	}

	return 0;
}
#endif