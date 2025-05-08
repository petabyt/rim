# Tree patcher performance

No performance benchmarks have been done but there are several things that can reduce the amount of work needed for relayouts.

- Widget Recycler
  - Detach a widget from its parent and re-attach it to something else later on
  - This mainly just involves removing unnecessary allocs/frees, so it's not clear if it would help that much
- Remove at index X and Insert at index X
  - LibUI is not capable of inserting widgets into a tree, only appending.
  - Without the ability to insert widgets, the entire node has to be thrown away if only the type of a sibling widget changes 
- A smarter tree differ
  - https://thume.ca/2017/06/17/tree-diffing/

# Runtime performance

Several things have been done to make sure performance is good here:

- The entire tree is layed out into a single buffer rather than being a linked list.
- Each member of this structure is *strictly* aligned by 8 to prevent misaligned accesses.
- `already_fulfilled` API in property nodes to prevent duplicate property changes
