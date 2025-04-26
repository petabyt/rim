### Why

There's plenty of reasons why immediate-mode libraries are better then retained-mode:
- Faster and more responsive
- State only has to be maintained in one place
- Reduces potential for bugs

But given the nature of immediate-mode UI libriares, they have many limitations by design:

- No layout engine - most layouts have to be calculated manually
- Can't make external windows
- Doesn't respect the desktop's global theme, fonts, scaling, etc
- No screen reader support (or other accessibility features)
- Renders the entire window at 60fps - this means high resource usage, especially in 4k fullscreen
- No animations or transitions
- No native file picker widgets or dialogs
- Can't embed complex widgets such as a webview or a video player

This means that for many desktop applications, immediate-mode libraries are not an option even though they could make development
drastically easier.

What if you can get the best of both worlds without the drawbacks?

Rim is able to solve all of the above stated limitations while still being as easy to use as Dear ImGui.

Although there are some important limitations (by design) in Rim to consider:
- It's not finished yet (duh)
- Higher RAM usage and bigger binary
  - Static binaries will be 10mb for Linux and 1mb for Windows. ImGui binaries tend to be around 1mb.
- Slow updating
  - Virtual DOM is [always slower](https://svelte.dev/blog/virtual-dom-is-pure-overhead) if you compare it to a perfect retained-mode UI
  - The Rim tree differ is not very smart, and will often do a lot more work than necessary for drastic relayouts
  - For single property updates (such as a text change), rim should be very fast and have virtually no overhead
- Not nearly as flexible as the renderers of some imgui libraries
