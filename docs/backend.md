# Virtual DOM
*Has nothing to do with the DOM of course.*

Essentially Rim has to work with 3 different trees:
- New Tree
- Old Tree
- UI Tree in the backend

The tree differ is given 2 trees:
- The old tree
- The new tree

The old tree represents what the UI currently looks like. Each node has a pointer to
the backend toolkit widget handle which has all its properties exactly as the node describes.

The new tree represents what the UI needs to look like. None of the nodes have a pointer to
a backend toolkit widget handle.

It's up to the differ to transform the backend UI tree from what it used to be (old tree)
into what it needs to be (new tree). Sometimes this means changing properties, sometimes it means destroying
or creating layouts and attaching them to a window.

In the React world, this process is known as reconciliation.

# Event Model

In order for this to work, we need two threads:
1. **Main thread** which will be running the `while (rim_poll(ctx)) { ... }` UI loop (this be `main()`)
2. **Backend thread**, which will call `uiMain`/`wxEntry`/etc
  - Those calls will block, but code can still be run on this thread with functions like `uiQueueMain`

In order for events to go from an onClick to being returned as an event value in functions like `im_button`, these two conditions must be gauranteed:

- Only one event can be processed at a time
- An event must come from a widget that existed in the old tree and will exist in the new tree in the same place.

If these are gauranteed, then it's possible to match a widget between two layouts and route events to the correct `im_button` call.

This doesn't work if the state of the UI changes between events. Basic property changes such as an event counter are fine, but adding/removing/changing widgets
aren't allowed.
