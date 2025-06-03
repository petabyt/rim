# Writing Rim extensions

Extensions need these things:
- A unique extension ID
- A unique widget ID
- Retained-mode handlers
- Immediate-mode interface (`im_` functions)

Once you have implemented those things, you can register an extension in Rim like:
```
int rim_myext_init(struct RimContext *ctx) {
	struct RimExtension ext = {0};
	ext.create = my_create,
	ext.append = my_append,
	ext.tweak = my_tweak,
	ext.remove = my_remove,
	ext.destroy = my_destroy,
	ext.update_onclick = my_update,
	ext.ext_id = EXTENSION_ID,
	ext.priv = calloc(1, sizeof(struct Priv));
	rim_add_extension(ctx, &ext);
	return 0;
}
```
