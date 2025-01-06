/*
 *	loli-loader testsuite
 *	/tests/lextlinux.c
 *	Lua binding to extlinux.c
 */

#include "extlinux.h"

#include <lua.h>
#include <lauxlib.h>

static int
lextlinux_next(lua_State *l)
{
	const char *conf = NULL, *entry = NULL;

	switch (lua_type(l, 1)) {
	case LUA_TLIGHTUSERDATA:
		entry = lua_touserdata(l, 1);
		break;
	case LUA_TSTRING:
		conf = lua_tostring(l, 1);
		break;
	}

	const char *p = extlinux_next_entry(conf, entry);
	if (p)
		lua_pushlightuserdata(l, (void *)p);

	return p ? 1 : 0;
}

static int
lextlinux_get(lua_State *l)
{
	const char *p = NULL;
	switch (lua_type(l, 1)) {
	case LUA_TLIGHTUSERDATA:
		p = lua_touserdata(l, 1);
		break;
	case LUA_TSTRING:
		p = lua_tostring(l, 1);
		break;
	}

	const char *key = luaL_checkstring(l, 2);

	size_t len;
	const char *value = extlinux_get_value(p, key, &len);
	if (value)
		lua_pushlstring(l, value, len);

	return value ? 1 : 0;
}

static const luaL_Reg extlinux_funcs[] = {
	{ "next", lextlinux_next },
	{ "get", lextlinux_get },
	{ NULL, NULL },
};

int
luaopen_lextlinux(lua_State *l)
{
	luaL_newlib(l, extlinux_funcs);
	return 1;
}
