#include <android_native_app_glue.h>
#include <android/log.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "build_date.h"

#include "util.h"

char *gAndroidInternalDataPath;
char *gAndroidExternalDataPath;

int knGetShimVersion(lua_State *script) {
	lua_pushinteger(script, SHIM_BUILD_DATE);
	return 1;
}

int knGetAppSdk(lua_State *script) {
	lua_pushinteger(script, KNGetAppSDK());
	return 1;
}

int knGetDeviceSdk(lua_State *script) {
	lua_pushinteger(script, KNGetDeviceSDK());
	return 1;
}

int knGetInternalDataPath(lua_State *script) {
	lua_pushstring(script, gAndroidInternalDataPath);
	return 1;
}

int knGetExternalDataPath(lua_State *script) {
	lua_pushstring(script, gAndroidExternalDataPath);
	return 1;
}

int knInclude(lua_State *script) {
	/**
	 * Include a lua script from the APK assets directory.
	 */
	
	const char *path = lua_tostring(script, 1);
	char *data = NULL;
	
	if (!path) {
		return luaL_error(script, "path is null or not a string");
	}
	
	bool success = KNLoadAsset(path, (void**)&data, NULL);
	
	if (success) {
		int lerror = luaL_dostring(script, data);
		
		free(data);
		
		// If there is an error present within dostring, it will have been
		// pushed to the top of the stack, so just throw it after freeing our
		// buffer.
		if (lerror != 0) {
			return lua_error(script);
		}
	}
	else {
		return luaL_error(script, "failed to load script asset %s", path);
	}
	
	return lua_gettop(script) - 1;
}

int knEnableSystem(lua_State *script) {
	knRegisterFunc(script, knGetShimVersion);
	knRegisterFunc(script, knGetAppSdk);
	knRegisterFunc(script, knGetDeviceSdk);
	knRegisterFunc(script, knGetInternalDataPath);
	knRegisterFunc(script, knGetExternalDataPath);
	knRegisterFunc(script, knInclude);
	
	return 0;
}
