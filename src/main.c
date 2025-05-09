// runtime and virtual DOM implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include "rim.h"
#include "rim_internal.h"

// TODO: Move to thread local storage in backend thread?
static struct RimContext *global_context = NULL;

__attribute__((noreturn))
void rim_abort(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
	abort();
}

struct RimContext *rim_init(void) {
	if (global_context != NULL) return global_context;

	struct RimContext *ctx = (struct RimContext *)calloc(1, sizeof(struct RimContext));
	ctx->nop_event_counter = 1; // 1 event for rim_poll to be called twice at beginning
	ctx->current_event_id = 1; // 0 will be an invalid event ID

	ctx->tree_new = rim_create_tree();
	ctx->tree_old = rim_create_tree();

	ctx->last_event.data = malloc(100);
	ctx->last_event.data_buf_size = 100;
	ctx->last_event.data_length = 0;

	sem_unlink("event_signal");
	sem_unlink("backend_done_signal");
	sem_unlink("event_consumed_signal");

	ctx->event_signal = sem_open("event_signal", O_CREAT | O_EXCL, 0, 0);
	ctx->backend_done_signal = sem_open("backend_done_signal", O_CREAT | O_EXCL, 0, 0);
	ctx->event_consumed_signal = sem_open("event_consumed_signal", O_CREAT | O_EXCL, 0, 0);
	if (ctx->event_signal == SEM_FAILED || ctx->backend_done_signal == SEM_FAILED || ctx->event_consumed_signal == SEM_FAILED) {
		rim_abort("sem_open failed %d\n", errno);
	}

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&ctx->event_mutex, &attr);

	global_context = ctx;

	return ctx;
}

struct ThreadArg {
	int (*func)(rim_ctx_t *, void *);
	struct RimContext *ctx;
	void *arg;
};

void *ui_thread(void *arg) {
	struct ThreadArg *thread_arg = (struct ThreadArg *)arg;

	sem_wait(thread_arg->ctx->backend_done_signal);

	thread_arg->func(thread_arg->ctx, thread_arg->arg);

	// TODO: Should signal to rim_start that we are done so this thread isn't killed prematurely

	pthread_exit(NULL);
}

int rim_start(int (*func)(rim_ctx_t *, void *), void *arg) {
	struct RimContext *ctx = rim_init();

	struct ThreadArg thread_arg = {
		.func = func,
		.ctx = ctx,
		.arg = arg,
	};

	if (pthread_create(&ctx->second_thread, NULL, ui_thread, &thread_arg) != 0) {
		perror("pthread_create() error");
		return 1;
	}

	rim_backend_thread(ctx, ctx->backend_done_signal);

	return 0;
}

void rim_close(struct RimContext *ctx) {
	sem_close(ctx->event_signal); sem_unlink("event_signal");
	sem_close(ctx->backend_done_signal); sem_unlink("backend_done_signal");
	sem_close(ctx->event_consumed_signal); sem_unlink("event_consumed_signal");
}

int rim_get_prop_default_value(struct RimContext *ctx, enum RimPropType type, uint8_t *buffer, unsigned int length) {
	if (type == RIM_PROP_DISABLED) {
		uint32_t v = 0;
		memcpy(buffer, &v, 4);
		return 0;
	}
	return 1;
}

void rim_add_extension(struct RimContext *ctx, struct RimExtension *ext) {
	if (ctx->n_exts >= RIM_MAX_EXTS) rim_abort("more than 5 exts\n");
	memcpy(&ctx->exts[ctx->n_exts], ext, sizeof(struct RimExtension));
	ctx->n_exts++;
}

void *rim_get_ext_priv(struct RimContext *ctx, int id) {
	for (int i = 0; i < ctx->n_exts; i++) {
		if (ctx->exts[i].ext_id == id) {
			return ctx->exts[i].priv;
		}
	}
	return NULL;
}

int rim_widget_create(struct RimContext *ctx, struct WidgetHeader *w) {
	int rc = rim_backend_create(ctx, w);
	if (rc == 0) return 0;
	for (int i = 0; i < ctx->n_exts; i++) {
		if (ctx->exts[i].create == NULL) continue;
		rc = ctx->exts[i].create(ctx->exts[i].priv, w);
		if (rc == 0) {
			return 0;
		}
	}

	return -1;
}

int rim_widget_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct PropHeader *prop, enum RimPropTrigger type) {
	// Only these two property types should ever be ignored
	if (prop->type == RIM_PROP_SECONDARY_ID || prop->type == RIM_PROP_META) {
		return 0;
	}

	char temp[sizeof(struct PropHeader) + 100];
	if (type == RIM_PROP_REMOVED) {
		memcpy(temp, prop, sizeof(struct PropHeader));
		prop = (struct PropHeader *)temp;
		prop->length = sizeof(struct PropHeader) + 100;
		if (rim_get_prop_default_value(ctx, prop->type, prop->data, 100)) {
			rim_abort("Failed getting default property value\n");
		}
	}

	int rc = rim_backend_tweak(ctx, w, prop, type);
	if (rc == 0) return 0;
	for (int i = 0; i < ctx->n_exts; i++) {
		if (ctx->exts[i].tweak == NULL) continue;
		rc = ctx->exts[i].tweak(ctx->exts[i].priv, w, prop, type);
		if (rc == 0) {
			return 0;
		}
	}
	return -1;
}

int rim_widget_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	int rc = rim_backend_append(ctx, w, parent);
	if (rc == 0) return 0;
	for (int i = 0; i < ctx->n_exts; i++) {
		if (ctx->exts[i].append == NULL) continue;
		rc = ctx->exts[i].append(ctx->exts[i].priv, w, parent);
		if (rc == 0) {
			return 0;
		}
	}
	return -1;
}

int rim_widget_remove(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	int rc = rim_backend_remove(ctx, w, parent);
	if (rc == 0) return 0;
	for (int i = 0; i < ctx->n_exts; i++) {
		if (ctx->exts[i].remove == NULL) continue;
		rc = ctx->exts[i].remove(ctx->exts[i].priv, w, parent);
		if (rc == 0) {
			return 0;
		}
	}
	return -1;
}

int rim_widget_destroy(struct RimContext *ctx, struct WidgetHeader *w) {
	int rc = rim_backend_destroy(ctx, w);
	if (rc == 0) return 0;
	for (int i = 0; i < ctx->n_exts; i++) {
		if (ctx->exts[i].destroy == NULL) continue;
		rc = ctx->exts[i].destroy(ctx->exts[i].priv, w);
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
	// TODO: Actually get display dp
	return 96;
}
