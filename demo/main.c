#include <stdio.h>
#include <rim.h>
#include <im.h>

int main(void) {
	struct RimContext *ctx = rim_init();
	rim_scintilla_init(ctx);
	while (rim_poll(ctx)) {
		if (im_begin_window("My Window", 500, 500)) {

			im_scintilla();

			im_end_window();
		}
	}

	return 0;
}
