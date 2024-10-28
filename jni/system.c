#include <android_native_app_glue.h>
#include <android/log.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

extern char *gAndroidInternalDataPath;
extern char *gAndroidExternalDataPath;

int knGetShimVersion(lua_State *script) {
    lua_pushinteger(script, 6);
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

int knEnableSystem(lua_State *script) {
    lua_register(script, "knGetShimVersion", knGetShimVersion);
    lua_register(script, "knGetInternalDataPath", knGetInternalDataPath);
    lua_register(script, "knGetExternalDataPath", knGetExternalDataPath);
    
    return 0;
}
