#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <im.h>

int dummy_window_var = 1;

static int window(lua_State *L) {
	const char *title = luaL_checkstring(L, 1);
	int wid = lua_tointeger(L, 2);
	int height = lua_tointeger(L, 3);
	int rc = im_begin_window_ex(title, wid, height, &dummy_window_var);
	lua_pushinteger(L, rc);
	return 1;
}

static int end_window(lua_State *L) {
	im_end_window();
	lua_pushinteger(L, 0);
	return 1;
}

static int button(lua_State *L) {
	const char *text = luaL_checkstring(L, 1);
	int rc = im_button(text);
	lua_pushinteger(L, rc);
	return 1;
}

static int label(lua_State *L) {
	const char *text = luaL_checkstring(L, 1);
	int rc = im_label(text);
	lua_pushinteger(L, rc);
	return 1;
}


static const luaL_Reg rimlib[] = {
	{"beginWindow", window},
	{"endWindow", end_window},
	{"button", button},
	{"label", label},
	{NULL, NULL}
};

LUALIB_API int luaopen_rim(lua_State *L) {
	luaL_newlib(L, rimlib);
	return 1;
}
