#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rim.h>
#include <im.h>
#include <libui_scintilla.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "script.h"

LUALIB_API int luaopen_rim(lua_State *L);

extern int dummy_window_var;

void init(uiScintilla *handle) {
	script_lua[script_lua_len - 1] = '\0';
	uiScintillaSetText(handle, (char *)script_lua);
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

int mymain(rim_ctx_t *ctx, void *arg) {
	rim_scintilla_init(ctx);

	lua_State *L = luaL_newstate();
	luaopen_base(L);
	luaL_requiref(L, "im", luaopen_rim, 1);

	int is_running = 0;
	int lua_rc = 0;
	while (rim_poll(ctx)) {
		int run_lua = 0;
		if (im_begin_window("Rim Lua Demo", 1000, 1000)) {
			im_scintilla(1, init);
			if (is_running && dummy_window_var) {
				if (im_button("Stop")) {
					is_running = 0;
					dummy_window_var = 0;
				}
			} else {
				if (im_button("Run")) {
					run_lua = 1;
					is_running = 1;
					dummy_window_var = 1;
				}
			}
			im_end_window();
		}

		char error_buffer[128];
		if (run_lua) {
			printf("Loading\n");
			char *code = rim_scintilla_get_text(ctx, 1);
			if (luaL_loadbuffer(L, code, strlen(code), "script") != LUA_OK) {
				snprintf(error_buffer, sizeof(error_buffer), "Failed to load Lua: %s", lua_tostring(L, -1));
				printf("%s\n", error_buffer);
				lua_rc = 1;
			} else {
				if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
					snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
					printf("%s\n", error_buffer);
					lua_rc = 1;
				} else {
					lua_rc = 0;
				}
			}
			free(code);
		}

		if (is_running && lua_rc == 0) {
			rim_tree_save_state();
			lua_getglobal(L, "loop");
			if (lua_isfunction(L, -1)) {
				if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
					snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
					printf("%s\n", error_buffer);
					lua_rc = 1;
				}
			} else {
				snprintf(error_buffer, sizeof(error_buffer), "loop is not a function: %s", lua_tostring(L, -1));
				printf("%s\n", error_buffer);
				lua_rc = 1;
			}
			if (lua_rc) {
				rim_tree_restore_state();
			}
		}

		if (is_running && lua_rc == 1) {
			if (im_begin_window_ex("Error", 100, 100, &is_running)) {
				im_label(error_buffer);
				im_end_window();
			}
		}
	}

	return 0;
}

int main(void) {
	return rim_start(mymain, NULL);
}
