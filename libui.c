// LibUI backend for nim
// Copyright Daniel C 2025
#include <stdio.h>
#include <stdlib.h>
#include <ui.h>
#include <assert.h>
#include <pthread.h>
#include <nim.h>
#include <nim_internal.h>
#include <signal.h>

struct Priv {
	/// @brief If 1, the root widget of a window will be a uiVerticalBox.
	/// If 0, it will be uiWindow
	int make_window_a_layout;
	pthread_t thread;
};

int onClosing(uiWindow *w, void *data) {
	uiQuit();
	return 1;
}

int on_destroy(void *priv, struct WidgetHeader *w) {
	(void)priv;
	switch (w->type) {
	case NIM_WINDOW:
	case NIM_BUTTON:
	case NIM_LAYOUT_STATIC:
		uiControlDestroy((uiControl *)w->os_handle);
	}
	return 1;
}

void button_clicked(uiButton *button, void *arg) {
	puts("Clicked");
}

int on_create(void *priv, struct WidgetHeader *w) {
	struct Priv *p = priv;
	struct NimProp prop;
	switch (w->type) {
	case NIM_WINDOW: {
		assert(nim_get_prop(w, &prop, NIM_PROP_WIN_TITLE) == 0);
		uiWindow *handle = uiNewWindow(prop.value, 300, 300, 0);
		uiWindowOnClosing(handle, onClosing, NULL);
		if (p->make_window_a_layout) {
			uiBox *container = uiNewVerticalBox();
			uiWindowSetChild(handle, uiControl(container));
			uiControlShow(uiControl(handle));
			w->os_handle = (uintptr_t)container;
		} else {
			w->os_handle = (uintptr_t)handle;
		}
		} return 0;
	case NIM_BUTTON: {
		assert(nim_get_prop(w, &prop, NIM_PROP_TEXT) == 0);
		uiButton *handle = uiNewButton(prop.value);
		uiButtonOnClicked(handle, button_clicked, 0);
		w->os_handle = (uintptr_t)handle;
		} return 0;
	case NIM_LAYOUT_STATIC: {
		w->os_handle = (uintptr_t)uiNewHorizontalBox();
		} return 0;
	}
	return 1;
}
int on_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	struct Priv *p = priv;
	if (parent == NULL) {
		// Handle root element
		return 0;
	}

	switch (parent->type) {
	case NIM_WINDOW:
		if (p->make_window_a_layout) {
			uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		} else {
			uiWindowSetChild((uiWindow *)parent->os_handle, (uiControl *)w->os_handle);
		}
		break;
	case NIM_LAYOUT_STATIC:
		uiBoxAppend((uiBox *)parent->os_handle, (uiControl *)w->os_handle, 0);
		return 0;
	}
	return 1;
}

static void *ui_thread(void *arg) {
	nim_ctx_t *backend = arg;

	uiInitOptions o = {0};
	const char *err;

	printf("Dothing nothing\n");
	fflush(stdout);
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui-ng: %s\n", err);
		uiFreeInitError(err);
		return NULL;
	}

	uiMain();
	uiUninit();

	return NULL;
}

static void handle_int(int code) {
	printf("Handling sig int");
	struct Priv *p = (struct Priv *)nim_get_global_ctx()->priv;
	pthread_kill(p->thread, SIGKILL);
	exit(0);
}

int nim_libui_init(nim_ctx_t *ctx) {
	ctx->create = on_create;
	ctx->append = on_append;
	ctx->priv = malloc(sizeof(struct Priv));
	struct Priv *p = ctx->priv;
	p->make_window_a_layout = 1;

	if (pthread_create(&p->thread, NULL, ui_thread, (void *)ctx) != 0) {
		perror("pthread_create() error");
		return 1;
	}

	signal(SIGINT, handle_int);
}

int nim_libui_ui_rebuild(nim_ctx_t *ctx) {
	return 0;
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
