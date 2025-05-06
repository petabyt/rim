#ifndef NAITVE_IMGUI_H
#define NAITVE_IMGUI_H

typedef struct RimContext rim_ctx_t;

/// @brief Try and get the correct DPI value for the system, ideally what has been set by the user
int rim_get_dpi(void);

#define rim_dp_to_px(dp) ((rim_get_dpi() * (dp)) / 160)

/// @brief Initialize the context structure
/// After this, the UI backend must be initialized.
rim_ctx_t *rim_init(void);

// TODO
//void rim_close(rim_ctx_t *ctx);

/// @brief Poll the UI for events such as button clicks and inputs,
/// as well as external events triggered from another thread.
/// @returns Returns 1 when there is an event.
/// @note When 0 is returned, the backend and context will already be closed down.
int rim_poll(rim_ctx_t *ctx);

/// @brief Can be called from any other thread to trigger an update to the UI.
/// Caller will have to ensure thread safety between any data shared between threads.
void rim_trigger_event(void);

/// @brief Saves the state of the current tree to a backup
/// This is useful in case you anticipate the tree being screwed up, such as running untrusted code.
/// This may cause problems with events.
void rim_tree_save_state(void);

/// @brief Restores the tree that was saved by rim_tree_save_state
void rim_tree_restore_state(void);

#endif
