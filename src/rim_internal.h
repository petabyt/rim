#ifndef RIM_INTERNAL_H
#define RIM_INTERNAL_H

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

	uint32_t res0; // extra 4 bytes to ensure 8 alignment

	// Pointer handle for UI backend
	// Note: may not be aligned by 8 bytes. May need to do two 32 bit loads.
	uintptr_t os_handle;
	uint8_t data[];
	// properties start here
	// children start here
};
_Static_assert(sizeof(struct WidgetHeader) == 28 + 4, "fail size");

// This is the common layout of all properties
// What is stored in `data` can be determined by the widget type.
struct __attribute__((packed)) WidgetProp {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};
_Static_assert(sizeof(struct WidgetProp) == 8, "fail size");

enum RimPropType {
	RIM_PROP_WIN_TITLE = 1,
	RIM_PROP_WIN_WIDTH = 2,
	RIM_PROP_WIN_HEIGHT = 3,
	RIM_PROP_TEXT,
	RIM_PROP_META,
};

enum RimWidgetType {
	// 1-0xfff is reserved for Rim
	// >=0x1000 is reserved for custom widgets
	RIM_WINDOW = 1,
	RIM_LABEL,
	RIM_BUTTON,
	RIM_PROGRESS_BAR,
	RIM_IMAGE,
	RIM_ENTRY,
	RIM_SPINBOX,
	RIM_SLIDER,
	RIM_COMBOBOX,
	RIM_RADIO,
	RIM_DATE_PICKER,
	RIM_TABLE,
	RIM_LAYOUT_STATIC,
	RIM_LAYOUT_DYNAMIC,
	RIM_LAYOUT_FLEX,
	RIM_CUSTOM,
	RIM_NATIVE,
	RIM_EOF,
};

enum RimWidgetEvent {
	RIM_EVENT_NONE = 0,
	RIM_EVENT_CLICK = 1,
};

enum RimPropTrigger {
	RIM_PROP_CHANGED,
	RIM_PROP_ADDED,
	RIM_PROP_REMOVED,
}

struct RimTree {
#define TREE_MAX_DEPTH 5
	struct WidgetHeader *widget_stack[TREE_MAX_DEPTH];
	int widget_stack_depth;
	uint8_t *buffer;
	unsigned int buffer_length;
	int of;
};

struct RimEvent {
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
struct RimProp {
	int type;
	const char *value;
};

typedef void rim_on_run_callback(void *priv);

typedef int rim_on_create_widget(struct RimContext *ctx, struct WidgetHeader *w);
typedef int rim_on_tweak_widget(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type);
typedef int rim_on_append_widget(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
typedef int rim_on_destroy_widget(struct RimContext *ctx, struct WidgetHeader *w);
typedef int rim_on_run(struct RimContext *ctx, rim_on_run_callback *callback);

struct RimContext {
	struct WidgetHeader *header;
	struct RimTree *tree_old;
	struct RimTree *tree_new;

	// Backend context pointer
	void *priv;

	/// @brief Create a backend widget given the widget header
	rim_on_create_widget *create;
	/// @brief Change a property of a backend widget
	rim_on_tweak_widget *tweak;
	/// @brief Append a backend widget to a parent backend widget.
	/// @todo What should happen if a widget can't be appended to something?
	rim_on_append_widget *append;
	/// @brief Destroy the backend widget and all of its children
	rim_on_destroy_widget *destroy;
	/// @brief Queue a function to run in the UI backend thread
	rim_on_run *run;

	// Dummy event counter
	int event_counter;

	sem_t event_sig; // main event signal
	struct RimEvent last_event;
};

/// @brief Create a widget tree
struct RimTree *rim_create_tree(void);

/// @brief Reset a tree to empty
void rim_reset_tree(struct RimTree *tree);

/// @brief Setup fields of RimBackend
void rim_init_backend(struct RimContext *backend);

/// @brief Add widget to tree and make it the current widget
void rim_add_widget(struct RimTree *tree, enum RimWidgetType type, int allowed_children);
/// @brief End adding properties or children to the current widget and switch to it's parent
void rim_end_widget(struct RimTree *tree);
/// @brief Add a property with a string being the only payload
void rim_add_prop_text(struct RimTree *tree, enum RimPropType type, const char *value);

/// @brief Find property in widget from PropType
int rim_get_prop(struct WidgetHeader *h, struct RimProp *np, int type);

/// @brief Run down current widget in `tree` offset `base` and call backend->create for all of it's widgets.
/// @brief The start widget and all of it's sublings will be appended to `parent`.
/// @depth Optional, for debugging
int rim_init_tree_widgets(struct RimContext *ctx, struct RimTree *tree, int base, struct WidgetHeader *parent, int depth);

/// @brief Returns a pointer to the tree to currently append to.
/// Will never return NULL.
struct RimTree *rim_get_current_tree(void);

// Get event code for last created event
int rim_last_widget_event(void);

/// @brief backend calls this when a widget has an event
void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id);

// Run the differ using old and new tree
int rim_diff_tree(struct RimContext *ctx);

// debugging only
const char *rim_eval_widget_type(int type);
// debugging only
int rim_abort(const char *reason);

/// @brief To be used sparingly, hopefully not permanently
struct RimContext *rim_get_global_ctx(void);

// Demo UIs
int rim_demo_window1(int state);
int rim_demo_window2(int state);

#endif
