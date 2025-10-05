#ifndef RIM_INTERNAL_H
#define RIM_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#define _Static_assert static_assert
#endif

#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

// Note: All structures are 8-byte aligned

/// @note A pointer to this structure may not be kept during runtime as
/// the tree buffer can be reallocated by the tree builder
struct __attribute__((packed)) WidgetHeader {
	// Internal 32 bit widget type
	uint32_t type;
	// Offset of this widget's parent. If 0xffffffff, then this is a top-level widget.
	uint32_t parent_of;
	// Number of children following this header
	uint32_t n_children;
	/// @brief Number of properties following this header
	uint32_t n_props;
	/// @brief Unique ID (only modified by UI backend and tree patcher)
	uint32_t unique_id;
	/// @brief If 1, then this node has been detached from its parent.
	/// This can be set by the tree differ to keep track of which widgets have been removed from their parent.
	/// In that case this widget must be skipped when calculating the index of this node's siblings.
	/// It may also be set by the tree builder to signal that a widget has destroyed itself (such as a window closing)
	/// and it shouldn't try and destroy it.
	uint8_t is_detached;
	/// @brief Set to 1 to tell the tree differ to throw away this widget even if the types match.
	/// This is needed to make inserting widgets work, because the backend doesn't have an 'insert' method yet.
	uint8_t invalidate;

	uint8_t res0;
	uint8_t res1;

	/// @brief Pointer handle for UI backend
	uintptr_t os_handle;
	uint8_t data[];
	// What follows are 'n_props' properties,
	// and 'n_children' child nodes
};
_Static_assert(sizeof(struct WidgetHeader) == 32, "fail size");

/// @brief This is the common layout of all properties
/// @brief What is stored in `data` can be determined by the property type.
struct __attribute__((packed)) PropHeader {
	/// @brief Length in bytes of this property structure and data that follows
	uint32_t length;
	/// @brief See enum RimPropType
	uint32_t type;
	// If 1, then this property has already been applied to the backend or doesn't need to be applied.
	// This may be used in a weird case where the backend initializes a property before the tree differ gets to it.
	// It's also used to prevent SetText being called on a widget on every keystroke. This messes with the cursor in some toolkits.
	uint8_t already_fulfilled;
	// If 1, then this property will be set once the widget it belongs to has all its children set up.
	uint8_t res0;
	uint8_t res1;
	uint8_t res2;
	/// @brief Last Event ID that caused this property to change
	uint32_t last_changed_by;
	uint8_t data[];
};
_Static_assert(sizeof(struct PropHeader) == 16, "fail size");

struct __attribute__((packed)) RimPropData {
	uint32_t length;
	uint32_t res0;
	uintptr_t ptr;
};
_Static_assert(sizeof(struct RimPropData) == 16, "fail size");

// TODO: Cast PropHeader.data to this
union RimPropUnion {
	uint32_t u32;
	uint64_t u64;
	struct RimPropData data;
};

/// @brief Tree ID for all Rim widgets
/// 1-0xfff is reserved for Rim
/// >=0x1000 is reserved for custom widgets
static inline int is_rim_widget(unsigned int type) {
	return (type >= 0) && (type <= 0xfff);
}
enum RimWidgetType {
	RIM_NONE = 0,
	// Full-size window
	RIM_WINDOW = 1,
	// Smaller popup - might have an 'ok' or 'cancel' button
	RIM_POPUP,
	// A label with plain text
	RIM_LABEL,
	// A button with plain text
	RIM_BUTTON,
	// Horizontal progress bar
	RIM_PROGRESS_BAR,
	// Static image widget
	RIM_IMAGE,
	// Single-line text box that can be modified by user
	RIM_ENTRY,
	// Multiline text box entry
	RIM_MULTILINE_ENTRY,
	// Entry that hides text
	RIM_PASSWORD_ENTRY,
	// Entry that is styled like a search box
	RIM_SEARCH_ENTRY,
	// Box with buttons to decrement/increment, usually for a number
	RIM_SPINBOX,
	// Horizontal slider
	RIM_SLIDER,
	// Also known as a dropdown box
	RIM_COMBOBOX,
	// Item in the combo box with a text value
	RIM_COMBOBOX_ITEM,
	// Multiple choice single answer element
	RIM_RADIO,
	// Selectable item in a radio box
	RIM_RADIO_ITEM,
	// Date picker, may open a window if clicked
	RIM_DATE_PICKER,
	// A container where tabs can be added
	RIM_TAB_BAR,
	// A button in a tab bar which when clicked will show its contents
	RIM_TAB,
	// Linear horizontal box
	RIM_HORIZONTAL_BOX,
	// Linear vertical box
	RIM_VERTICAL_BOX,
	// TODO:
	RIM_FLEX_BOX,
	// A container for menus
	RIM_WINDOW_MENU_BAR,
	// A menu bar at the top of a window
	RIM_WINDOW_MENU,
	// A menu bar item
	RIM_WINDOW_MENU_ITEM,
	// A fast scrollable table
	RIM_TABLE,
	// A form container that has neatly arranged form entries
	// https://doc.qt.io/qt-6/qformlayout.html
	RIM_FORM,
	// A form entry with text and a child widget, normally a single input widget.
	RIM_FORM_ENTRY,
};

/// 1-0xfff is reserved for Rim
/// >=0x1000 is reserved for custom widgets
enum RimPropType {
	RIM_PROP_NONE = 0,
	// Generic title property
	RIM_PROP_TITLE,
	// string path to window icon
	RIM_PROP_WIN_ICON_PATH,
	// ICO (Microsoft icon) data
	RIM_PROP_WIN_ICON_DATA,
	// Generic 'dp' width
	RIM_PROP_WIDTH_DP,
	// Generic 'dp' height
	RIM_PROP_HEIGHT_DP,
	// Generic primary text for a widget
	RIM_PROP_TEXT,
	// Generic secondary text usually placed to the left of the widget
	RIM_PROP_LABEL,
	// Whether to expand horizontally & vertically
	RIM_PROP_EXPAND,
	// Padding inside a container in dp
	RIM_PROP_MARGIN,
	// Gap in dp between children
	RIM_PROP_GAP,
	// Set to 1 to disable widget
	RIM_PROP_DISABLED,
	// Text tooltip that shows on hover
	RIM_PROP_TOOLTIP,
	// Integer value of an input widget
	RIM_PROP_NUMBER_VALUE,
	// Min integer value of an input widget
	RIM_PROP_NUMBER_MIN,
	// Max integer value of an input widget
	RIM_PROP_NUMBER_MAX,
	// Bool
	RIM_PROP_ENTRY_READ_ONLY,
	// Secondary internal ID for extensions to use
	RIM_PROP_SECONDARY_ID,
	// Etc property to ignore
	RIM_PROP_META,
};

/// @brief API event codes. im_ functions will return these events rather than 1/0 or bool.
enum RimWidgetEvent {
	RIM_EVENT_NONE = 0,
	RIM_EVENT_CLICK = 1,
	RIM_EVENT_WINDOW_CLOSE = 2,
	RIM_EVENT_VALUE_CHANGED = 3,
};

/// @brief Backend needs to know whether a property has been added, changed, or removed.
enum RimPropTrigger {
	RIM_PROP_CHANGED,
	RIM_PROP_ADDED,
	RIM_PROP_REMOVED,
};

/// @brief State of a tree for the tree builder
struct RimTree {
	/// @brief Used to assign IDs to widgets in the tree
	int counter;
	/// @brief Number of children at root in tree
	int n_root_children;
#define TREE_MAX_DEPTH 100
	/// @brief List of widgets in offsets from tree pointer
	unsigned int widget_stack[TREE_MAX_DEPTH];
	int widget_stack_depth;
	uint8_t *buffer;
	unsigned int buffer_length;
	unsigned int of;
};

struct RimEvent {
	// If 1, then this event hasn't been consumed yet.
	int is_valid;
	// Type of action that triggered this event
	enum RimWidgetEvent type;
	// Unique ID of the widget this event corresponds to, if any
	int unique_id;
	// What property in the widget this event changes, if any. If none, then RIM_PROP_NONE.
	int affected_property;

	void *data;
	unsigned int data_buf_size;
	unsigned int data_length;
};

typedef void rim_on_run_callback(void *priv);

enum RimPropFlags {
	// Force this property to only be set after children have been appended to the node
	// and all other proeprties that don't have this flag have been set.
	RIM_FLAG_SET_PROP_AFTER_CHILDREN = (1 << 0),
};
enum RimWidgetFlags {
	// Force children of this node to be initialized before initializing this node itself
	RIM_FLAG_INIT_CHILDREN_FIRST = (1 << 0),
};

/// @brief Structure holding handlers for an extension (or backend).
/// This is a basic abstraction layer over a retained-mode toolkit, giving the tree differ
/// what it needs to maintain state.
struct RimExtension {
	void *priv;

	/// @brief Unique extension ID for functions that need to get the priv pointer
	/// that aren't the callbacks
	int ext_id;

	/// @brief Create a backend widget given the widget header
	int (*create)(void *priv, struct WidgetHeader *w);
	/// @brief Change a property of a backend widget
	int (*tweak)(void *priv, struct WidgetHeader *w, struct PropHeader *prop, enum RimPropTrigger type);
	/// @brief Append a backend widget to a parent backend widget.
	int (*append)(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent);
	/// @brief Remove a widget from a parent
	int (*remove)(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent);
	/// @brief Free the widget from memory
	int (*destroy)(void *priv, struct WidgetHeader *w);
	/// @brief Update the onclick handler to use the widgets new unique ID
	int (*update_onclick)(void *priv, struct WidgetHeader *w);
	/// @brief Close down any context and handle freeing data in 'priv'
	void (*close)(void *priv);

	/// @brief Get rules for how to treat a property
	/// @see enum RimPropFlags
	int (*get_prop_rules)(void *priv, const struct WidgetHeader *w, const struct PropHeader *p);
	/// @brief Get rules for how to treat/initialize a widget
	/// @see enum RimWidgetFlags
	int (*get_widget_rules)(void *priv, const struct WidgetHeader *w, const struct WidgetHeader *parent);
};

struct RimContext {
	struct RimTree tree_saved;
	// TODO: Maybe inline these instead of ptr
	struct RimTree *tree_old;
	struct RimTree *tree_new;

	struct RimExtension backend;

	struct RimExtension exts[20];
	int n_exts;

	// Handle for the second thread. This may be the backend or ui state thread.
	pthread_t second_thread;

	// If 1, context will shut down on next cycle
	int quit_immediately;
	/// @brief Used to signal when the backend thread is finished doing something
	sem_t *backend_done_signal;
	/// @brief Mutex protecting all the event members of this struct
	pthread_mutex_t event_mutex;
	/// @brief Only one event is processed at a time
	struct RimEvent last_event;
	// Used by rim_poll for how many times to cycle without waiting for events
	int nop_event_counter;
	// main event signal
	sem_t *event_signal;
	// Signal to thread waiting with an event that the last event was consumed
	sem_t *event_consumed_signal;
	// Used to ID events, nop cycles (nop_event_counter) not counted
	int current_event_id;
};

/// @brief Add an extension to the current context
void rim_add_extension(struct RimContext *ctx, struct RimExtension *ext);

/// @brief Get extension priv pointer from extension ID
/// @returns NULL if not found
void *rim_get_ext_priv(struct RimContext *ctx, int id);

/// @brief Initialize the backend and start its thread
void rim_backend_start(struct RimContext *ctx, sem_t *done);
/// @brief Run some code on the backend UI thread
int rim_backend_run(struct RimContext *ctx, rim_on_run_callback *callback, void *arg);

/// @defgroup Backend/extension wrapper interface for widgets
/// @addtogroup backend
/// @{
int rim_widget_create(struct RimContext *ctx, struct WidgetHeader *w);
int rim_widget_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct PropHeader *prop, enum RimPropTrigger type);
int rim_widget_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
int rim_widget_remove(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
int rim_widget_destroy(struct RimContext *ctx, struct WidgetHeader *w);
int rim_widget_update_onclick(struct RimContext *ctx, struct WidgetHeader *w);
int rim_widget_get_rules(struct RimContext *ctx, const struct WidgetHeader *w, const struct WidgetHeader *parent);
int rim_prop_get_rules(struct RimContext *ctx, const struct WidgetHeader *w, const struct PropHeader *p);
/// @}

/// @brief Create a widget tree
struct RimTree *rim_create_tree(void);

/// @brief Reset a tree to be empty
void rim_reset_tree(struct RimTree *tree);

/// @brief Add widget to tree and make it the current widget
void rim_add_widget(struct RimTree *tree, enum RimWidgetType type);
/// @brief End adding properties or children to the current widget and switch to its parent
void rim_end_widget(struct RimTree *tree, uint32_t type);
/// @brief Add a property with a string being the only payload
void rim_add_prop_string(struct RimTree *tree, enum RimPropType type, const char *value);

/// @brief Add uint32 prop
void rim_add_prop_u32(struct RimTree *tree, enum RimPropType type, uint32_t val);

/// @brief add uint64 prop
void rim_add_prop_u64(struct RimTree *tree, enum RimPropType type, uint64_t val);

/// @brief Set 'already_fulfilled' setting in a property
int rim_mark_prop_fulfilled(struct WidgetHeader *h, int type);

/// @note 'length' field will be adjusted to the correct alignment
void rim_add_prop_data(struct RimTree *tree, enum RimPropType type, void *val, unsigned int length);

/// @brief Find property in widget by type
struct PropHeader *rim_get_prop(struct WidgetHeader *h, int type);

/// @brief Get string property
/// @param val Set to the pointer of a standard null terminated string
int rim_get_prop_string(struct WidgetHeader *h, int type, char **val);

/// @brief Get a u32 property by type for a widget
int rim_get_prop_u32(struct WidgetHeader *h, int type, uint32_t *val);

/// @brief Get a u32 property by type for a widget
int rim_get_prop_u64(struct WidgetHeader *h, int type, uint64_t *val);

/// @brief Find the length of a node (so it can be skipped through)
unsigned int rim_get_node_length(struct WidgetHeader *w);

/// @brief Get the index of a child using its parent
/// This will skip dead nodes to get the accurate index in the backend widget
int rim_get_child_index(struct WidgetHeader *w, struct WidgetHeader *parent);

/// @returns NULL if out of bounds
struct WidgetHeader *rim_get_child(struct WidgetHeader *w, int index);

/// @brief Find a node in a tree via its unique id
/// @param of Set to the offset of where the widget is
/// @returns 1 if found
int rim_find_in_tree(struct RimTree *tree, unsigned int *of, uint32_t unique_id);

/// @brief Run down current widget in `tree` offset `base` and call backend->create for all of its widgets.
/// @brief The start widget and all of its sublings will be appended to `parent`.
/// @depth Optional, for debugging
unsigned int rim_init_tree_widgets(struct RimContext *ctx, struct RimTree *tree, unsigned int base, struct WidgetHeader *parent);

/// @brief Get event code for last widget in the tree
int rim_last_widget_event(int lookback);

/// @brief Mark the last widget in tree as detached from parent.
/// This is only useful for widgets that detach themselves
int rim_last_widget_detach(int lookback);

/// @brief Called by code during the tree building process to check if a widget has an event associated with it
void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id);

/// @brief Variant of rim_on_widget_event that writes data to the event buffer
void rim_on_widget_event_data(struct RimContext *ctx, enum RimWidgetEvent event, enum RimPropType type, int unique_id, const void *buffer, unsigned int length);

/// @brief Run the tree differ using old and new tree
int rim_diff_tree(struct RimContext *ctx);

/// @brief Init a new tree (to get the differ cycle started)
int rim_init_tree(struct RimContext *ctx);

// debugging only
void rim_dump_tree(struct RimTree *tree);
const char *rim_eval_widget_type(uint32_t type);
const char *rim_eval_prop_type(uint32_t type);
__attribute__((noreturn))
void rim_abort(const char *fmt, ...);

/// @brief Returns the tree that is currently being built up.
/// Will never return NULL.
/// @note Should only be used during tree building
struct RimTree *rim_get_current_tree(void);

/// @brief Returns the old tree (what the UI currently looks like)
/// @note Should only be used during tree building
struct RimTree *rim_get_old_tree(void);

/// @brief Get the Rim context for this process
struct RimContext *rim_get_global_ctx(void);

static void check_prop(int c) {
	if (c) {
		rim_abort("Getting prop failed\n");
	}
}

#ifdef __cplusplus
}
#endif

#endif
