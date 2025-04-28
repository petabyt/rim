#include <stdio.h>
#include <rim.h>
#include <im.h>

#include <libui_scintilla.h>

void init(uiScintilla *handle) {
	uiScintillaSetText(handle, "Hello");
	uiScintillaSendMessage(handle, SCI_SETVIEWWS, SCWS_INVISIBLE, 0);
	uiScintillaSendMessage(handle, SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET, 0xffffffff);
	uiScintillaSendMessage(handle, SCI_SETELEMENTCOLOUR, SC_ELEMENT_WHITE_SPACE_BACK, 0x0);
	uiScintillaSendMessage(handle, SCI_STYLESETBACK, STYLE_DEFAULT, 0x080808);
	uiScintillaSendMessage(handle, SCI_STYLESETFORE, STYLE_DEFAULT, 0xffffff);

	uiScintillaSendMessage(handle, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, 0xffaaaaaa);
	uiScintillaSendMessage(handle, SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_TEXT, 0xff000000);

#ifdef WIN32
	uiScintillaSendMessage(handle, SCI_STYLESETFONT, STYLE_DEFAULT, (uintptr_t)"FreeMono");
#else
	uiScintillaSendMessage(handle, SCI_STYLESETFONT, STYLE_DEFAULT, (uintptr_t)"Monospace");
#endif

	uiScintillaSendMessage(handle, SCI_STYLECLEARALL, 0, 0);
	uiScintillaSendMessage(handle, SCI_SETTABWIDTH, 4, 0);

	uiScintillaSendMessage(handle, SCI_SETMARGINMASKN, 0, 0);
	uiScintillaSendMessage(handle, SCI_STYLESETFORE, STYLE_LINENUMBER, 0xeeeeee);
	uiScintillaSendMessage(handle, SCI_STYLESETBACK, STYLE_LINENUMBER, 0x262626);
	uiScintillaSendMessage(handle, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
	uiScintillaSendMessage(handle, SCI_SETMARGINWIDTHN, 0, 30);
}

int main(void) {
	struct RimContext *ctx = rim_init();
	rim_scintilla_init(ctx);
	while (rim_poll(ctx)) {
		if (im_begin_window("Rim Lua Demo", 1000, 1000)) {
			im_scintilla(1, init);
			if (im_button("Run")) {
				printf("%s\n", rim_scintilla_get_text(ctx, 1));
			}
			im_end_window();
		}
	}

	return 0;
}
