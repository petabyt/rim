#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <pthread.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "rim.h"
#include "rim_internal.h"

static struct Data {
	sem_t *ready;
	pthread_t thread;
	int (*main_func)(void);
	int (*main_arg_func)(int, char **);
	int argv;
	void **argc;
	int rc;
}global_data;

static void *backend_thread(void *arg) {
	return NULL;
}

int rim_backend_init(struct RimContext *ctx) {
	sem_post(global_data.ready);
	return 0;
}

int rim_switch_main(int (*main_func)(void), int (*main_arg_func)(int, char **), int argc, char **argv) {
	if (pthread_create(&global_data.thread, NULL, backend_thread, NULL) != 0) {
		perror("pthread_create() error");
		return 1;
	}

	sem_unlink("wait_until_ready");
	global_data.ready = sem_open("wait_until_ready", O_CREAT | O_EXCL, 0, 0);
	if (global_data.ready == SEM_FAILED) {
		rim_abort("sem_open errno: %d\n", errno);
	}

	sem_wait(global_data.ready);

	rim_backend_thread(rim_get_global_ctx(), NULL);

	return 0;
}
