## Virtual DOM Technical Details

In order for this to work, we need two threads:
1. **UI State thread** which will be running the `while (rim_poll(ctx)) { ... }` UI loop (this be `main()`)
2. **Backend thread**, which will call `uiMain`/`wxEntry`/etc
  Those calls will block, but code can still be run on this thread with functions like `uiQueueMain`

### UI State Thread responsibilities
- `rim_poll` will be listening to events from the backend thread
- An input list from the backend must be processed in order to know which buttons have been clicked
- Build a tree through the frontend API
- Send the current tree and the last tree to the backend thread

## Backend thread
- On init, process a tree and initialize new widgets
- Diff a current tree and last tree:
  1. Move backend UI widget references to new tree
  2. If a widget property has changed, handle that before moving
  3. If a widget type has changed, handle that (or update all children)
  4. More virtual dom magic ...
  5. Tell UI state thread current tree has been refreshed
  6. Switch current tree and last tree
- If a widget has been interacted with, such as a button press:
  1. Store the widget's unique ID and interaction type into the input list
  2. Send an event to the UI state thread
  3. Do it again because we want to mimic BUTTON_DOWN and BUTTON_UP events

### Maintaining the backend

### Building the tree
Consider the following:
```c++
if (im_window("My Window", 640, 480, 0)) {
	if (im_button("Hello")) {
		puts("Hello");
	}
}
```
This will create a tree in memory as you'd expect.  
```xml
<window title="My Window">
  <button>Hello</button>
</window>
```

But how does `im_button` know what to return? 

Consider these scenarios:
```c++
static int state = 0;
if (state) im_label("Hello");
if (im_button("Hello")) state = 1;
```

In the implementation of `im_button`, we need to check if the widget in the last tree 
had a click event associated with it.

1. Check for events in the event queue. If none, return 0.
2. If there is an event, we need to try and check any of the events appear to point to the current widget.
  - It is guaranteed that the event will come from a widget that was created in the previous tree.
    1. Only one event is processed at a time.
    2. Each time `rim_poll` returns `1`, only a single event will be processed.

## Events
Events include button presses, keypresses, and other widget inputs. Only one event will be processed at a time.
In the case of retained mode backend:
- Every time the UI is refreshed, the onClick handler will be set to an event handler function.
- The event handler function requires an argument (that is set for each widget)

## event handler function
For any event type, accepts a single argument: `struct RimEvent`
- This struct will reside in the widget tree as a property of the widget in question.
- When the UI is refreshed, the onClick handler will be set every time with the `struct RimEvent` structure as an argument
The event handler function will copy it to the RimContext struct and trigger an event. The code in `rim_poll` will see there is an event, and return `1` for the widget that corresponds to that event.
