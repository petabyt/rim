# imgui-native
imgui backend that creates native widgets (GTK/Cocoa/Win32)

## Benefits of native UI
- Screen reader support + other accessibility features
- Adapts to OS theme/style, DPI, window decorations, etc
- UI engines can avoid unnececssary re-renders where imgui can't
- Animations
- Optimized scrolling

## Drawbacks of imgui-native
- Much larger binary
- Not finished yet
- Doesn't support imgui widgets
