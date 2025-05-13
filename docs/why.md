### Why not just use <imgui library>?

There's plenty of reasons why immediate-mode libraries are better then retained-mode ones:
- State only has to be maintained in one place
- Faster and more responsive
- Reduces potential for bugs

But given the nature of immediate-mode UI libriares, they have many limitations:

- No layout engine - most layouts have to be calculated manually
- Can't embed complex widgets such as a webview or a video player
- Can't make external windows or dialogs using the OS window decorations
- No screen reader support (or other accessibility features)
- Doesn't respect the desktop's global theme, fonts, scaling, etc
- Renders the entire window on every update - this means high GPU usage, especially in 4k fullscreen
- No animations or transitions
- No native-looking file picker widgets or dialogs

This means that for many C/C++ desktop applications, immediate-mode libraries can't be considered as an option.

Rim is an attempt to get the best of both worlds. It's capable of solving all of the problems with traditional ImGui but with some drawbacks:
- It's not finished yet (duh)
- Higher RAM usage and bigger binary if statically linked
  - Static binaries will be 10mb for Linux and 1mb for Windows. ImGui binaries tend to be around 1mb.
  - Of course this is at the mercy of however big these native toolkits are
- Slow updating
  - Virtual DOM is [always slower](https://svelte.dev/blog/virtual-dom-is-pure-overhead) if you compare it to a perfect retained-mode UI
  - The Rim tree differ is not very smart, and will often do a lot more work than necessary for drastic relayouts
  - Although for single property updates (such as a text change), rim should be very fast and have virtually no overhead
- Not nearly as flexible as the renderers of some imgui libraries
  - Just look at https://raw.githubusercontent.com/wiki/ocornut/imgui/web/v176/tracy_profiler.png - GTK will never be able to do anything like that :(

### Why not just use <retained-mode library>

It's well known that properly maintaining UI state in a retained-mode application is difficult, very bug prone, and mostly downright painful.
State has to be maintained in two different places using glue code (with the best approach being [MVVM](https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93viewmodel)).
To put it simply, it takes a lot of time to do it right - and in the end you have thousands of lines of code to look after.

Immediate-mode UIs solve this problem, since the UI automatically reflects the state the backend uses. It also removes the need for onclick callbacks, and hard
to read UI setup code.

There are still important use-cases for writing retained-mode UIs, especially for scenarios where performance is important.
