/**
 * Load assets from ZIP files.
 */

#ifdef HYPERSPACE

#if __ANDROID_API__ < 23
	#error Overlays require at least android API level 23
#endif

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
bool (*QiFileInputStream_open)(QiFileInputStream *this, char *path);
void (*Game_loadTemplates)(Game *this);
void (*Player_zero)(Player *this);

// Values which should be used for player zeroing
int zrBalls = 25;
int zrStreak = 0;

FILE *KNLoadFromZIPOverlayInternal(mz_zip_archive *archive, const char *path, int *sizeout);

bool file_input_stream_open_hook(QiFileInputStream *this, char *path) {
	/**
	 * Hook which sits between QiFileInputStream::open() calls and tries to open
	 * files from overlays before moving on to the assets directory.
	 */
	
	// Try loading from overlay first
	FILE *fi = KNLoadFromZIPOverlayInternal(gZip, path, &this->length);
	
	if (fi) {
		this->file = fi;
		this->aasset = NULL; // ignored if null
		this->headpos = 0;
		return true;
	}
	
	// Also try loading without .mp3 (this is really needed ig...)
	char path_no_mp3[strlen(path)+1];
	strcpy(path_no_mp3, path);
	path_no_mp3[strlen(path)-4] = '\0';
	
	fi = KNLoadFromZIPOverlayInternal(gZip, path_no_mp3, &this->length);
	
	if (fi) {
		this->file = fi;
		this->aasset = NULL; // ignored if null
		this->headpos = 0;
		return true;
	}
	
	// Try real assets dir if that doesn't work
	return QiFileInputStream_open(this, path);
}

void player_zero_hook(Player *this) {
	/**
	 * Hooks Player::zero() to customise the starting balls and streak.
	 */
	
	Player_zero(this);
	this->balls = zrBalls;
	this->streak = zrStreak;
}

void KNOverlayInit(struct android_app *app, Leaf *leaf) {
	// Needed for reloading templates
	Game_loadTemplates = KNGetSymbolAddr("_ZN4Game13loadTemplatesEv");
	
	// Hook res man load
	KNHookFunction(KNGetSymbolAddr("_ZN17QiFileInputStream4openEPKc"), file_input_stream_open_hook, (void **) &QiFileInputStream_open);
	
	// Hook player zero
	KNHookFunction(KNGetSymbolAddr("_ZN6Player4zeroEv"), player_zero_hook, (void **) &Player_zero);
}

bool mount_overlay(const char *path) {
	/**
	 * Attach an overlay zip so that any files in the zip will be loaded before
	 * ones in the assets directory.
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

bool unmount_overlay(void) {
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

FILE *KNLoadFromZIPOverlayInternal(mz_zip_archive *archive, const char *path, int *sizeout) {
	/**
	 * Load a file from the current overlay, if it exists.
	 * 
	 * TODO: In the future, copy data to the FILE* more efficently.
	 */
	
	if (!archive) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Overlay is not mounted");
		return NULL;
	}
	
	size_t size;
	void *data = mz_zip_reader_extract_file_to_heap(archive, path, &size, 0);
	
	if (!data) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "miniz zip error: %s: %s", path, mz_zip_get_error_string(mz_zip_get_last_error(archive)));
		return NULL;
	}
	else {
		// Open an in-memory stream
		FILE *file = fmemopen(NULL, size, "rb+");
		
		if (!file) {
			free(data);
			return NULL;
		}
		
		// Write data to buffer
		if (fwrite(data, 1, size, file) != size) {
			free(data);
			return NULL;
		}
		
		free(data);
		
		// Seek back to start for reading
		rewind(file);
		
		// Flush any changes to the stream
		// Probably(?) not needed with fmemopen(), but we might use tmpfile() as
		// a fallback in the future when the API level is too low to support
		// fmemopen() or allocation fails.
		fflush(file);
		
		*sizeout = size;
		
		__android_log_print(ANDROID_LOG_INFO, TAG, "loaded from overlay: %s (sz=%d, fp=%p)", path, size, file);
		
		return file;
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
	
	bool status = mount_overlay(path);
	
	lua_pushboolean(script, status);
	
	return 1;
}

int knUnmountOverlay(lua_State *script) {
	lua_pushboolean(script, unmount_overlay());
	
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
