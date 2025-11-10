/**
 * Handle loading the target game (using Leaf), built-in modules and extensions.
 * 
 * -----------------------------------------------------------------------------
 * 
 * This file is part of KnShim. Copyright (c) 2025 Knot126.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <android_native_app_glue.h>
#include <android/log.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

#define LEAF_IMPLEMENTATION
#include "extern/leaf.h"
#undef LEAF_IMPLEMENTATION

#include "util.h"

#include "loader.h"

/* Shim-wide globals. They are kept here since they are used here most. */
struct android_app *gApp;
Leaf *gLeaf;
Game **gGamePtr;
void *gLibAndroid;
void *gLibC;
char *gGameName;

/* Early init, init, and release */
const char *KnShim_EarlyInit(void) {
	/**
	 * Initialise some core stuff the shim needs. This happens *before* Smash
	 * Hit is loaded.
	 */
	
	// dynamically load libandroid.so for functions that might not be available
	// in older api levels and thus cannot be statically linked if we want to
	// keep running on these older versions.
	gLibAndroid = dlopen("libandroid.so", RTLD_NOW | RTLD_GLOBAL);
	
	if (!gLibAndroid) {
		return "Loading libandroid.so failed";
	}
	
	// same goes for libc
	gLibC = dlopen("libc.so", RTLD_NOW | RTLD_GLOBAL);
	
	if (!gLibC) {
		return "Loading libc.so failed";
	}
	
	return NULL;
}

const char *KnShim_Init(void) {
	gGamePtr = KNGetSymbolAddr("gGame");
	atexit(&KnShim_Release);
	return NULL;
}

void KnShim_Release(void) {
	LeafFree(gLeaf);
}

/* Game loading */
static inline AAsset *KnShim_LoadMainSharedObject(const char *path, const void **data, size_t *length) {
	AAssetManager *asset_manager = gApp->activity->assetManager;
	
	AAsset *asset = AAssetManager_open(asset_manager, path, AASSET_MODE_BUFFER);
	
	if (!asset) {
		return NULL;
	}
	
	length[0] = AAsset_getLength(asset);
	data[0] = AAsset_getBuffer(asset);
	
	return asset;
}

static inline char *KnShim_FindGameObject(void) {
	/**
	 * Find any single shared object in the native/<current-arch> path and 
	 * return the path to it. This is a fairly clean way to load the game .so
	 * without needing to know the exact game name ahead of time. The returned
	 * string must be freed!
	 */
	
	AAssetDir *natives = AAssetManager_openDir(gApp->activity->assetManager, "native/" KN_ARCH_STRING);
	const char *filename_am = AAssetDir_getNextFileName(natives);
	char *filename = NULL;
	if (filename_am) {
		filename = strdup(filename_am);
	}
	AAssetDir_close(natives);
	return filename;
}

static inline char *KnShim_NameOfGameFromObjectPath(const char *path) {
	/**
	 * Parse out the name of the game from the given object path. Returned
	 * string should be freed (unless you rely on the OS to do that after
	 * closing).
	 */
	
	const char *slashlib = strstr(path, "/lib");
	
	if (!slashlib) {
		return NULL;
	}
	
	slashlib += 4; // skip "/lib"
	
	const char *end = strstr(path, ".so");
	
	if (!end) {
		return NULL;
	}
	
	return strndup(slashlib, end - slashlib);
}

const char *KnShim_LoadGame(void) {
	/**
	 * Find and load the main game binary
	 */
	
	// Create an instance of Leaf for loading the main binary
	gLeaf = LeafInit();
	
	if (!gLeaf) {
		return "Leaf init failed";
	}
	
	// Find shared object path for this game
	char *so_path = KnShim_FindGameObject();
	
	if (!so_path) {
		return "Could not find any shared object for the current archiecture";
	}
	
	// Find name of game
	gGameName = KnShim_NameOfGameFromObjectPath(so_path);
	
	if (!gGameName) {
		return "Could not parse game name from object path";
	}
	
	LogI("Found game object: %s (name: %s)", so_path, gGameName);
	
	// Load the contents of the game's library
	const void *data;
	size_t length;
	AAsset *asset = KnShim_LoadMainSharedObject(so_path, &data, &length);
	
	if (!asset) {
		return "Failed to load game shared object from shim native dir";
	}
	
	// Load from the buffer we just read
	const char *error = LeafLoadFromBuffer(gLeaf, (void *) data, length);
	
	if (error) {
		LogF("Leaf loading elf failed: %s", error);
		return "Leaf loading elf failed";
	}
	
	// Close asset handle, its not needed anymore
	AAsset_close(asset);
	free(so_path);
	
	return NULL;
}

/* Modules */
const char *KnShim_Init(void);
const char *KNInitLua(void);
const char *KNDatabaseInit(void);
const char *KNAntitamperInit(void);
const char *KNOverlayInit(void);

ModuleInitFunc gModuleInitFuncs[] = {
	KnShim_Init,
	KNInitLua,
	KNDatabaseInit,
	KNAntitamperInit,
	KNOverlayInit,
	NULL,
};

const char *KnShim_LoadMods(void) {
	for (size_t i = 0; gModuleInitFuncs[i] != NULL; i++) {
		const char *status = (gModuleInitFuncs[i])();
		
		if (status) {
			LogE("KnShim module at index %zu failed to load: %s", i, status);
			abort();
		}
	}
	
	return NULL;
}
