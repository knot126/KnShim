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

extern char *gAndroidInternalDataPath;
extern char *gAndroidExternalDataPath;

#ifndef HYPERSPACE
int load_lua_libs(lua_State *script) {
	// __android_log_print(ANDROID_LOG_INFO, TAG, "Hello from lua! Opening libs..");
	
	// Open default libs
	luaL_openlibs(script);
	
	// Logging tools
	knEnableLog(script);
	
	// Memory manipulation
	knEnablePeekPoke(script);
	
	// HTTP
	knEnableHttp(script);
	
	// System utilities
	knEnableSystem(script);
	
	// Registry
	knEnableRegistry(script);
	
	// Database
	knEnableDatabase(script);
	
	// Better file reading and writing
	knEnableFile(script);
	
	// Game control
	knEnableGamectl(script);
	
	return 0;
}

void KNInitLua(struct android_app *app, Leaf *leaf) {
	// Install the Lua extensions
	
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

#else

int knEnableOverlay(lua_State *script);

void (*real_script_load_func)(Script *this, QiString *path);

static void script_load_hook(Script *this, QiString *path) {
	real_script_load_func(this, path);
	
	const char *path_string = path->data ? path->data : path->cached;
	lua_State *script = *this->script->state;
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Loading script %s...", path_string);
	
	if (!strcmp(path_string, "menu/main.lua")) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Injecting functions into %s", path_string);
		
		luaL_openlibs(script);
		knEnableLog(script);
		knEnablePeekPoke(script);
		knEnableHttp(script);
		knEnableSystem(script);
		knEnableRegistry(script);
		knEnableDatabase(script);
		knEnableFile(script);
		knEnableGamectl(script);
		knEnableOverlay(script);
	}
}

void KNInitLua(struct android_app *app, Leaf *leaf) {
	// Install the lua extensions, but only for menu and hud scripts
	KNHookFunction(KNGetSymbolAddr("_ZN6Script4loadERK8QiString"), &script_load_hook, (void *) &real_script_load_func);
	
	// Set internal and external data paths
	gAndroidInternalDataPath = strdup(app->activity->internalDataPath);
}
#endif
