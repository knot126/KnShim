#include <android_native_app_glue.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#if defined(__ARM_ARCH_7A__)
#define LEAF_32BIT
#endif

#define LEAF_IMPLEMENTATION
#include "andrleaf.h"
Leaf *gLeaf;
#undef LEAF_IMPLEMENTATION

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

char *gAndroidInternalDataPath;
char *gAndroidExternalDataPath;

typedef void (*AndroidMainFunc)(struct android_app *app);

int knEnableLog(lua_State *script);
int knEnablePeekPoke(lua_State *script);
int knEnableHttp(lua_State *script);
int knEnableSystem(lua_State *script);
int knEnableRegistry(lua_State *script);
int knEnableDatabase(lua_State *script);
int knEnableFile(lua_State *script);
int knEnableGamectl(lua_State *script);

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

#ifdef BUILD_CIPHER
void KNCipherInit(struct android_app *app, Leaf *leaf);
#endif

void KNDebugLogInit(struct android_app *app, Leaf *leaf);
void KNDatabaseInit(struct android_app *app, Leaf *leaf);

ModuleInitFunc gModuleInitFuncs[] = {
	KNDebugLogInit,
	KNDatabaseInit,
	KNInitLua,
#ifdef BUILD_CIPHER
	KNCipherInit,
#endif
	NULL,
};

AAsset *load_libsmashhit(struct android_app *app, const void **data, size_t *length) {
	// Read libsmashhit.so
	AAssetManager *asset_manager = app->activity->assetManager;
	
	AAsset *asset = AAssetManager_open(asset_manager, "native/" KN_ARCH_STRING "/libsmashhit.so.mp3", AASSET_MODE_BUFFER);
	
	if (!asset) {
		return NULL;
	}
	
	length[0] = AAsset_getLength(asset);
	data[0] = AAsset_getBuffer(asset);
	
	return asset;
}

void android_main(struct android_app *app) {
	// Create an instance of Leaf for loading the main binary
	gLeaf = LeafInit();
	
	if (!gLeaf) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Leaf init failed");
		return;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Leaf initialised");
	}
	
	// Load the contents of LSH
	const void *data;
	size_t length;
	AAsset *asset = load_libsmashhit(app, &data, &length);
	
	if (!asset) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Failed to load libsmashhit.so from shim native dir");
		return;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Loaded libsmashhit.so from native directory");
	}
	
	// Load from the buffer we just read
	const char *error = LeafLoadFromBuffer(gLeaf, (void *) data, length);
	
	if (error) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Leaf loading elf failed: %s", error);
		return;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Loading elf succeeded");
	}
	
	// Close asset handle, not needed anymore
	AAsset_close(asset);
	
	// Install modules
	for (size_t i = 0; gModuleInitFuncs[i] != NULL; i++) {
		(gModuleInitFuncs[i])(app, gLeaf);
	}
	
	// Get main func and call
	AndroidMainFunc func = LeafSymbolAddr(gLeaf, "android_main");
	
	if (!func) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Could not find android_main()");
		return;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Found android_main() at %p", func);
	}
	
	func(app);
}
