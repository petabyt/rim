#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

struct Tree {
	uint8_t *buffer;
	int of;
};

struct Context {
	struct Tree curr;
	struct Tree last;
	struct WidgetHeader *last_widget;
};

enum WidgetTypes {
	UI_WINDOW = 0,
	UI_LAYOUT_STATIC,
	UI_LABEL,
	UI_BUTTON,
	UI_CUSTOM,
	UI_LIBUI,
	UI_EOF,
};

struct __attribute__((packed)) WidgetHeader {
	uint32_t type;
	uint32_t unique_id;
	uint32_t n_children;
	uint32_t n_props;
	uintptr_t os_handle;
	uint8_t data[];
	// properties start here
	// children start here
};

struct __attribute__((packed)) WidgetProp {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};

struct __attribute__((packed)) WidgetPropButton {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};

static struct Context *thread_ctx;

struct WidgetContext {
	struct WidgetHeader *header;
};

void append_child(void) {
	
}

void append_widget_properties(struct WidgetContext *w) {

}

inline static int copy_string(uint8_t *to, const char *str) {
	size_t len = strlen(str);
	int aligned_len = (((int)len + 1) / 4 + 1) * 4;
	memset(to, 0, aligned_len);
	strcpy((char *)to, str);
	return aligned_len;
}
inline static int write_u32(uint8_t *from, uint32_t x) {
	((uint32_t *)from)[0] = x;
	return 4;
}
inline static uint32_t read_u32(const uint8_t *from, uint32_t *temp) {
	*temp = ((uint32_t *)from)[0];
	return 4;
}

void add_widget(struct Context *ctx, enum WidgetTypes type) {
	struct WidgetHeader *h = (struct WidgetHeader *)(ctx->curr.buffer + ctx->curr.of);
	h->type = type;
	h->n_children = 0;
	h->n_props = 0;
	h->os_handle = 0;
	h->unique_id = 123;
	ctx->curr.of += sizeof(struct WidgetHeader);
	ctx->last_widget = h;
}

enum PropTypes {
	UI_PROP_WIN_TITLE = 0,
	UI_PROP_TEXT,
};
void add_widget_prop_text(struct Context *ctx, enum PropTypes type, const char *value) {
	struct WidgetProp *prop = (struct WidgetProp *)ctx->last_widget->data;
	prop->length = 8;
	prop->type = type;
	prop->length += copy_string(prop->data, value);
	ctx->curr.of += prop->length;
	ctx->last_widget->n_props++;
}

int nim_window(const char *title) {
	add_widget(thread_ctx, UI_WINDOW);
	add_widget_prop_text(thread_ctx, UI_PROP_WIN_TITLE, title);
	return 1;
}

int nim_init(struct Context *ctx) {
	ctx->curr.buffer = malloc(1000);
	ctx->curr.of = 0;
	return 0;
}

int nim_start(struct Context *ctx) {
	thread_ctx = ctx;
	return 0;
}

int parse_tree(struct Tree tree, int of) {
	//while (1) {
	uint32_t type;
	struct WidgetHeader *h = (struct WidgetHeader *)(tree.buffer + of);
	of += sizeof(struct WidgetHeader);

	const char *names[] = {"UI_WINDOW", "UI_LAYOUT_STATIC", "UI_LABEL", "UI_BUTTON", "UI_CUSTOM", "UI_LIBUI", "UI_EOF"};
	
	printf("Type: %s\n", names[h->type]);

	for (size_t i = 0; i < h->n_props; i++) {
		const char *prop_names[] = {"UI_PROP_WIN_TITLE", "UI_PROP_TEXT"};
		struct WidgetProp *p = (struct WidgetProp *)(tree.buffer + of);
		printf("%s = %s\n", prop_names[p->type], p->data);
		of += p->length;
	}

	//}
	return 0;
}

int main(void) {
	struct Context ctx;
	nim_init(&ctx);

	nim_start(&ctx);

	nim_window("Hello, World");
	add_widget(&ctx, UI_EOF);

	parse_tree(ctx.curr, 0);

	//nim_end(&ctx);
	
	return 0;
}
