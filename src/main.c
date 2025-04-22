// runtime and virtual DOM implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "rim.h"
#include "rim_internal.h"

// TODO: Move to thread local storage in backend thread?
static struct RimContext *global_context = NULL;

void rim_abort(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
	abort();
}

struct RimContext *rim_init(void) {
	struct RimContext *ctx = (struct RimContext *)calloc(1, sizeof(struct RimContext));
	ctx->header = 0;
	ctx->event_counter = 1; // 1 event for rim_poll to be called twice at beginning

	ctx->tree_new = rim_create_tree();
	ctx->tree_old = rim_create_tree();

	ctx->last_event.data = malloc(100);
	ctx->last_event.data_buf_size = 100;
	ctx->last_event.data_length = 0;

	sem_init(&ctx->event_sig, 0, 0);
	sem_init(&ctx->run_done_signal, 0, 0);
	sem_init(&ctx->event_consumed_sig, 0, 0);

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&ctx->event_mutex, &attr);

	global_context = ctx;

	return ctx;
}

int rim_backend_create(struct RimContext *ctx, struct WidgetHeader *w) {
	void *priv = ctx->priv;
	for (int i = 0; i < ctx->n_backends; i++) {
		int rc = ctx->backends[i].create(priv, w);
		if (rc == 0) {
			return 0;
		}
	}

	return -1;
}

struct RimContext *rim_get_global_ctx(void) {
	if (global_context == NULL) {
		rim_abort("global_context is NULL\n");
	}
	return global_context;
}

struct RimTree *rim_get_current_tree(void) {
	return rim_get_global_ctx()->tree_new;
}

int rim_get_dpi(void) {
	return 96;
}
