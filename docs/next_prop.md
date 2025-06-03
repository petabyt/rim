# next_prop API

For setting widget properties with the C API, Rim has a bunch of functions named something like `im_set_next_xxx`
that will apply on the next widget created.

A few alternatives to this were considered:
- Adding more parameters to widget creation functions
  - Cumbersome
  - Worse for API backwards compatibility if more props need to be added
- Adding `_ex` or `2`/`3` widget creation functions
  - Less cumbersome, but would be more repetitive and annoying when you only need it for one change
- set_last APIs
  - Works better with rim's tree model (could just call (`rim_add_prop_xxx`)), but wouldn't work if called after a child has been added.

Dear ImGui uses a combination of both _ex functions and C++ optional parameters, as well as other specialized functions
(BeginDisabled, EndDisabled, SetTooltip, etc) to allow properties to be set.

`im_set_next_xxx` APIs allow:
- Easier to maintain API backwards compatibility
- Can be called before custom widget functions to mimick the Jetpack Compose `Modifier`

## Implementation
```
struct RimTree prop_stack_tree;
struct RimTree prop_next_tree;
```
Both of these trees have a dummy root node. Properties are added and removed during runtime.

Operations:
- Push a property
- Pop a property
- Consume all 'next' props, and read all 'stack' props

