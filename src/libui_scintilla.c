// LibUI scintilla extension for rim
// This is an extension that provides both retained and immediate mode APIs.
#include <stdlib.h>
#include <pthread.h>
#include <rim_internal.h>
#include <string.h>
#include <rim_internal.h>
#include <rim.h>
#include <rim_internal.h>
#include <im.h>
#include <ui.h>
#include <ui_scintilla.h>
#include <Scintilla.h>

#define EXTENSION_ID 0x5C1
#define RIM_SCINTILLA 0x1000
#define RIM_PROP_SC_INIT 0x1000

struct Priv {
	unsigned int list_len;
	struct RetainedList {
		int id;
		uiScintilla *handle;
	}list[5];
};

typedef void sc_init(uiScintilla *handle);

int im_scintilla(int id, void (*init)(uiScintilla *handle)) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_SCINTILLA);
	rim_add_prop_u32(tree, RIM_PROP_EXPAND, 100);
	rim_add_prop_u32(tree, RIM_PROP_SECONDARY_ID, id);
	uint64_t ptr = (uint64_t)(uintptr_t)init;
	rim_add_prop_u64(tree, RIM_PROP_SC_INIT, ptr);
	rim_end_widget(tree, RIM_SCINTILLA);
	return RIM_EVENT_VALUE_CHANGED;
}

char *rim_scintilla_get_text(rim_ctx_t *ctx, int id) {
	struct Priv *p = (struct Priv *)rim_get_ext_priv(ctx, EXTENSION_ID);
	if (p == NULL) rim_abort("Couldn't find extension\n");
	uiScintilla *sc = NULL;
	for (unsigned int i = 0; i < p->list_len; i++) {
		if (p->list[i].id == id) {
			sc = p->list[i].handle;
		}
	}
	if (sc == NULL) rim_abort("Couldn't find sc from handle\n");
	return uiScintillaText(sc);
}

static int rim_sc_remove(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	return 1;
}

static int rim_sc_destroy(void *priv, struct WidgetHeader *w) {
	if (w->type == RIM_SCINTILLA) {
		uiControlDestroy((uiControl *)w->os_handle);
		return 0;
	}
	return 1;
}

static int rim_sc_create(void *priv, struct WidgetHeader *w) {
	struct Priv *p = priv;
	if (w->type == RIM_SCINTILLA) {
		uint32_t id = 0;
		check_prop(rim_get_prop_u32(w, RIM_PROP_SECONDARY_ID, &id));
		uiScintilla *sc = uiNewScintilla();
		p->list[p->list_len].id = (int)id;
		p->list[p->list_len].handle = sc;
		p->list_len++;
		w->os_handle = (uintptr_t)sc;
		{
			uint64_t ptr;
			check_prop(rim_get_prop_u64(w, RIM_PROP_SC_INIT, (uint64_t *)&ptr));
			sc_init *init = (sc_init *)(void *)(uintptr_t)ptr;
			init(sc);
		}
		return 0;
	}
	return 1;
}

static int rim_sc_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	return 1;
}

static int rim_sc_tweak(void *priv, struct WidgetHeader *w, struct PropHeader *prop, enum RimPropTrigger type) {
	// No properties to change
	return 0;
}

int rim_scintilla_init(struct RimContext *ctx) {
	struct RimExtension ext = {
		.create = rim_sc_create,
		.append = rim_sc_append,
		.tweak = rim_sc_tweak,
		.remove = rim_sc_remove,
		.destroy = rim_sc_destroy,
		.ext_id = EXTENSION_ID,
		.priv = calloc(1, sizeof(struct Priv)),
	};
	rim_add_extension(ctx, &ext);
	return 0;
}
