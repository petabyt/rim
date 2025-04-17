#include <stdio.h>
#include <rim.h>
#include <im.h>
#include <rim_internal.h>

int dump_widget_location(void) {
	struct RimTree *tree = rim_get_current_tree();
	for (int i = 0; i < tree->widget_stack_depth; i++) {
		printf("parent type %s child %d\n", rim_eval_widget_type(tree->widget_stack[i]->type), tree->widget_stack[i]->n_children - 1);
	}
	printf("child is type: %s\n", rim_eval_widget_type(tree->widget_stack[tree->widget_stack_depth]->type));
	return 0;
}

int main() {
	struct RimContext *ctx = rim_init();
	rim_libui_init(ctx);
	int show_more = 0;

	if (im_window("My Window", 640, 480)) {
		if (im_button("Show More")) {
			show_more = 1;
		}
		dump_widget_location();
		if (show_more) {
			im_label("Hello, World");
		}
	}

	return 0;
}
