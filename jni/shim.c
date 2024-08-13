#include <android_native_app_glue.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#define TAG "smashshim"

void *gLibsmashhitHandle;
char *gAndroidInternalDataPath;
char *gAndroidExternalDataPath;

typedef void (*AndroidMainFunc)(struct android_app *app);

int unprotect_memory(void *addr, size_t length) {
    size_t page_size = getpagesize();
    size_t align_diff = (size_t)addr % page_size; 
    addr -= align_diff;
    length += align_diff;
    
    __android_log_print(ANDROID_LOG_INFO, TAG, "unprotecting 0x%zx bytes at <%p> (aligned to %zu)", length, addr, page_size);
    
    int result = mprotect(addr, length, PROT_READ | PROT_WRITE | PROT_EXEC);
    
    if (result) {
        __android_log_print(ANDROID_LOG_FATAL, TAG, "mprotect() failed: status %d: %d %s", result, errno, strerror(errno));
    }
    
    return result;
}

int knEnableLog(lua_State *script);
int knEnablePeekPoke(lua_State *script);
int knEnableHttp(lua_State *script);
int knEnableSystem(lua_State *script);
int knEnableRegistry(lua_State *script);

int load_lua_libs(lua_State *script) {
    __android_log_write(ANDROID_LOG_INFO, TAG, "Hello from lua! Opening libs..");
    
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
    
    return 0;
}

void android_main(struct android_app *app) {
    void *handle;
    AndroidMainFunc func = NULL;
    
    handle = dlopen("libsmashhit.so", RTLD_NOW | RTLD_GLOBAL);
    
    if (!handle) {
        __android_log_write(ANDROID_LOG_FATAL, TAG, "Shim couldn't load smash hit lib, fuck !");
        return;
    }
    
    func = (AndroidMainFunc) dlsym(handle, "android_main");
    
    if (!handle) {
        __android_log_write(ANDROID_LOG_FATAL, TAG, "Loaded smash hit lib but didn't find android_main");
        return;
    }
    
    // By some luck ARM32 and ARM64 only differ by the pointer size here - the
    // lua_openlibs reg table is the same offset from this symbol aside from that!
    luaL_Reg *lua_reg_table = (luaL_Reg *) (dlsym(handle, "_ZTV17QiFileInputStream") + 6 * sizeof(void *));
    
    __android_log_print(ANDROID_LOG_INFO, TAG, "Lua register funcs probably at <%p>", lua_reg_table);
    
    if (!unprotect_memory(lua_reg_table, 64)) {
        // smash hit always loads it's own luaopen_base first
        // regardless of what's in the array so we actually load second
        // and avoid loading the base lib.
        lua_reg_table[1].name = "";
        lua_reg_table[1].func = load_lua_libs;
        lua_reg_table[2].name = NULL;
        lua_reg_table[2].func = NULL;

        // Set gLibsmashhitHandle
        gLibsmashhitHandle = handle;
        
        // Set internal and external data paths
        gAndroidInternalDataPath = strdup(app->activity->internalDataPath);
        gAndroidExternalDataPath = strdup(app->activity->externalDataPath);
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "Calling android_main at <0x%p> with app at <0x%p>", (void*)func, (void*)app);
    
    func(app);
    
    if (gAndroidInternalDataPath) { free(gAndroidInternalDataPath); }
    if (gAndroidExternalDataPath) { free(gAndroidExternalDataPath); }
}
