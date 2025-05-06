#include <rim.h>
#include <im.h>
int main(void) {
    struct RimContext *ctx = rim_init();
    int show_more = 0;
    while (rim_poll(ctx)) {
        if (im_begin_window("My Window", 500, 500)) {
            if (im_button("Button")) show_more = !show_more;
            if (show_more) im_label("Hello World");
            im_end_window();
        }
    }
    return 0;
}
