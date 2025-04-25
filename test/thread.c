#include <stdio.h>
#include <rim.h>
#include <im.h>
#include <pthread.h>
#include <unistd.h>

int counter = 0;

void *ext(void *arg) {
	while (1) {
		usleep(1000000);
		counter++;
		rim_trigger_event();
	}
	return NULL;
}

int main(void) {
	struct RimContext *ctx = rim_init();
	int show_more = 1;
	//int counter = 0;

	pthread_t thread;
	pthread_create(&thread, 0, ext, NULL);
	
	while (rim_poll(ctx)) {
		if (im_window("My Window", 500, 500)) {
			char buffer[64];
			sprintf(buffer, "Events: %04d\n", counter);
			im_label(buffer);
			if (show_more) {
				im_label("Hello, World");
			}
			if (im_button("Show More")) {
				show_more = !show_more;
			}
			im_end_window();
		}
	}

	return 0;
}
