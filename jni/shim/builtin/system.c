#include <android_native_app_glue.h>
#include <android/log.h>

#include "lua_utils.h"
#include "../version.h"
#include "../util.h"

char *gAndroidInternalDataPath;
char *gAndroidExternalDataPath;

int knSystemAbi(lua_State *script) {
	/**
	 * abi = knSystemAbi()
	 * 
	 * Return the system ABI/CPU architecture as a string.
	 * 
	 * * ARMv7 is "armeabi-v7a"
	 * * ARMv8 is "arm64-v8a"
	 * * x86 is "x86"
	 * * Anything else returns "unknown"
	 */
	
	lua_pushstring(script, KN_ARCH_STRING);
	
	return 1;
}

int knGetShimVersion(lua_State *script) {
	lua_pushstring(script, SHIM_VERSION);
	return 1;
}

int knGetAppVersion(lua_State *script) {
	char buf[256];
	
	if (KNGetAppVersion(buf, sizeof buf)) {
		lua_pushstring(script, buf);
	}
	else {
		lua_pushnil(script);
	}
	
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
		int lerror = (luaL_loadstring(script, data) || lua_pcall(script, 0, LUA_MULTRET, 0));
		
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
	knRegisterFunc(script, knSystemAbi);
	knRegisterFunc(script, knGetShimVersion);
	knRegisterFunc(script, knGetAppVersion);
	knRegisterFunc(script, knGetAppSdk);
	knRegisterFunc(script, knGetDeviceSdk);
	knRegisterFunc(script, knGetInternalDataPath);
	knRegisterFunc(script, knGetExternalDataPath);
	knRegisterFunc(script, knInclude);
	
	return 0;
}
