# native-imgui
drop-in immediate mode UI that creates native widgets (GTK/Cocoa/Win32)
It achieves this by implementing a virtual DOM similar to React.

Features
- Easy to setup and cross-compile
- Escape hatch for falling back to native toolkits

imgui-native breathes new life into retained UI engines that would otherwise be hard to use, and gives
users of dear-imgui another choice for rendering.

Given that traditional imgui libraries are not suitable for large scale apps,
native OS apps must still use traditional retained frameworks like GTK or Qt to create UIs.

imgui-native combines the benefits the modern immediate mode API and the performance and versatility benefits
of traditional retained UI engines.

Most immediate mode UI libraries suffer from some common drawbacks when used in desktop apps:
- Doesn't respect OS theme
- No accessibility features (screen reader, high contrast)
- *Very high* GPU usage
- Frame re-renders on every user interaction in the best case scenario
- Constant rendering at 60fps or higher in the worst case scenario

Retained mode libraries are very good at what imguis tend to be bad at: 
- Screen reader support + other accessibility features
- Automatically adapts to OS theme/style, DPI, window decorations, etc
- Re-rendering only widgets that have changed
- Animations

So 

## Drawbacks of native-imgui over dear-imgui
- Much larger binary footprint
- Not finished yet
- Doesn't support imgui widgets
