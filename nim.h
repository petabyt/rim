#ifndef NAITVE_IMGUI_H
#define NAITVE_IMGUI_H
#include <stdint.h>

struct __attribute__((packed)) WidgetHeader {
	// Internal 32 bit widget type
	// If UI_CUSTOM, then a custom handler will be called
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
	// Note: may not be aligned by 8 bytes. May need to do two 32 bit loads.
	uintptr_t os_handle;
	uint8_t data[];
	// properties start here
	// children start here
};

// This is the common layout of all properties
// What is stored in `data` can be determined by the widget type.
struct __attribute__((packed)) WidgetProp {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};

enum PropType {
	UI_PROP_WIN_TITLE = 1,
	UI_PROP_TEXT,
};

enum WidgetType {
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

/// @brief Generic data structure holding information on a property
struct NimProp {
	int type;
	const char *value;
};

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

/// @brief Setup fields of Tree
void nim_setup_tree(struct Tree *tree);

/// @brief Setup fields of NimBackend
void nim_init_backend(struct NimBackend *backend);

/// @brief Add widget to tree and make it the current widget
void nim_add_widget(struct Tree *tree, enum WidgetType type, int allowed_children);
// @brief End adding properties or children to the current widget and switch to it's parent
void nim_end_widget(struct Tree *tree);
/// @brief Add a property with a string being the only payload
void nim_add_prop_text(struct Tree *tree, enum PropType type, const char *value);

/// @brief Find property in widget from PropType
int nim_get_prop(struct WidgetHeader *h, struct NimProp *np, int type);

/// @brief Run down current widget in `tree` offset `base` and call backend->create for all of it's widgets.
/// @brief The start widget and all of it's sublings will be appended to `parent`.
/// @depth Optional, for debugging
int nim_init_tree_widgets(struct NimBackend *backend, struct Tree *tree, int base, struct WidgetHeader *parent, int depth);

// Demo UIs
int nim_demo_window1(struct Tree *tree, int state);
int nim_demo_window2(struct Tree *tree, int state);

#endif
