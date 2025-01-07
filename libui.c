#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include "nim.h"

int onClosing(uiWindow *w, void *data) {
	uiQuit();
	return 1;
}

uint32_t gen_unique_id() {
	static uint32_t x = 1;
	return x++;
}

void on_create(void *priv, struct WidgetHeader *w) {
	switch (w->type) {
	case UI_WINDOW: {
		uiWindow *handle = uiNewWindow("filler", 300, 300, 0);
		uiBox *container = uiNewVerticalBox();
		uiWindowSetChild(handle, uiControl(container));
		uiControlShow(uiControl(handle));
		uiWindowOnClosing(handle, onClosing, NULL);
		w->os_handle = (uintptr_t)container;
		w->unique_id = gen_unique_id();
		} break;
	case UI_BUTTON: {
		uiButton *handle = uiNewButton("filler");
		w->os_handle = (uintptr_t)handle;
		w->unique_id = gen_unique_id();
		} break;
	case UI_LAYOUT_STATIC: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
		w->unique_id = gen_unique_id();
		} break;
	default:
		printf("Unhandled widget type %d\n", w->type);
		break;
	}
}
void on_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	if (parent == NULL) {
		// Handle root element
		return;
	}

	switch (parent->type) {
	case UI_WINDOW:
// uiWindow is a container
//		uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
//		break;
	case UI_LAYOUT_STATIC:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		break;
	default:
		printf("Append widget to parent\n");
		break;
	}
}

int build_my_ui(struct Tree *tree, int state);

int nim_libui_start(void) {
	struct Tree tree;
	tree.widget_stack_depth = 0;
	tree.buffer = malloc(1000);
	tree.of = 0;

	build_my_ui(&tree, 0);

	struct NimBackend backend = {
		.header = NULL,
		.tree = &tree,
		.of = 0,
	};
	backend.create = on_create;
	backend.append = on_append;

	uiInitOptions o = {0};
	const char *err;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	nim_init_tree(&backend, 0, NULL, 0);

	uiMain();
	uiUninit();
	return 0;
}
