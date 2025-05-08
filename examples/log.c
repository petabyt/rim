#include <stdio.h>
#include <string.h>
#define REPLACE_MAIN
#include <rim.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	char buffer[256] = "...\n";
	while (rim_poll(ctx)) {
		if (im_begin_window("Log Demo", 500, 500)) {
			if (im_button("Click to add stuff"))
				strncat(buffer, "Hello World\n", sizeof(buffer) - 1);
			im_set_next_expand();
			im_multiline_entry(buffer, sizeof(buffer));
			if (im_button("Print contents"))
				printf("%s\n", buffer);
			im_end_window();
		}
	}

	return 0;
}
