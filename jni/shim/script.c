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

#include "enablement.h"

int gDisabledModules = 0;

int knSetDisabled(lua_State *script) {
	gDisabledModules = lua_tointeger(script, 1);
	return 0;
}

#define LOAD_CORE_LIB(L, NAME, FUNC) lua_pushcfunction(L, FUNC); lua_pushstring(L, NAME); lua_call(L, 1, 0);

int luaL_openlibs_hook(lua_State *script) {
	// Load core libs manually so all of them are loaded
	LOAD_CORE_LIB(script, "", luaopen_base);
	LOAD_CORE_LIB(script, LUA_LOADLIBNAME, luaopen_package);
	LOAD_CORE_LIB(script, LUA_TABLIBNAME, luaopen_table);
	LOAD_CORE_LIB(script, LUA_IOLIBNAME, luaopen_io);
	LOAD_CORE_LIB(script, LUA_OSLIBNAME, luaopen_os);
	LOAD_CORE_LIB(script, LUA_STRLIBNAME, luaopen_string);
	LOAD_CORE_LIB(script, LUA_MATHLIBNAME, luaopen_math);
	LOAD_CORE_LIB(script, LUA_DBLIBNAME, luaopen_debug);
	
	// Load KnShim extensions
	KNSHIM_ENABLE();
	
	if (!gDisabledModules) {
		KNSHIM_PUSH_ENABLE_ENUM();
		knRegisterFunc(script, knSetDisabled);
	}
	
	return 0;
}

void KNLoadLua(void);

extern char *gAndroidInternalDataPath;
extern char *gAndroidExternalDataPath;

const char *KNInitLua(void) {
	// NOTE: We used to ship our own copy of Lua built into the shim, but this
	// frequently broke because Smash Hit changes the internal structure of
	// tables from the publicly released version, leading to profound memory
	// corruption. Relying on Smash Hit's internal copy is a lot nicer, anyway,
	// and reduces the shim size by ~30%.
	KNLoadLua();
	
	// While we used to add our own function to the lua loading library, we
	// don't do that anymore and instead just make a replacement hook for
	// luaL_openlibs. It's easier and more portable across games.
	if (!KNHookFunctionByName("luaL_openlibs", luaL_openlibs_hook, true)) {
		return "Failed to replace luaL_openlibs";
	}
	
	// TODO: Most likely not needed anymore
	gAndroidInternalDataPath = strdup(gApp->activity->internalDataPath);
	gAndroidExternalDataPath = strdup(gApp->activity->externalDataPath);
	
	return NULL;
}
