# native-imgui
Immediate mode like UI library that renders down to native retained-mode widgets (such as GTK/Cocoa/Win32)
It achieves this by implementing a virtual DOM similar to React.

native-imgui tries to be easy to adopt by supporting multiple different imgui APIs (frontends), making it almost a drop-in
replacement.

Frontends:
- [x] `nim_` API
- [ ] ImGUI 1.8XX
- [ ] [microui](https://github.com/rxi/microui)
- [ ] https://github.com/Immediate-Mode-UI/Nuklear

The backend for native-imgui can be swapped too, depending on the needs of each project.

Backends:
- [ ] Libui (GTK3/win32/Cocoa)
- [ ] nappgui (GTK3/win32/Cocoa)
- [ ] https://github.com/wxWidgets/ (GTK3/4, Cocoa, Qt, Win32, etc)

Advantages over most immediate mode UIs:
- Respects OS global theme, window decorations, fonts, scaling, etc
- Screen reader support + other accessibility features
- (Sometimes) state of the art rendering engines - this means very low resource usage
- Animations
- Native file picker windows
- Embed complex widgets such as webview or video player

Drawbacks of native-imgui:
- Much larger RAM usage + binary
- Doesn't support all imgui widgets/addons

## Technical Details
In order for this to work, we need two threads:
1. UI State thread which will be running the `while (next_frame()) { ... }` UI loop (this will probably be `main()`)
2. Backend thread, which will call `uiMain`/`wxEntry`/etc
  Those calls will block, but code can still be run on this thread with functions like `uiQueueMain`

### UI State Thread responsibilities
- `next_frame` will be listening to events from the backend thread
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
  3. Do it again because we want to mimick BUTTON_DOWN and BUTTON_UP events
