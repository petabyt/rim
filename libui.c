// LibUI backend for nim
// Copyright Daniel C 2025
#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include <assert.h>
#include "nim.h"

int onClosing(uiWindow *w, void *data) {
	uiQuit();
	return 1;
}

uint32_t gen_unique_id(void) {
	static uint32_t x = 1;
	return x++;
}

int on_destroy(void *priv, struct WidgetHeader *w) {
	(void)priv;
	switch (w->type) {
	case UI_WINDOW:
		return 0;
	case UI_BUTTON:
		return 0;
	case UI_LAYOUT_STATIC:
		return 0;
	}
	return 1;
}

int on_create(void *priv, struct WidgetHeader *w) {
	(void)priv;
	struct NimProp prop;
	switch (w->type) {
	case UI_WINDOW: {
		assert(nim_get_prop(w, &prop, UI_PROP_WIN_TITLE) == 0);
		uiWindow *handle = uiNewWindow(prop.value, 300, 300, 0);
		uiBox *container = uiNewVerticalBox();
		uiWindowSetChild(handle, uiControl(container));
		uiControlShow(uiControl(handle));
		uiWindowOnClosing(handle, onClosing, NULL);
		w->os_handle = (uintptr_t)container;
		w->unique_id = gen_unique_id();
		} return 0;
	case UI_BUTTON: {
		assert(nim_get_prop(w, &prop, UI_PROP_TEXT) == 0);
		uiButton *handle = uiNewButton(prop.value);
		w->os_handle = (uintptr_t)handle;
		w->unique_id = gen_unique_id();
		} return 0;
	case UI_LAYOUT_STATIC: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
		w->unique_id = gen_unique_id();
		} return 0;
	}
	return 1;
}
int on_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	if (parent == NULL) {
		// Handle root element
		return 0;
	}

	switch (parent->type) {
	case UI_WINDOW:
// uiWindow is a container
//		uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
//		break;
	case UI_LAYOUT_STATIC:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		return 0;
	}
	return 1;
}

int nim_libui_start(void) {
	struct NimBackend backend;
	nim_init_backend(&backend);
	backend.create = on_create;
	backend.append = on_append;

	nim_demo_window1(&backend.tree_old, 0);

	uiInitOptions o = {0};
	const char *err;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	nim_init_tree_widgets(&backend, &backend.tree_old, 0, NULL, 0);

	uiMain();
	uiUninit();
	return 0;
}
