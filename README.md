# Rim
*Retained &larr; Immediate Mode UI*

Rim is an immediate-mode UI library written in C that renders down to native retained mode widgets.
It's able to do this using a [virtual DOM](https://en.wikipedia.org/wiki/Virtual_DOM) to diff UI trees and apply
changes to a retained mode tree.

If that doesn't make sense to you, imagine React but:
- Replace Javascript with C
- Replace JSX API with the Dear ImGui API
- Replace HTML/CSS with GTK, Cocoa, or any other UI toolkit

That's what Rim is.

# Features:
**Modular backend**

- Rim is capable of supporting multiple different UI toolkits (backends).
- currently [LibUI](https://github.com/libui-ng/libui-ng) is the only backend. Adding a Qt or wxWidgets backend would be trivial.
- LibUI backend Supports Win32 (Windows Common Controls), MacOS (Cocoa), and Linux (GTK3)

**Powerful API**

- Based on Dear ImGui but greatly simplified
- 
- `dp` is used as a unit of measurement rather than `px`. (It's not 2009 anymore, HiDPI screens exist)
- The API is also modular. If you don't like it, you can just rewrite it.

**Fast**

The tree builder and differ is fast enough that there should be virtually zero overhead for basic changes. (eg: text in a label changes)

**Easy to use**
```
int main(void) {
	struct RimContext *ctx = rim_init();
	rim_libui_init(ctx);
	int show_more = 0;
	while (rim_poll(ctx)) {
		if (im_window("My Window", 500, 500)) {
			if (im_button("Hello")) show_more != show_more;
			if (show_more) im_label("Two labels with the same text!");
		}
	}
	return 0;
}
```

# Compiling
Rim is compiled as a static library with the backend included.
```
cmake -G Ninja -B build && cmake --build build
```



# Roadmap

- A way to handle RecyclerViews/tables
- wxWidgets backend
- Jetpack Compose or Views backend
