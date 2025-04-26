// Dear-ImGUI wrapper
// Last edited 2025/4/19
#ifndef IM_H
#define IM_H

#ifdef __cplusplus
extern "C" {
#endif

/*
Widget function naming:
- im_<widget>
  - Create/start a widget with minimal arguments
- im_<widget>_ex
  - Create/start a widget with complete arguments
- im_end_<widget>
  - Ends a widget

 - Must only accept units of length/width/size in dp, not pixels.

Property functions:
- Handle widget-specific properties
- Handle pushing/popping the same properties

im_disable();
im_push_last_prop();
...
im_pop_prop();

*/

enum ImReturnCode {
	IM_NONE = 0,
	IM_CLICKED = 1,
	IM_SELECTED = 2,
	IM_CHILDREN_VISIBLE = 3,
};

struct ImModifier {
	unsigned int color;
	unsigned int width_x_type;
	unsigned int width_x;
	unsigned int width_y_type;
	unsigned int width_y;
	unsigned int margin[4];
	unsigned int padding[4];
};

void im_begin_disabled(void);
void im_end_disabled(void);

void im_set_next_tooltip(const char *label);
void im_set_next_disabled(int opt);
void im_set_next_expand();

/// @returns IM_CHILDREN_VISIBLE
int im_begin_tab_bar(int *selected);
int im_begin_tab(const char *title);
void im_end_tab(void);
void im_end_tab_bar(void);

int im_begin_combo_box(const char *label, const char *preview);
int im_add_combo_box_item(const char *label, int *selected);
void im_end_combo_box(void);

int im_checkbox_label(const char *label, int *checked);

int im_button(const char *label);
int im_button_ex(const char *label, struct ImModifier *mod);
int im_label(const char *label);

int im_begin_window(const char *name, int width_dp, int height_dp);
int im_begin_window_ex(const char *name, int width_dp, int height_dp, int *is_open);
void im_end_window(void);

/// @param buffer Buffer that the text will be read from, and where characters will be written to
/// @param size Size of buffer
void im_multiline_entry(char *buffer, unsigned int size);

/// @param buffer Buffer that the text will be read from, and where characters will be written to
/// @param size Size of buffer
void im_entry(const char *label, char *buffer, unsigned int size);

void im_slider(int min, int max, int *value);

// extension
int im_scintilla();

// For internal/extension use only
//void im_apply_prop(struct RimTree *tree);

#ifdef __cplusplus
}
#endif

#endif
