# Rim
*Retained &larr; Immediate Mode UI*

A virtual DOM for retained mode UI libraries.

Features:
- Modular backend
  - currently libui is the only backend. This could be swapped with wxWidgets or any other toolkit.
- Modern API
  - `dp` is used as a unit of measurement rather than `px`.
  - The API is also modular. If you don't like it, you can just rewrite it.
- Supports Win32 (Windows Common Controls), MacOS (Cocoa), and Linux (GTK3) through [libui](https://github.com/libui-ng/libui-ng)

```
cmake -G Ninja -B build && cmake --build build
```
