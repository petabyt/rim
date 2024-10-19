```
Hello, World
<h1>Hello {name}!</h1>
```

```
on start:
	push label to stack
	set widget -1 id to 1
	add widget -1 child to 0 (root)
	pop

properties:
	on `name` updated:
		push root to stack
		push widget from id 1 in box -1 to stack
		set text 'Hello '
		add string property `name`
		add text '!'
		set widget -1 text to buffer
		pop
		pop
```


```
{#if x == 10}
	<p>Hello, World</p>
	{#if y == 10}
		<p>Hello, World 2</p>
	{/if}
{/if}
```

```
on start:
	push box to stack
	set widget -1 id to 1
	pop

properties:
on x updated:
	push root to stack
	push widget from id 1 in box -1 to stack
	push label to stack
	set text 'Hello, World'
	set widget -1 text to buffer
	add widget -1 to widget -2
	pop
	pop
	pop
	
on y updated:
	// Check previous conditionals
	if x != 10 goto exit
	push root to stack
	push widget from id 1 in box -1 to stack
	push label to stack
	set text 'Hello, World'
	set widget -1 text to buffer
	add widget -1 to widget -2
	pop
	pop
	pop	
```


```
Hello, World
<h1>Hello {name}!</h1>

-- on initial setup --
40 00 length
01 00 decl type (declare widget)
01 00 type (h1)
34 12 element id

40 00 length
02 00 decl type (set plain text)
00 00 parent (root)

-- on property update --
actions:
- recycle and remove all children of element ID or tree pos
- recycle child by element ID or tree pos
- `set plain text` of element ID or tree pos
- declare widget in element ID or tree pos

conditions:
- string equal
- `!=`
```
