# UI Definition

## Qt/GTK
Each widget is a source file with method implementations

## Dear ImGui/Nuklear/MicroUI
For each widget, draw graphics immediately and return event if mouse is hovered

## Rim
- switch statements

Maybe a widget model could be used:
- Can be used to validate the tree (make sure only combo box item is appended to combo box)
- Can be used to debug the tree (show what properties a widget has, and what it doesn't have)

```
struct RimWidgetModel {
	int id;
	int chilren_policy;
	int *alllowed_chilren_list;
	unsigned int alllowed_chilren_list_length;
};

struct RimWidgetModel model[] = {
	{
		.id = RIM_WINDOW
	}
}
```
