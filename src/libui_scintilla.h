#include <ui.h>
#include <ui_scintilla.h>
#include <Scintilla.h>

int rim_scintilla_init(rim_ctx_t *ctx);

int im_scintilla(int id, void (*init)(uiScintilla *handle));

char *rim_scintilla_get_text(rim_ctx_t *ctx, int id);
