Rim is an immediate-mode UI library that renders down to native retained-mode widgets.

Unlike libraries like dear-imgui, it doesn't render everything from scratch. Instead, it uses the OS's native retained-mode UI toolkit while still having an immediate-mode API.

```
int main() {
	struct RimContext *ctx = rim_init();
	rim_libui_init(ctx);
	int show_more = 0;
	while (rim_poll(ctx)) {
		if (im_window("My Window", 400, 400, 0)) {
			if (im_button("Show More")) {
				show_more = 1;
			}
			if (show_more) {
				im_label("Hello, World");
			}
		}
	}
	return 0;	
}

```

It achieves this by implementing a virtual DOM similar to React. The result is a UI library with an API just like Dear ImGui, but looks, feels, and works like a native application.

### Why
Immediate-mode UI libraries are great for games, but have many limitations for desktop applications:
- Doesn't respect OS global theme, window decorations, fonts, scaling, etc
- Can't make external windows
- No screen reader support (or other accessibility features)
- Renders the entire frame at 60fps - this means high resource usage, especially in 4k fullscreen
- No animations or transitions
- No native file picker widgets or dialogs
- Can't embed complex widgets such as a webview or a video player

Drawbacks of Rim by design:
- Higher RAM usage and bigger binary
  - Static binaries will be 10mb for Linux and 1mb for Windows. ImGui binaries tend to be around 1mb.
- Doesn't support all imgui widgets/addons
- Not nearly as flexible as the renderers of some imgui libraries
