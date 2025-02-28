#ifndef NIM_INTERNAL_H
#define NIM_INTERNAL_H

#include <stdint.h>
#include <semaphore.h>

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
_Static_assert(sizeof(struct WidgetHeader) == 28);

// This is the common layout of all properties
// What is stored in `data` can be determined by the widget type.
struct __attribute__((packed)) WidgetProp {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};
_Static_assert(sizeof(struct WidgetProp) == 8);

enum NimPropType {
	NIM_PROP_WIN_TITLE = 1,
	NIM_PROP_WIN_WIDTH = 2,
	NIM_PROP_WIN_HEIGHT = 3,
	NIM_PROP_TEXT,
};

enum NimWidgetType {
	NIM_WINDOW = 1,
	NIM_LABEL,
	NIM_BUTTON,
	NIM_PROGRESS_BAR,
	NIM_IMAGE,
	NIM_ENTRY,
	NIM_SPINBOX,
	NIM_SLIDER,
	NIM_COMBOBOX,
	NIM_RADIO,
	NIM_DATE_PICKER,
	NIM_TABLE,
	NIM_LAYOUT_STATIC,
	NIM_LAYOUT_DYNAMIC,
	NIM_LAYOUT_FLEX,
	NIM_CUSTOM,
	NIM_NATIVE,
	NIM_EOF,
};

enum NimWidgetEvent {
	NIM_EVENT_NONE = 0,
	NIM_EVENT_CLICK_DOWN = 1,
	NIM_EVENT_CLICK_UP = 2,
};

struct NimTree {
	struct WidgetHeader *widget_stack[5];
	int widget_stack_depth;
	uint8_t *buffer;
	int buffer_length;
	int of;
};

struct NimEvent {
	// Used to trace where the widget is in the tree
	// So {1, 2, 3} would mean:
	// - select child 1 of node 0
	// - select child 2 of selected node
	// - select child 3 of selected node
	int branch[5];
	int widget_type;
	int event_type;
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

struct NimContext {
	struct WidgetHeader *header;
	struct NimTree *tree_old;
	struct NimTree *tree_new;
	int of;

	// Backend context pointer
	void *priv;

	// Virtual DOM handlers
	nim_on_create_widget *create;
	nim_on_free_widget *free;
	nim_on_tweak_widget *tweak;
	nim_on_append_widget *append;

	// Set to 1 after first frame (for nim_poll)
	int first_frame;

	sem_t event_sig;
	struct NimEvent last_event;
};

/// @brief Create a widget tree
struct NimTree *nim_create_tree(void);

/// @brief Setup fields of NimBackend
void nim_init_backend(struct NimContext *backend);

/// @brief Add widget to tree and make it the current widget
void nim_add_widget(struct NimTree *tree, enum NimWidgetType type, int allowed_children);
/// @brief End adding properties or children to the current widget and switch to it's parent
void nim_end_widget(struct NimTree *tree);
/// @brief Add a property with a string being the only payload
void nim_add_prop_text(struct NimTree *tree, enum NimPropType type, const char *value);

/// @brief Find property in widget from PropType
int nim_get_prop(struct WidgetHeader *h, struct NimProp *np, int type);

/// @brief Run down current widget in `tree` offset `base` and call backend->create for all of it's widgets.
/// @brief The start widget and all of it's sublings will be appended to `parent`.
/// @depth Optional, for debugging
int nim_init_tree_widgets(struct NimContext *ctx, struct NimTree *tree, int base, struct WidgetHeader *parent, int depth);

/// @brief Returns a pointer to the tree to currently append to.
/// Will never return NULL.
struct NimTree *nim_get_current_tree(void);

// Get event code for last created event
int nim_last_widget_event(void);

const char *nim_eval_widget_type(int type);

int nim_abort(const char *reason);

// Demo UIs
int nim_demo_window1(int state);
int nim_demo_window2(int state);

#endif
