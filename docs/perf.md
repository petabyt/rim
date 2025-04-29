# Performance improvements

No performance benchmarks have been done but there are several things that can reduce the amount of work needed for relayouts.

- Widget Recycler
  - Detach a widget from its parent and retach it to something else later on
  - This mainly just involves removing unnecessary allocs/frees, so it's not clear if it would help that much
- Remove at index X and Insert at index X
  - LibUI is not capable of inserting widgets into a tree, only appending.
  - Without the ability to insert widgets, the entire tree has to be thrown away if only the type of a widget changes 
- A smarter tree differ
  - https://thume.ca/2017/06/17/tree-diffing/
