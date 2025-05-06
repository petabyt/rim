#ifndef RIM_INTERNAL_H
#define RIM_INTERNAL_H

#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

// TODO: Add parent_of?
struct __attribute__((packed)) WidgetHeader {
	// Internal 32 bit widget type
	// If UI_CUSTOM, then a custom handler will be called
	uint32_t type;
	// Number of allowed children in this widget
	// If 0xffffffff, assume unlimited children
	// TODO: Remove this
	uint32_t allowed_children;
	// Number of children following this header
	uint32_t n_children;
	// Number of properties following this header
	uint32_t n_props;
	// Unique ID (only modified by UI backend and tree patcher)
	uint32_t unique_id;
	// This is set to 1 by the tree differ if this node is detached from its parent.
	// In that case this widget must be skipped when calculating the index of this node's siblings.
	uint8_t is_detached;
	// Set to 1 to tell the tree differ to throw away this widget even if the types match
	// This may be used in case the widget needs to be re-appended to it's parent.
	uint8_t invalidate;

	uint8_t res0;
	uint8_t res1;

	// Pointer handle for UI backend
	// This must be aligned by 8 bytes in the structure
	uintptr_t os_handle;
	// properties start here
	// children start here
	uint8_t data[];
};
_Static_assert(sizeof(struct WidgetHeader) == 32, "fail size");

// This is the common layout of all properties
// What is stored in `data` can be determined by the widget type.
// TODO: Rename to PropHeader?
struct __attribute__((packed)) WidgetProp {
	/// @brief Length in bytes of this property structure and data that follows
	uint32_t length;
	/// @brief See enum RimPropType
	uint32_t type;
	// If 1, then this property has already been applied to the backend or doesn't need to be applied.
	// This may be used in a weird case where the backend initializes a property before the tree differ gets to it.
	uint32_t already_fufilled;
	uint32_t res0;
	uint8_t data[];
};

_Static_assert(sizeof(struct WidgetProp) == 16, "fail size");

struct __attribute__((packed)) RimPropData {
	uint32_t length;
	uint32_t res0;
	uintptr_t ptr;
};
_Static_assert(sizeof(struct RimPropData) == 16, "fail size");

/// @brief Tree ID for all Rim widgets
/// 1-0xfff is reserved for Rim
/// >=0x1000 is reserved for custom widgets
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
};

/// 1-0xfff is reserved for Rim
/// >=0x1000 is reserved for custom widgets
enum RimPropType {
	RIM_PROP_NONE = 0,
	// string window title
	// TODO: Rename to RIM_PROP_TITLE
	RIM_PROP_WIN_TITLE,
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
	RIM_PROP_INNER_PADDING,
	// Set to 1 to disable widget
	RIM_PROP_DISABLED,
	// String tooltip on hover
	RIM_PROP_TOOLTIP,

	RIM_PROP_SLIDER_VALUE,
	RIM_PROP_SLIDER_MIN,
	RIM_PROP_SLIDER_MAX,

	/// @brief Valid values are 0-100
	RIM_PROP_PROGRESS_BAR_VALUE,

	// Combo box current selected value/child index
	RIM_PROP_COMBOBOX_SELECTED,

	// Set to 1 to make entry read-only
	RIM_PROP_ENTRY_READ_ONLY,

	// Secondary internal ID for extensions to use
	RIM_PROP_SECONDARY_ID,

	// Etc property to ignore
	RIM_PROP_META,
};

/// @brief im_ API will return widget events rather than 1/0 or bool.
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
#define TREE_MAX_DEPTH 15
	struct WidgetHeader *widget_stack[TREE_MAX_DEPTH];
	int widget_stack_depth;
	uint8_t *buffer;
	unsigned int buffer_length;
	int of;
};

struct RimEvent {
	// If 1, then this event hasn't been consumed yet.
	int is_valid;
	// Type of action that triggered this event
	enum RimWidgetEvent type;
	// Unique ID of the widget this event corrosponds to, if any
	int unique_id;
	// What property in the widget this event changes, if any. If none, then RIM_PROP_NONE.
	int affected_property;

	void *data;
	unsigned int data_buf_size;
	unsigned int data_length;
};

typedef void rim_on_run_callback(void *priv);

/// @brief Structure holding handlers for an extension widget(s) on top of the backend
struct RimExtension {
	void *priv;

	/// @brief Unique extension ID for functions that need to get the priv pointer
	/// that aren't the callbacks
	int ext_id;

	// TODO: it's not clear which of these callbacks will be needed for an extension...
	// TODO: Should the backend use these callbacks?

	/// @brief Create a backend widget given the widget header
	int (*create)(void *priv, struct WidgetHeader *w);
	/// @brief Change a property of a backend widget
	int (*tweak)(void *priv, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type);
	/// @brief Append a backend widget to a parent backend widget.
	int (*append)(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent);
	/// @brief Remove a widget from a parent
	int (*remove)(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent);
	/// @brief Free the widget from memory
	int (*destroy)(void *priv, struct WidgetHeader *w);
};

struct RimContext {
	struct RimTree tree_saved;
	// TODO: Inline these instead of ptr
	struct RimTree *tree_old;
	struct RimTree *tree_new;

#define RIM_MAX_EXTS 10
	struct RimExtension exts[RIM_MAX_EXTS];
	int n_exts;

	// Backend context pointer
	void *priv;

	// If 1, context will shut down on next cycle
	int quit_immediately;
	// Used to signal when rim_backend_run is finished
	sem_t run_done_signal;
	// Mutex protecting all the event members of this struct
	pthread_mutex_t event_mutex;
	// Only one event is processed at a time
	struct RimEvent last_event;
	// Used by rim_poll for how many times to cycle without waiting for events
	int nop_event_counter;
	// main event signal
	sem_t event_signal;
	// Signal to thread waiting with an event that the last event was consumed
	sem_t event_consumed_signal;
	// Used to ID events, nop cycles (nop_event_counter) not counted
	int current_event_id;
};

/// @brief Add an extension to the current context
void rim_add_extension(struct RimContext *ctx, struct RimExtension *ext);

void *rim_get_ext_priv(struct RimContext *ctx, int id);

/// @defgroup Backend implementation functions
/// @addtogroup backend
/// @{
// Setup backend and priv context
int rim_backend_init(struct RimContext *ctx);
// Free everything and close down thread
void rim_backend_close(struct RimContext *ctx);
/// @brief Create a backend widget given the widget header
/// @note All of the widget's properties will be set with rim_backend_tweak after this is called
int rim_backend_create(struct RimContext *ctx, struct WidgetHeader *w);
/// @brief Update unique ID in backend widget so events can be correctly traced from it when it sends an event
int rim_backend_update_id(struct RimContext *ctx, struct WidgetHeader *w);
/// @brief Free the widget from memory
int rim_backend_destroy(struct RimContext *ctx, struct WidgetHeader *w);
/// @brief Remove a widget from a parent
int rim_backend_remove(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
/// @brief Append a backend widget to a parent backend widget.
int rim_backend_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
/// @brief Change a property of a backend widget
int rim_backend_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type);
/// @brief Queue a function to run in the UI backend thread
int rim_backend_run(struct RimContext *ctx, rim_on_run_callback *callback);
/// @}

/// @defgroup Extension/backend wrapper interface for
/// @addtogroup backend
/// @{
int rim_widget_create(struct RimContext *ctx, struct WidgetHeader *w);
int rim_widget_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type);
int rim_widget_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
int rim_widget_remove(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent);
int rim_widget_destroy(struct RimContext *ctx, struct WidgetHeader *w);
/// @}

/// @brief Create a widget tree
struct RimTree *rim_create_tree(void);

/// @brief Reset a tree to empty
void rim_reset_tree(struct RimTree *tree);

/// @brief Add widget to tree and make it the current widget
void rim_add_widget(struct RimTree *tree, enum RimWidgetType type, int allowed_children);
/// @brief End adding properties or children to the current widget and switch to it's parent
void rim_end_widget(struct RimTree *tree);
/// @brief Add a property with a string being the only payload
void rim_add_prop_string(struct RimTree *tree, enum RimPropType type, const char *value);

/// @brief Add uint32 prop
void rim_add_prop_u32(struct RimTree *tree, enum RimPropType type, uint32_t val);

/// @brief Mark prop as fufilled so tree differ doesn't fufill it again
int rim_mark_prop_fufilled(struct WidgetHeader *h, int type);

/// @note 'length' field will be adjusted to the correct alignment
void rim_add_prop_data(struct RimTree *tree, enum RimPropType type, void *val, unsigned int length);

struct WidgetProp *rim_get_prop(struct WidgetHeader *h, int type);

/// @brief Get string property
/// @param val Set to the pointer of a standard null terminated string
int rim_get_prop_string(struct WidgetHeader *h, int type, char **val);

/// @brief Get a u32 property by type for a widget
int rim_get_prop_u32(struct WidgetHeader *h, int type, uint32_t *val);

/// @brief Find the length of a node (so it can be skipped through)
unsigned int rim_get_node_length(struct WidgetHeader *w);

/// @brief Get the index of a child using its parent
/// This will skip dead nodes to get the accurate index in the backend widget
int rim_get_child_index(struct WidgetHeader *w, struct WidgetHeader *parent);

/// @returns NULL if out of bounds
struct WidgetHeader *rim_get_child(struct WidgetHeader *w, int index);

/// @brief Find a node in a tree via it's unique id
/// @param of Set to the offset of where the widget is
int rim_find_in_tree(struct RimTree *tree, unsigned int *of, uint32_t unique_id);

/// @brief Run down current widget in `tree` offset `base` and call backend->create for all of it's widgets.
/// @brief The start widget and all of it's sublings will be appended to `parent`.
/// @depth Optional, for debugging
unsigned int rim_init_tree_widgets(struct RimContext *ctx, struct RimTree *tree, unsigned int base, struct WidgetHeader *parent);

// Get event code for last created event
int rim_last_widget_event(int lookback);

int rim_last_widget_detach(int lookback);

/// @brief backend calls this when a widget has an event
void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id);

/// @brief Variant of rim_on_widget_event that writes data to the event buffer
void rim_on_widget_event_data(struct RimContext *ctx, enum RimWidgetEvent event, enum RimPropType type, int unique_id, const void *buffer, unsigned int length);

/// @brief Run the differ using old and new tree and fixup the widget tree
int rim_diff_tree(struct RimContext *ctx);

// debugging only
const char *rim_eval_widget_type(uint32_t type);

// debugging only
__attribute__((noreturn))
void rim_abort(char *fmt, ...);

/// @brief Returns a pointer to the tree to currently append to.
/// Will never return NULL.
struct RimTree *rim_get_current_tree(void);

/// @brief To be used sparingly, hopefully not permanently
struct RimContext *rim_get_global_ctx(void);

static void check_prop(int c) {
	if (c) {
		rim_abort("Getting prop failed\n");
	}
}

#endif
