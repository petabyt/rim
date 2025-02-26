// Experimental imgui frontend
#include <imgui.h>
extern "C" {
	#include "nim.h"
}

bool ImGui::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, NIM_WINDOW, -1);
	nim_add_prop_text(tree, NIM_PROP_WIN_TITLE, name);
	return true;
}
void ImGui::End() {
	struct NimTree *tree = nim_get_current_tree();
	nim_end_widget(tree);
}
bool ImGui::Button(const char* label, const ImVec2& size) {
	struct NimTree *tree = nim_get_current_tree();
	nim_add_widget(tree, NIM_BUTTON, 0);
	nim_add_prop_text(tree, NIM_PROP_TEXT, label);
	nim_end_widget(tree);
	return false;
}

extern "C" int nim_example_ui(int state) {
	ImGui::Begin("Hello");
	ImGui::Button("Tester");
	ImGui::Button("Tester two");
	ImGui::End();
	return 0;
}

extern "C" int nim_demo_window1(int state) {
	ImGui::Begin("Hello");
	ImGui::Button("Tester");
	ImGui::Button("Tester two");
	ImGui::End();
	return 0;
}

extern "C" int nim_demo_window2(int state) {
	ImGui::Begin("Hello");
	ImGui::Button("Tester");
	ImGui::End();
	return 0;
}
