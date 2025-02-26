#ifndef IM_H
#define IM_H

int im_button(const char *label);
int im_label(const char *label);

int im_window(const char *name, int width, int height, int flags);
int im_end_window(void);

#endif
