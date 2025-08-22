/**
 * Handle hooking the script, calling the module register functions, etc
 */

#include <android_native_app_glue.h>
#include <android/log.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "smashhit.h"
#include "util.h"

int knEnableLog(lua_State *script);
int knEnablePeekPoke(lua_State *script);
int knEnableHttp(lua_State *script);
int knEnableSystem(lua_State *script);
int knEnableRegistry(lua_State *script);
int knEnableDatabase(lua_State *script);
int knEnableFile(lua_State *script);
int knEnableGamectl(lua_State *script);
int knEnableOverlay(lua_State *script);

extern char *gAndroidInternalDataPath;
extern char *gAndroidExternalDataPath;

#define LOAD_CORE_LIB(L, NAME, FUNC) lua_pushcfunction(L, FUNC); lua_pushstring(L, NAME); lua_call(L, 1, 0);

int load_lua_libs(lua_State *script) {
	// Load core libs manually so all of them are loaded
	LOAD_CORE_LIB(script, LUA_LOADLIBNAME, luaopen_package);
	LOAD_CORE_LIB(script, LUA_TABLIBNAME, luaopen_table);
	LOAD_CORE_LIB(script, LUA_IOLIBNAME, luaopen_io);
	LOAD_CORE_LIB(script, LUA_OSLIBNAME, luaopen_os);
	LOAD_CORE_LIB(script, LUA_STRLIBNAME, luaopen_string);
	LOAD_CORE_LIB(script, LUA_MATHLIBNAME, luaopen_math);
	LOAD_CORE_LIB(script, LUA_DBLIBNAME, luaopen_debug);
	
	// Load KnShim extensions
	knEnableLog(script);
	knEnablePeekPoke(script);
	knEnableHttp(script);
	knEnableSystem(script);
	knEnableRegistry(script);
	knEnableDatabase(script);
	knEnableFile(script);
	knEnableGamectl(script);
	knEnableOverlay(script);
	
	return 0;
}

void KNLoadLua(void);

void KNInitLua(struct android_app *app, Leaf *leaf) {
	// Install the Lua extensions
	KNLoadLua();
	
	// By some luck ARM32 and ARM64 only differ by the pointer size here - the
	// lua_openlibs reg table is the same offset from this symbol aside from that!
	luaL_Reg *lua_reg_table = (luaL_Reg *) (LeafSymbolAddr(leaf, "_ZTV17QiFileInputStream") + 6 * sizeof(void *));
	
	if (!lua_reg_table) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Failed to find lua register table");
		return;
	}
	
	// smash hit always loads it's own luaopen_base first
	// regardless of what's in the array so we actually load second
	// and avoid loading the base lib.
	lua_reg_table[1].name = "";
	lua_reg_table[1].func = load_lua_libs;
	lua_reg_table[2].name = NULL;
	lua_reg_table[2].func = NULL;
	
	// Set internal and external data paths
	gAndroidInternalDataPath = strdup(app->activity->internalDataPath);
	gAndroidExternalDataPath = strdup(app->activity->externalDataPath);
}
