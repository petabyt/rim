### Why

ImGui libraries are great. Especially for games. But when it comes to making desktop applications, they fall short (by design):

- No layout engine - most layouts have to be calculated manually
- Can't make external windows
- Doesn't respect the desktop's global theme, fonts, scaling, etc
- No screen reader support (or other accessibility features)
- Renders the entire window at 60fps - this means high resource usage, especially in 4k fullscreen
- No animations or transitions
- No native file picker widgets or dialogs
- Can't embed complex widgets such as a webview or a video player

Even though libraries like Dear ImGui are a joy to use, these limitations put them out of reach for many use cases.

Drawbacks of Rim by design:
- Higher RAM usage and bigger binary
  - Static binaries will be 10mb for Linux and 1mb for Windows. ImGui binaries tend to be around 1mb.
- Doesn't support all imgui widgets/addons
- Not nearly as flexible as the renderers of some imgui libraries
