Nim is an immediate-mode UI library that renders down to native retained-mode widgets.

Unlike libraries like dear-imgui, it doesn't render everything from scratch. Instead, it uses the OS's native retained-mode UI toolkit while still having an immediate-mode API.

```
int main() {
	struct NimContext *ctx = nim_init();
	nim_libui_init(ctx);
	int show_more = 0;
	while (nim_poll(ctx)) {
		if (im_window("My Window", 640, 480, 0)) {
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
Retained mode libraries tend to have many advantages over most immediate-mode UI libraries:
- Respects OS global theme, window decorations, fonts, scaling, etc
- Screen reader support + other accessibility features
- Only re-renders when the window content has changed (very low resource usage)
- Animations and transitions
- Native file picker windows, dialogs, etc
- Embed complex widgets such as a webview or video player

Drawbacks of native-imgui:
- Much larger RAM usage + binary
- Doesn't support all imgui widgets/addons
- Not nearly as flexible as the renderers of some imgui libraries
