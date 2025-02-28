#include <stdlib.h>
#include <imgui.h>
extern "C" {
	#include <nim.h>
	#include <nim_internal.h>
}

extern int imgui_backend_setup_window();
extern void imgui_set_window_title(const char *title);
extern void imgui_backend_done_render();
extern int imgui_backend_poll_event_start_frame();

static struct NimContext *global_context = NULL;

extern "C" struct NimTree *nim_get_current_tree() {
	return global_context->tree_new;
}

struct NimContext *nim_init(void) {
	struct NimContext *ctx = (struct NimContext *)calloc(1, sizeof(struct NimContext));
	ctx->of = 0;
	ctx->header = 0;
	ctx->tree_new = nim_create_tree();

	return ctx;
}

extern "C" nim_ctx_t *nim_imgui_init(void) {
	struct NimContext *ctx = (struct NimContext *)calloc(1, sizeof(struct NimContext));
	ctx->tree_new = nim_create_tree();
	ctx->first_frame = 0;

	imgui_backend_setup_window();

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/open-sans/OpenSans-Regular.ttf", 30.0f);

	if (global_context == NULL) {
		global_context = ctx;
	} else {
		nim_abort("global_context can't be set twice");
	}

	return ctx;
}

static int render(struct NimTree *tree, int of, int depth, int first_window) {
	struct WidgetHeader *h = (struct WidgetHeader *)(tree->buffer + of);

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(tree->buffer + of);
		of += (int)p->length;
	}

	struct NimProp prop;

	switch (h->type) {
	case NIM_WINDOW:
		if (depth == 0 && first_window) {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::Begin("...", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);
		} else {
			if (nim_get_prop(h, &prop, NIM_PROP_WIN_TITLE)) nim_abort("no window title");
			ImGui::Begin(prop.value);
		}
		break;
	case NIM_BUTTON:
		if (ImGui::Button("...")) {
			// ?????

		}
		break;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of = render(tree, of, depth + 1, 0);
	}

	switch (h->type) {
	case NIM_WINDOW:
		if (depth == 0 && first_window) {
			ImGui::End();
			ImGui::PopStyleVar(1);
		} else {
			ImGui::End();
		}
		break;
	}

	return of;
}

extern "C" int nim_poll(struct NimContext *ctx) {
	if (ctx->first_frame) {
		render(ctx->tree_new, 0, 0, 1);
		imgui_backend_done_render();
	}

	ctx->first_frame = 1;
	return imgui_backend_poll_event_start_frame();
}