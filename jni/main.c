#include <android_native_app_glue.h>
#include <android/log.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define LEAF_IMPLEMENTATION
#include "extern/leaf.h"
#undef LEAF_IMPLEMENTATION

#include "util.h"

typedef void (*AndroidMainFunc)(struct android_app *app);

const char *KNInitLua(void);
const char *KNDatabaseInit(void);

ModuleInitFunc gModuleInitFuncs[] = {
	KNInitLua,
	KNDatabaseInit,
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
	// Set gApp to android app structure
	gApp = app;
	
	if (!KNInit()) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "Early shim init failed");
		return;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Early shim successful, app sdk = %d, device sdk = %d", KNGetAppSDK(), KNGetDeviceSDK());
	}
	
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
		const char *status = (gModuleInitFuncs[i])();
		
		if (status) {
			LogE("KnShim module at index %i failed to load: %s", i, status);
			return;
		}
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
