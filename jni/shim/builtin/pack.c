#include "lua_utils.h"
#include "../util.h"

int knPack(lua_State *script) {
	int atype = lua_type(script, 1);
	
	if (atype == LUA_TSTRING) {
		const char *typename = lua_tostring(script, 1);
		
		if (!strcmp(typename, "float")) {
			float n = lua_tonumber(script, 2);
			lua_pushlstring(script, (const char *) &n, sizeof n);
		}
		else if (!strcmp(typename, "double")) {
			double n = lua_tonumber(script, 2);
			lua_pushlstring(script, (const char *) &n, sizeof n);
		}
		else if (!strcmp(typename, "char") || !strcmp(typename, "bool")) {
			unsigned char n = lua_tointeger(script, 2);
			lua_pushlstring(script, (const char *) &n, sizeof n);
		}
		else if (!strcmp(typename, "short")) {
			short n = lua_tointeger(script, 2);
			lua_pushlstring(script, (const char *) &n, sizeof n);
		}
		else if (!strcmp(typename, "int")) {
			int n = lua_tointeger(script, 2);
			lua_pushlstring(script, (const char *) &n, sizeof n);
		}
		else if (!strcmp(typename, "long")) {
			int64_t n = lua_tointeger(script, 2);
			lua_pushlstring(script, (const char *) &n, sizeof n);
		}
		else {
			luaL_error(script, "Invalid pack type string: '%s'", typename);
		}
	}
	else if (atype == LUA_TNIL || atype == LUA_TNONE) {
		luaL_error(script, "Cannot pack nil (or none) value; you probably forgot to pass any arguments");
	}
	else {
		luaL_error(script, "Pack type expects a string, not a value");
	}
	
	return 1;
}

int knEnablePack(lua_State *script) {
	knRegisterFunc(script, knPack);
	
	return 0;
}
