#include <android_native_app_glue.h>
#include <android/log.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

// LOG
int knLog(lua_State *script) {
	int args = lua_gettop(script);
	
	if (args < 1 || args > 2) {
		return 0;
	}
	
	const char *msg = lua_tostring(script, args);
	
	int level = (args == 2) ? lua_tointeger(script, 1) : ANDROID_LOG_INFO;
	
	if (msg) {
		__android_log_write(level, "smashhit", msg);
	}
	else {
		return luaL_error(script, "Message is NULL");
	}
	
	return 0;
}
// END LOG

int knEnableLog(lua_State *script) {
	lua_register(script, "knLog", knLog);
	lua_pushinteger(script, ANDROID_LOG_INFO); lua_setglobal(script, "LOG_INFO");
	lua_pushinteger(script, ANDROID_LOG_WARN); lua_setglobal(script, "LOG_WARN");
	lua_pushinteger(script, ANDROID_LOG_ERROR); lua_setglobal(script, "LOG_ERROR");
	
	return 0;
}
