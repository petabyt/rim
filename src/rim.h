#ifndef NAITVE_IMGUI_H
#define NAITVE_IMGUI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <im.h>

typedef struct RimContext rim_ctx_t;

/// @brief Initialize the context structure
/// @note For testing use only
rim_ctx_t *rim_init(void);

// @brief Close down the context structure
/// @note For testing use only
void rim_close(rim_ctx_t *ctx);

/// @brief Start a ui state thread and backend. Blocks until application is finished.
/// Once returned, everything is automatically freed and torn down.
int rim_start(int (*func)(rim_ctx_t *, void *), void *arg);

/// @brief Poll the UI for events such as button clicks and inputs,
/// as well as external events triggered from another thread.
/// @returns 1, once an event is triggered. If 0, then the application has closed down.
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

// Hack to relocate ui state thread onto a second thread for programs that want to call rim_init in main()
#ifdef REPLACE_MAIN
struct MyArgStruct {
	int argc;
	char **argv;
};

int rim_main(int argc, char **argv);

int __mymain(struct RimContext *ctx, void *arg) {
	struct MyArgStruct *my = (struct MyArgStruct *)arg;
	return rim_main(my->argc, my->argv);
}

int main(int argc, char **argv) {
	struct MyArgStruct my = {
		.argc = argc,
		.argv = argv,
	};
    return rim_start(__mymain, (void *)&my);
}
// Force main() to always take arguments
#define main(__ARGV__) rim_main(int argc, char **argv)
#endif

#ifdef __cplusplus
}
#endif

#endif
