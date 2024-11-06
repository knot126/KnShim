#include <android_native_app_glue.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifdef USE_LEAF
#define LEAF_IMPLEMENTATION
#include "../../Leaf/andrleaf.h"
Leaf *gLeaf;
void *gLibsmashhitData;
size_t gLibsmashhitLength;
#endif

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

// TODO Move this stuff around
#ifndef USE_LEAF
void *gLibsmashhitHandle;
#endif
char *gAndroidInternalDataPath;
char *gAndroidExternalDataPath;

typedef void (*AndroidMainFunc)(struct android_app *app);

int knEnableLog(lua_State *script);
int knEnablePeekPoke(lua_State *script);
int knEnableHttp(lua_State *script);
int knEnableSystem(lua_State *script);
int knEnableRegistry(lua_State *script);
int knEnableFile(lua_State *script);
int knEnableGamectl(lua_State *script);

int load_lua_libs(lua_State *script) {
	__android_log_print(ANDROID_LOG_INFO, TAG, "Hello from lua! Opening libs..");
	
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
	
	// Better file reading and writing
	knEnableFile(script);
	
	// Game control
	knEnableGamectl(script);
	
	return 0;
}

int load_libsmashhit(struct android_app *app) {
	// Read libsmashhit.so
	AAssetManager *asset_manager = app->activity->assetManager;
	
	AAsset *asset = AAssetManager_open(asset_manager, "native/" KN_ARCH_STRING "/libsmashhit.so.mp3", AASSET_MODE_BUFFER);
	
	gLibsmashhitData = AAsset_getBuffer(asset);
	gLibsmashhitLength = AAsset_getLength(asset);
	
	AAsset_close(asset);
	
	return gLibsmashhitData ? 0 : 1;
}

#ifdef USE_LEAF
void android_main(struct android_app *app) {
	// Create an instance of Leaf for loading the main binary
	gLeaf = LeafInit();
	
	if (!gLeaf) {
		__android_log_print(ANDROID_LOG_FATAL, "leafshim", "Leaf init failed");
		return;
	}
	
	// Load the contents of LSH into a buffer
	if (load_libsmashhit(app)) {
		__android_log_print(ANDROID_LOG_FATAL, "leafshim", "Failed to load libsmashhit.so from shim native dir");
		return;
	}
	
	// Load from the buffer we just read
	const char *error = LeafLoadFromBuffer(gLeaf, gLibsmashhitData, gLibsmashhitLength);
	
	if (error) {
		__android_log_print(ANDROID_LOG_FATAL, "leafshim", "Leaf loading elf failed: %s", error);
		return;
	}
	
	AndroidMainFunc func = LeafSymbolAddr(gLeaf, "android_main");
	
	if (!func) {
		__android_log_print(ANDROID_LOG_FATAL, "leafshim", "Could not find android_main");
		return;
	}
	
	func(app);
}
#else
void android_main(struct android_app *app) {
	void *handle;
	AndroidMainFunc func = NULL;
	
	handle = dlopen("libsmashhit.so", RTLD_NOW | RTLD_GLOBAL);
	
	if (!handle) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Shim couldn't load smash hit lib, fuck !");
		return;
	}
	
	func = (AndroidMainFunc) dlsym(handle, "android_main");
	
	if (!handle) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Loaded smash hit lib but didn't find android_main");
		return;
	}
	
	// By some luck ARM32 and ARM64 only differ by the pointer size here - the
	// lua_openlibs reg table is the same offset from this symbol aside from that!
	luaL_Reg *lua_reg_table = (luaL_Reg *) (dlsym(handle, "_ZTV17QiFileInputStream") + 6 * sizeof(void *));
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Lua register funcs probably at <%p>", lua_reg_table);
	
	if (!set_memory_protection(lua_reg_table, 64, KN_MEM_READ_WRITE)) {
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
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Failed to mprotect() memory!");
		return;
	}
	
#ifdef BUILD_CIPHER
	void KNCipherInit(void *libsmashhit);
	KNCipherInit(gLibsmashhitHandle);
#endif
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Calling android_main at <0x%p> with app at <0x%p>", (void*)func, (void*)app);
	
	func(app);
	
	if (gAndroidInternalDataPath) { free(gAndroidInternalDataPath); }
	if (gAndroidExternalDataPath) { free(gAndroidExternalDataPath); }
}
#endif // USE_LEAF
