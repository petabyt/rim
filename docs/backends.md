Goal is to have a minimal number of backends while supporting as many use cases and platforms as possible.

# LibUI

Pros:
- Easy to compile (100 C source files)
- CMake and meson build system
- Easy to cross compile
Cons:
- Not active, dead
- Hardly any features
- Will only ever support GTK3/Win32/Cocoa

LibUI is not very active, but it's pretty stable and easy to compile. This is especially useful for cross compiling
(LibUI can be compiled on Linux, for Windows/Mac/Linux). It's great for very simple applications.

I'm using my own fork with a ton of merged features: https://github.com/petabyt/libui-dev

# wxWidgets

Pros:
- Active, mature, very feature rich
- Very stable and works well
- Supports lots of toolkits (qt6 and gtk4 in progress)
Cons:
- Huge (long compile time)
- Has lots of submodules

wxWidgets is active, mature, and very feature rich. The main problem is that it's huge.
Around 1000 source files for each platform. Not to mention how slow GCC is at compiling C++.
It takes around 30 mintes to compile on my PC compared to a few seconds for LibUI.

With a minimized build, it only takes around 400 source files and 2 minutes for a full build.
That's closer but I would like it under a minute.

# ~~Direct GTK4/5 backend for Linux~~

Pros:
- Most popular Linux toolkit
Cons:

I've lost all faith in the GTK project. They had so many chances to get their API right.
GTK1, GTK2, GTK3, 4, and how 5 is in the talks. Even within those versions they create and deprecate APIs
often. If they can't get the API right after 4 versions I don't think they ever will.

What's even worse is that even after all this work, it debatably hasn't improved much at all over the years.

# Direct Qt 5/6 backend for Linux (and others?)

Pros:
- Best in class toolkit for desktop applications, and more
Cons:
- Huge, 40-60mb (maybe can use a custom minimal build?)

I think Qt is the very best framework for desktop applications right now, and it clearly has been for a long time. It
dominates in desktop applications. Even Tesla used it for their head unit UI.

# Jetpack Compose

Jetpack Compose has a significant investment in it by both Google and Jetbrains. It's currently the primary UI engine for Android
with iOS and Desktop support being worked on. It's not as cross-platform as Flutter, but it definitely has more adoption.
Google has already rewritten the settings and and the play store in compose so I think that's a clear signal it's the way of the future.

# Android Views

This would be very easy to get working, but views is in maintanence mode and will only get slower and more broken over time.

# ~~Flutter~~

Sorry but I think Flutter will die the same death as Adobe Flash. Dart isn't used anywhere but Flutter. I mean honestly who would in
their right mind write anything other than a Flutter app in Dart? It's a dead language held up by Google.
And given Google's habits with killing things, I think they'd be happy to let go of Flutter.
Not to mention the rendering jank and all the other problems. For the few major apps that are written in Flutter you can often times tell
because of the lag.
Even Google barely uses it themselves.

# FLTK

Pros:
- Incredibly minimal, easy to compile
- Consistent look across platforms
Cons:
- Missing lots of features.
- Straight out of the 90s
