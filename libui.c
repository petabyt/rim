#include "tree.h"
#include <ui.h>

struct LibUIContext {
	int n_windows;
	uiWindow *windows[10];
};

static int imgui_wait_for_events(ctx) {
	// If current event tree is NULL:
	//   Return 1 immediately
	// else:
	//   Wait for signal from event queue

	// If imgui onclosing semaphore is signaled, return 0
	return 0;
}

static int imgui_begin(ctx) {

	// If there is a current tree:
	//   Set current tree to previous tree
	// Else:
	//   Allocate a new tree and a previous tree

	return 0;
}

void button_click_handler(uiButton *btn, void *data) {
	trigger_event(ctx, unique_widget_id);
}

static uiControl *create_ui_from_widget(struct UnpackedWidget *w) {
	int unique_widget_id = xxx();
	if (w->type == UI_BUTTON) {
		uiButton *b = uiNewButton(w->text);
		uiButtonOnClicked(b, button_click_handler, (ctx, unique_widget_id));
	}
}

static int imgui_end(ctx) {
	/*
	Start parsing prev and curr tree.
	if (old->type != old->type) {
		// recycle_views(old->os_handle)
		// For now, completely ignore old tree and reconstruct UI for new tree.
	}

	int diff_code = diff_widget(old, new)
	if (diff_code & UI_DIFF_TEXT) {
		uiSetText(old->os_handle)
	}
	*/
}

static void *parser_thread(void *arg) {
	// - Call the main imgui loop
	return NULL;
}

int main() {
	// - start parser thread

	// start libui loop
	return 0;
}
