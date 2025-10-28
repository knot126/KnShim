#include <android_native_app_glue.h>
#include <android/log.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#ifdef GRANNY
#warning "Granny smith builds are not reliable yet!"
#endif

#define LEAF_IMPLEMENTATION
#include "extern/leaf.h"
#undef LEAF_IMPLEMENTATION

#include "util.h"

typedef void (*AndroidMainFunc)(struct android_app *app);

const char *KNInitCore(void);
const char *KNInitLua(void);
const char *KNDatabaseInit(void);
const char *KNAntitamperInit(void);
const char *KNOverlayInit(void);

ModuleInitFunc gModuleInitFuncs[] = {
	KNInitCore,
	KNInitLua,
	KNDatabaseInit,
	KNAntitamperInit,
#ifndef GRANNY
	KNOverlayInit,
#endif
	NULL,
};

static AAsset *load_main_shared_object(struct android_app *app, const void **data, size_t *length) {
	// Read libsmashhit.so
	AAssetManager *asset_manager = app->activity->assetManager;
	
	AAsset *asset = AAssetManager_open(asset_manager, "native/" KN_ARCH_STRING "/lib" KN_GAME_STRING ".so.mp3", AASSET_MODE_BUFFER);
	
	if (!asset) {
		return NULL;
	}
	
	length[0] = AAsset_getLength(asset);
	data[0] = AAsset_getBuffer(asset);
	
	return asset;
}

bool KNInitEarlyCore(void);

#include "version.h"

void android_main(struct android_app *app) {
	// Set gApp to android app structure
	gApp = app;
	
	if (!KNInitEarlyCore()) {
		LogF("Early shim init failed");
		return;
	}
	else {
		LogI("KnShim r%s (%s for %s); App SDK %d, Device SDK %d", SHIM_VERSION, KN_GAME_STRING, KN_ARCH_STRING, KNGetAppSDK(), KNGetDeviceSDK());
	}
	
	// Create an instance of Leaf for loading the main binary
	gLeaf = LeafInit();
	
	if (!gLeaf) {
		LogF("Leaf init failed");
		return;
	}
	else {
		LogI("Leaf inited");
	}
	
	// Load the contents of LSH
	const void *data;
	size_t length;
	AAsset *asset = load_main_shared_object(app, &data, &length);
	
	if (!asset) {
		LogF("Failed to load game shared object from shim native dir");
		return;
	}
	else {
		LogI("Loaded game shared object from native directory");
	}
	
	// Load from the buffer we just read
	const char *error = LeafLoadFromBuffer(gLeaf, (void *) data, length);
	
	if (error) {
		LogF("Leaf loading elf failed: %s", error);
		return;
	}
	else {
		LogI("Loading elf succeeded");
	}
	
	// Close asset handle, not needed anymore
	AAsset_close(asset);
	
	// Install modules
	for (size_t i = 0; gModuleInitFuncs[i] != NULL; i++) {
		const char *status = (gModuleInitFuncs[i])();
		
		if (status) {
			LogE("KnShim module at index %zu failed to load: %s", i, status);
			return;
		}
	}
	
	// Get main func and call
	AndroidMainFunc func = LeafSymbolAddr(gLeaf, "android_main");
	
	if (!func) {
		LogF("Could not find android_main()");
		return;
	}
	else {
		LogI("Found android_main() at %p", func);
	}
	
	func(app);
}
