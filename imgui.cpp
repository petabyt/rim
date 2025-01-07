#include <imgui.h>
extern "C" {
#include "nim.h"
}

static struct Tree *global_tree = NULL;

bool ImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
	assert(global_tree);
	nim_add_widget(global_tree, UI_WINDOW, -1);
	nim_add_prop_text(global_tree, UI_PROP_WIN_TITLE, name);
	return true;
}
void ImGui::End() {
	assert(global_tree);
	nim_end_widget(global_tree);
}
bool ImGui::Button(const char* label, const ImVec2& size) {
	assert(global_tree);
	nim_add_widget(global_tree, UI_BUTTON, 0);
	nim_add_prop_text(global_tree, UI_PROP_TEXT, label);
	nim_end_widget(global_tree);
	return false;
}

extern "C" int build_my_ui(struct Tree *tree, int state) {
	global_tree = tree;
	ImGui::Begin("Hello");
	ImGui::Button("Tester");
	ImGui::Button("Tester");
	ImGui::End();
	return 0;
}
