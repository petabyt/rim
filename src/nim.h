#ifndef NAITVE_IMGUI_H
#define NAITVE_IMGUI_H

/// @brief Internal structure that maintains the state of the UI
typedef struct NimContext nim_ctx_t;

/// @brief Initialize the context structure
/// After this, the UI backend must be initialized.
nim_ctx_t *nim_init(void);

/// @brief Poll the UI for events such as button clicks and inputs,
/// as well as external events triggered from another thread.
int nim_poll(nim_ctx_t *ctx);

/// @brief Can be called from any other thread to trigger an update to the UI.
/// Caller will have to ensure thread safety between any data shared between threads.
int nim_update(void);

/// @brief Initializes the libui virtual dom
int nim_libui_init(nim_ctx_t *ctx);

/// @brief Creates a new instance
nim_ctx_t *nim_imgui_init(void);

enum NimBackend {
    NIM_BACKEND_LIBUI = 1,
    NIM_BACKEND_IMGUI = 2,
};
enum NimBackend nim_get_backend(nim_ctx_t *ctx);

#endif
