Due to the nature of the `rim_poll` function, Rim is strictly designed to be multi-threaded.
This is done for two reasons:
- Because most UI toolkits don't have a 'step' function, only a single 'main' function that blocks until the program finishes.
- Work is never done on the UI thread. If the UI thread needs to spend a few seconds doing something, the UI renderer doesn't freeze.

So the Rim runtime has two threads:
1. **UI state thread** which will be running the `while (rim_poll(ctx)) { ... }` loop
2. **Backend thread**, which will call `uiMain`/`wxEntry`/etc
  - Those calls will block, but code can still be run on this thread with functions like `uiQueueMain`

Sadly since some UI toolkits (looking at you Cocoa) have problems with running on a second thread, rim_start is needed
to run the `rim_poll` loop on a second thread.

```
#include <rim.h>
#include <im.h>

int rim_main(rim_ctx_t *ctx, void *arg) {
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

int main(int argc, char **argv) {
	return rim_start(rim_main, null);
}
```
