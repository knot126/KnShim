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
int knEnablePatching(lua_State *script);
int knEnableHttp(lua_State *script);
int knEnableSystem(lua_State *script);
int knEnableRegistry(lua_State *script);
int knEnableDatabase(lua_State *script);
int knEnableFile(lua_State *script);
int knEnableGamectl(lua_State *script);
int knEnableOverlay(lua_State *script);
int knEnableShaders(lua_State *script);

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
	knEnablePatching(script);
	knEnableHttp(script);
	knEnableSystem(script);
	knEnableRegistry(script);
	knEnableDatabase(script);
	knEnableFile(script);
	knEnableGamectl(script);
	knEnableOverlay(script);
	knEnableShaders(script);
	
	return 0;
}

void KNLoadLua(void);

const char *KNInitLua(void) {
	// NOTE: We used to ship our own copy of Lua built into the shim, but this
	// frequently broke because Smash Hit changes the internal structure of
	// tables from the publicly released version, leading to profound memory
	// corruption. Relying on Smash Hit's internal copy is a lot nicer, anyway,
	// and reduces the shim size by ~30%.
	KNLoadLua();
	
	// NOTE: By some luck ARM32 and ARM64 only differ by the pointer size here - the
	// lua_openlibs reg table is the same offset from this symbol aside from that!
	luaL_Reg *lua_reg_table = (luaL_Reg *) (KNGetSymbolAddr("_ZTV17QiFileInputStream") + 6 * sizeof(void *));
	
	// Probably due to compiler optimisations, the first value in the reg table
	// is never used and always point to Smash Hit's own luaopen_base. To make
	// less trouble, we just go after the base lib.
	// 
	// TODO: Would it be more worthwhile to replace luaL_openlibs() with a
	// custom implementation via hooking instead of doing this?
	lua_reg_table[1].name = "";
	lua_reg_table[1].func = load_lua_libs;
	lua_reg_table[2].name = NULL;
	lua_reg_table[2].func = NULL;
	
	// TODO: Most likely not needed anymore
	gAndroidInternalDataPath = strdup(gApp->activity->internalDataPath);
	gAndroidExternalDataPath = strdup(gApp->activity->externalDataPath);
	
	return NULL;
}
