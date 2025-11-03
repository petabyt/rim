Due to the nature of the `rim_poll` function, Rim is strictly designed to be multithreaded.
This is done for two reasons:
- Because most UI toolkits don't have a 'step' function, only a single 'main' function that blocks until the program finishes.
- Work is never done on the UI thread. If the UI thread needs to spend a few seconds doing something, the UI renderer doesn't freeze.
(Inputs would be blocked, but the button click animation would complete)

So the Rim runtime has two threads:
1. **UI state thread** which will be running the `while (rim_poll(ctx)) { ... }` loop
2. **Backend thread**, which will call `uiMain`/`wxEntry`/etc
  - (Those calls will block, but code can still be run on this thread with functions like `uiQueueMain`)

The UI state thread is responsible for:
- Building the UI tree into the current tree buffer
- Sending both trees to the tree differ for patching

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

In order for events to go from an onClick to being returned as an event value in functions like `im_button`, these two conditions must be guaranteed:

1. `rim_poll` must only process one event at a time
If more than one event were processed, then one event could completely change the layout and remove a widget that another event corrosponds to.

2. An event must come from a widget that existed in the old tree and will exist in the new tree in the same place.
This is more of a rule than a design issue. In Rim, you can't have the layout of the tree change between events without an event affecting it. Changing properties is fine though.

If these two conditions are guaranteed, then it's possible to match a widget between two layouts and route events to the correct `im_button` call.
