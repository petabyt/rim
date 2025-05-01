#include <stdio.h>
#include <rim.h>
#include <im.h>
#include <pthread.h>
#include <unistd.h>

int counter = 0;

void *ext(void *arg) {
	while (1) {
		if (counter >= 100) break;
		usleep(100000);
		counter++;
		rim_trigger_event();
	}
	return NULL;
}

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 1;

	pthread_t thread;
	pthread_create(&thread, 0, ext, NULL);
	
	while (rim_poll(ctx)) {
		if (im_begin_window("My Window", 500, 500)) {
			im_progress_bar(counter);
			if (counter == 100) {
				im_label("Done!");
			} else if (counter > 90) {
				im_label("Almost done..");
			} else if (counter > 50) {
				im_label("Halfway there..");
			} else if (counter > 10) {
				im_label("Keep waiting...");
			} else {
				im_label("Loading...");
			}
			im_end_window();
		}
	}

	return 0;
}
