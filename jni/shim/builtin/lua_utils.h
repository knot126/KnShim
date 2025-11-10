#ifndef _LUA_UTILS_H_
#define _LUA_UTILS_H_

#include "smashhit.h"

#include "../lua/lua.h"
#include "../lua/lualib.h"
#include "../lua/lauxlib.h"

static inline QiVec2 knLuaToVec2(lua_State *L, int index) {
	QiVec2 v;
	
	if (lua_istable(L, index)) {
		lua_pushinteger(L, 1); lua_gettable(L, index); v.x = lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushinteger(L, 2); lua_gettable(L, index); v.y = lua_tonumber(L, -1); lua_pop(L, 1);
	}
	else {
		v = (QiVec2) {0.0, 0.0};
	}
	
	return v;
}

static inline QiVec3 knLuaToVec3(lua_State *L, int index) {
	QiVec3 v;
	
	if (lua_istable(L, index)) {
		lua_pushinteger(L, 1); lua_gettable(L, index); v.x = lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushinteger(L, 2); lua_gettable(L, index); v.y = lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushinteger(L, 3); lua_gettable(L, index); v.z = lua_tonumber(L, -1); lua_pop(L, 1);
	}
	else {
		v = (QiVec3) {0.0, 0.0, 0.0};
	}
	
	return v;
}

static inline QiColor knLuaToColor(lua_State *L, int index) {
	QiColor v;
	
	if (lua_istable(L, index)) {
		lua_pushinteger(L, 1); lua_gettable(L, index); v.r = lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushinteger(L, 2); lua_gettable(L, index); v.g = lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushinteger(L, 3); lua_gettable(L, index); v.b = lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushinteger(L, 4); lua_gettable(L, index); v.a = lua_tonumber(L, -1); lua_pop(L, 1);
	}
	else {
		v = (QiColor) {0.0, 0.0, 0.0, 1.0};
	}
	
	return v;
}

static inline void knLuaPushVec2(lua_State *L, QiVec2 v) {
	lua_newtable(L);
	lua_pushinteger(L, 1); lua_pushnumber(L, v.x); lua_settable(L, -3);
	lua_pushinteger(L, 2); lua_pushnumber(L, v.y); lua_settable(L, -3);
}

#endif // _LUA_UTILS_H_
