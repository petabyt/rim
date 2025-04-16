# Basic example of a custom widget
```
void im_my_widget(const char *text) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, 0x8008135, 0);
	nim_add_prop_text(tree, NIM_PROP_TEXT, label);
	nim_end_widget(tree);
}
```

Like `nim_libui_init`, there is also this:
```
int nim_my_widget_init(nim_ctx_t *ctx);
```

This will add handlers to the extension list of the context that will look just like the libui handlers, but only work on widget type `0x8008135`.

# How is im_scintilla loaded

`im_scintilla` is a special widget, its state is retained through `widget_id` and other code must request the data from it, rather than it
updating a buffer like `im_entry`.

```
void im_scintilla(void (*sc_setup)(void *), int widget_id) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, NIM_CUSTOM, -1);
	nim_add_prop_u32(tree, NIM_PROP_META, (uint32_t)widget_id);
}
unsigned int im_scintilla_get_length(int widget_id);
unsigned int im_scintilla_get_buffer(int widget_id, char *buffer);
```
