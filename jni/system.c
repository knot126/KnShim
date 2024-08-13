#include <android_native_app_glue.h>
#include <android/log.h>
#include <sys/stat.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

extern char *gAndroidInternalDataPath;
extern char *gAndroidExternalDataPath;

int knGetInternalDataPath(lua_State *script) {
    lua_pushstring(script, gAndroidInternalDataPath);
    return 1;
}

int knGetExternalDataPath(lua_State *script) {
    lua_pushstring(script, gAndroidExternalDataPath);
    return 1;
}

int knMakeDir(lua_State *script) {
    if (lua_gettop(script) < 1) {
        lua_pushboolean(script, 0);
        return 1;
    }
    
    const char *dirname = lua_tostring(script, 1);
    
    if (!dirname) {
        lua_pushboolean(script, 0);
        return 1;
    }
    
    int status = mkdir(dirname, 0777);
    
    if (status == 0) {
        lua_pushboolean(script, 1);
    }
    else {
        lua_pushboolean(script, 0);
    }
    
    return 1;
}

int knEnableSystem(lua_State *script) {
    lua_register(script, "knGetInternalDataPath", knGetInternalDataPath);
    lua_register(script, "knGetExternalDataPath", knGetExternalDataPath);
    lua_register(script, "knMakeDir", knMakeDir);
    
    return 0;
}
