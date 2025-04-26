// LibUI scintilla extension for rim
// April 2025
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <rim_internal.h>
#include <signal.h>
#include <string.h>
#include <rim_internal.h>
#include <rim.h>
#include <im.h>
#include <ui.h>
#include <ui_scintilla.h>

#define EXTENSION_ID 0x5C1

#define RIM_SCINTILLA 0x1000

struct Priv {
	int cache;
};

int im_scintilla(void) {
	struct RimTree *tree = rim_get_current_tree();
	rim_add_widget(tree, RIM_SCINTILLA, 0);
	rim_add_prop_u32(tree, RIM_PROP_EXPAND, 100);
//	rim_add_prop_string(tree, RIM_PROP_TEXT, label);
	rim_end_widget(tree);
	return IM_CHILDREN_VISIBLE;
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
		w->os_handle = (uintptr_t)uiNewScintilla();
		return 0;
	}
	return 1;
}

static int rim_sc_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	return 1;
}

static int rim_sc_tweak(void *priv, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type) {
	return 1;
}

int rim_scintilla_init(struct RimContext *ctx) {
	struct RimExtension ext = {
		.create = rim_sc_create,
		.append = rim_sc_append,
		.tweak = rim_sc_tweak,
		.remove = rim_sc_remove,
		.destroy = rim_sc_destroy,
	};

	rim_add_extension(ctx, &ext);
	return 0;
}
