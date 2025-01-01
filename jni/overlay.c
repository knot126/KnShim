/**
 * Load assets from ZIP files.
 */

#ifdef HYPERSPACE
#include <android_native_app_glue.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "extern/miniz.h"

#include "util.h"
#include "smashhit.h"

// Zip reading related
mz_zip_archive *gZip;

// Functions
unsigned char (*ResMan_load)(ResMan *this, QiString *path, QiOutputStream *output);
void (*QiOutputStream_writeBuffer)(QiOutputStream *this, void *buffer, size_t length);
void (*Game_loadTemplates)(Game *this);

bool HSLoadFromOverlay(const char *path, void **buffer, size_t *length);

unsigned char HSResMan_load(ResMan *this, QiString *path, QiOutputStream *output) {
	const char *virt_path = path->data ? path->data : path->cached;
	void *buffer;
	size_t size;
	
	if (HSLoadFromOverlay(virt_path, &buffer, &size)) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Overlay load %s", virt_path);
		QiOutputStream_writeBuffer(output, buffer, size);
		return 1;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Normal resman load %s", virt_path);
		return ResMan_load(this, path, output);
	}
}

void HSOverlayInit(struct android_app *app, Leaf *leaf) {
	// TODO: 32bit
	QiOutputStream_writeBuffer = KNGetSymbolAddr("_ZN14QiOutputStream11writeBufferEPKvm");
	
	// Needed for reloading templates
	Game_loadTemplates = KNGetSymbolAddr("_ZN4Game13loadTemplatesEv");
	
	// Hook res man load
	KNHookFunction(KNGetSymbolAddr("_ZN6ResMan4loadERK8QiStringR14QiOutputStream"), HSResMan_load, (void **) &ResMan_load);
}

bool HSOverlayMount(const char *path) {
	/**
	 * Attach an overlay zip so that any files in the zip will be loaded before
	 * ones in the assets directory.
	 * 
	 * This also preforms some helpful fixups (e.g. reloading templates)
	 */
	
	if (gZip) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Overlay is already mounted");
		return false;
	}
	
	// Alloc zip archive reader
	gZip = malloc(sizeof *gZip);
	
	if (!gZip) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Could not allocate for zip reader");
		return false;
	}
	
	mz_zip_zero_struct(gZip);
	
	// Init zip reader
	if (!mz_zip_reader_init_file(gZip, path, 0)) {
		return false;
	}
	
	return true;
}

bool HSOverlayUnmount(void) {
	/**
	 * Un-attach an overlay
	 */
	
	if (!gZip) {
		return false;
	}
	
	mz_zip_reader_end(gZip);
	free(gZip);
	gZip = NULL;
	
	return true;
}

bool HSLoadFromOverlay(const char *path, void **buffer, size_t *length) {
	/**
	 * Load a file from the current overlay, if it exists.
	 */
	
	if (!gZip) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Overlay is not mounted");
		return false;
	}
	
	size_t size;
	void *data = mz_zip_reader_extract_file_to_heap(gZip, path, &size, 0);
	
	if (!data) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "mz error: %s", mz_zip_get_error_string(mz_zip_get_last_error(gZip)));
		return false;
	}
	else {
		*buffer = data;
		*length = size;
		return true;
	}
}

int knMountOverlay(lua_State *script) {
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *path = lua_tostring(script, 1);
	
	if (!path) {
		return 0;
	}
	
	bool status = HSOverlayMount(path);
	
	lua_pushboolean(script, status);
	
	return 1;
}

int knUnmountOverlay(lua_State *script) {
	lua_pushboolean(script, HSOverlayUnmount());
	
	return 1;
}

int HSEnableOverlay(lua_State *script) {
	knRegisterFunc(script, knMountOverlay);
	knRegisterFunc(script, knUnmountOverlay);
	
	return 0;
}
#endif // HYPERSPACE
