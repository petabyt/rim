### Why

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
- Renders the entire window constantly - this means high resource usage, especially in 4k fullscreen
- No animations or transitions
- No native-looking file picker widgets or dialogs

This means that for many C/C++ desktop applications, immediate-mode libraries are not an option even though they could make development
drastically easier.

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
