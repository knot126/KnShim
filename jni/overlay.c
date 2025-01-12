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
void (*Player_zero)(Player *this);

bool KNLoadFromOverlay(const char *path, void **buffer, size_t *length);

unsigned char KNResMan_load(ResMan *this, QiString *path, QiOutputStream *output) {
	const char *virt_path = path->data ? path->data : path->cached;
	void *buffer;
	size_t size;
	
	if (KNLoadFromOverlay(virt_path, &buffer, &size)) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Overlay load %s", virt_path);
		QiOutputStream_writeBuffer(output, buffer, size);
		free(buffer);
		return 1;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Normal resman load %s", virt_path);
		return ResMan_load(this, path, output);
	}
}

int zrBalls = 25;
int zrStreak = 0;

void KNPlayer_zero(Player *this) {
	Player_zero(this);
	this->balls = zrBalls;
	this->streak = zrStreak;
}

void KNOverlayInit(struct android_app *app, Leaf *leaf) {
	#ifdef __aarch64__
	QiOutputStream_writeBuffer = KNGetSymbolAddr("_ZN14QiOutputStream11writeBufferEPKvm");
	#elif defined(__arm__)
	QiOutputStream_writeBuffer = KNGetSymbolAddr("_ZN14QiOutputStream11writeBufferEPKvj");
	#endif
	
	// Needed for reloading templates
	Game_loadTemplates = KNGetSymbolAddr("_ZN4Game13loadTemplatesEv");
	
	// Hook res man load
	KNHookFunction(KNGetSymbolAddr("_ZN6ResMan4loadERK8QiStringR14QiOutputStream"), KNResMan_load, (void **) &ResMan_load);
	
	// Hook player zero
	KNHookFunction(KNGetSymbolAddr("_ZN6Player4zeroEv"), KNPlayer_zero, (void **) &Player_zero);
}

bool KNOverlayMount(const char *path) {
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
		free(gZip);
		gZip = NULL;
		return false;
	}
	
	return true;
}

bool KNOverlayUnmount(void) {
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

bool KNLoadFromOverlay_Raw(const char *path, void **buffer, size_t *length) {
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

bool KNLoadFromOverlay(const char *path, void **buffer, size_t *length) {
	bool status = KNLoadFromOverlay_Raw(path, buffer, length);
	
	if (!status) {
		// Try loading with the mp3 suffix
		char path_mp3[strlen(path) + 5];
		strcpy(path_mp3, path);
		strcat(path_mp3, ".mp3");
		
		status = KNLoadFromOverlay_Raw(path_mp3, buffer, length);
	}
	
	return status;
}

int knMountOverlay(lua_State *script) {
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *path = lua_tostring(script, 1);
	
	if (!path) {
		return 0;
	}
	
	bool status = KNOverlayMount(path);
	
	lua_pushboolean(script, status);
	
	return 1;
}

int knUnmountOverlay(lua_State *script) {
	lua_pushboolean(script, KNOverlayUnmount());
	
	return 1;
}

static inline Game *get_game(void) {
	Game **ppGame = KNGetSymbolAddr("gGame");
	return *ppGame;
}

int knLoadTemplates(lua_State *script) {
	Game *gGame = get_game();
	
	if (gGame) {
		Game_loadTemplates(gGame);
	}
	else {
		__android_log_print(ANDROID_LOG_WARN, TAG, "gGame is null");
	}
	
	return 0;
}

int knSetPlayerZeroState(lua_State *script) {
	zrBalls = lua_tointeger(script, 1);
	zrStreak = lua_tointeger(script, 2);
	
	return 0;
}

int knEnableOverlay(lua_State *script) {
	knRegisterFunc(script, knMountOverlay);
	knRegisterFunc(script, knUnmountOverlay);
	knRegisterFunc(script, knLoadTemplates);
	knRegisterFunc(script, knSetPlayerZeroState);
	
	return 0;
}
#endif // HYPERSPACE
