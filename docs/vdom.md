```
<box>
	<label>Hello</label>
</box>
```

```
<box>
	<box>Hello</box>
	<label>Hello</label>
</box>
```

- [0] is same
  - new[0][0] is different type
    - Delete old[0][0] (and remove from parent)
  - new[0][1] only exists on new tree, init & append

- [0] is same
  - new[0][0] is different type
    - Destroy old[0][0]
    - Init new[0][0] and append to parent index 0
    - new[0][1] and new[0][0] is the same
