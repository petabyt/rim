# Performance

- The entire tree is layed out into a single buffer rather than being a linked list.
- Each member of this structure is *strictly* aligned by 8 to prevent misaligned accesses.


