#ifndef NAITVE_IMGUI_H
#define NAITVE_IMGUI_H

int rim_get_dpi(void);

#define rim_dp_to_px(dp) ((rim_get_dpi() * (dp)) / 160)

/// @brief Internal structure that maintains the state of the UI
typedef struct RimContext rim_ctx_t;

/// @brief Initialize the context structure
/// After this, the UI backend must be initialized.
rim_ctx_t *rim_init(void);

/// @brief Poll the UI for events such as button clicks and inputs,
/// as well as external events triggered from another thread.
int rim_poll(rim_ctx_t *ctx);

/// @brief Can be called from any other thread to trigger an update to the UI.
/// Caller will have to ensure thread safety between any data shared between threads.
void rim_trigger_event(void);

/// @brief Initializes the libui virtual dom
int rim_libui_init(rim_ctx_t *ctx);

/// @brief Creates a new instance
rim_ctx_t *rim_imgui_init(void);

#endif
