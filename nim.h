#pragma once
#include <stdint.h>

enum PropTypes {
	UI_PROP_WIN_TITLE = 1,
	UI_PROP_TEXT,
};

enum WidgetTypes {
	UI_WINDOW = 1,
	UI_LABEL,
	UI_BUTTON,
	UI_PROGRESS_BAR,
	UI_IMAGE,
	UI_ENTRY,
	UI_SPINBOX,
	UI_SLIDER,
	UI_COMBOBOX,
	UI_RADIO,
	UI_DATE_PICKER,
	UI_TABLE,
	UI_LAYOUT_STATIC,
	UI_LAYOUT_DYNAMIC,
	UI_LAYOUT_FLEX,
	UI_CUSTOM,
	UI_NATIVE,
	UI_EOF,
};

struct Tree {
	struct WidgetHeader *widget_stack[5];
	int widget_stack_depth;
	uint8_t *buffer;
	int of;
};

struct __attribute__((packed)) WidgetHeader {
	// Internal 32 bit ID
	uint32_t type;
	// Number of allowed children in this widget
	// If 0xffffffff, assume unlimited children
	uint32_t allowed_children;
	// Number of children following this header
	uint32_t n_children;
	// Number of properties following this header
	uint32_t n_props;
	// Unique ID (only modified by UI backend and tree patcher)
	uint32_t unique_id;
	// Pointer handle for UI backend
	uintptr_t os_handle;
	uint8_t data[];
	// properties start here
	// children start here
};

struct NimProp {
	int type;
	const char *value;
};

void nim_add_widget(struct Tree *tree, enum WidgetTypes type, int allowed_children);
void nim_end_widget(struct Tree *tree);
void nim_add_prop_text(struct Tree *tree, enum PropTypes type, const char *value);

int nim_get_prop(struct WidgetHeader *h, struct NimProp *np, int type);

typedef int nim_on_create_widget(void *priv, struct WidgetHeader *w);
typedef int nim_on_free_widget(void *priv, struct WidgetHeader *w);
typedef int nim_on_tweak_widget(void *priv, struct WidgetHeader *w);
typedef int nim_on_append_widget(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent);

struct NimBackend {
	void *priv;
	struct WidgetHeader *header;
	struct Tree tree_old;
	struct Tree tree_new;
	int of;

	nim_on_create_widget *create;
	nim_on_free_widget *free;
	nim_on_tweak_widget *tweak;
	nim_on_append_widget *append;
};

int nim_init_tree_widgets(struct NimBackend *backend, struct Tree *tree, int base, struct WidgetHeader *parent, int depth);

void nim_init_backend(struct NimBackend *backend);

void nim_setup_tree(struct Tree *tree);

