/**
 * Load assets from ZIP files.
 */

#include <android_native_app_glue.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "extern/miniz.h"

#include "util.h"
#include "smashhit.h"

/**
 * A single, abstract overlay. The actual backing implementation may be a
 * physical directory, archive file, script callback or even something else but
 * the basic interface is still the same.
 */
struct Overlay;

typedef FILE *(*OverlayLoadFunc)(struct Overlay *this, const char *path);
typedef void (*OverlayReleaseFunc)(struct Overlay *this);

typedef struct Overlay {
	void *context;
	OverlayLoadFunc load;
	OverlayReleaseFunc release;
} Overlay;

FILE *OverlayLoad(Overlay *this, const char *path, size_t *size) {
	/**
	 * Load a file from the overlay into a temporary file. This also takes care
	 * of things like opening the file in the first place, getting the length,
	 * and calling rewind on it once it's been loaded.
	 */
	
	FILE *file = this->load(this, path);
	
	if (!file) {
		return NULL;
	}
	
	// Getting file length, the POSIX(tm) way.(tm)
	int fd = fileno(file);
	struct stat file_stat;
	
	if (fstat(fd, &file_stat)) {
		fclose(file);
		return NULL;
	}
	
	*size = file_stat.st_size;
	
	return file;
}

void OverlayRelease(Overlay *this) {
	/**
	 * Release all resources related to an overlay.
	 */
	
	this->release(this);
	free(this);
}

#define OverlayAllocate(T, StateType) Overlay *T = malloc(sizeof *T); \
	if (!T) { return NULL; } \
	T->context = malloc(sizeof(StateType)); \
	if (!T->context) { free(T); return NULL; }

/**
 * Manager to allow mounting multiple overlays at once, in an order.
 */
typedef struct OverlayManager {
	Overlay **overlay;
	size_t count;
} OverlayManager;

bool OverlayManagerPush(OverlayManager *this, Overlay *overlay) {
	/**
	 * Push a new overlay on top of the stack.
	 */
	
	size_t new_count = this->count + 1;
	
	// Important to note: Size can never really reach 0 in this case.
	Overlay **new_stack = realloc(this->overlay, sizeof *this->overlay * new_count);
	
	//LogI("%p = realloc(%p, %u)", new_stack, this->overlay, sizeof *this->overlay * new_count);
	
	if (!new_stack) {
		OverlayRelease(overlay);
		return false;
	}
	else {
		new_stack[new_count-1] = overlay;
		this->overlay = new_stack;
		this->count = new_count;
		return true;
	}
}

void OverlayManagerPop(OverlayManager *this) {
	/**
	 * Pop an overlay from the top of the stack, releasing it.
	 */
	
	if (this->count != 0) {
		this->count--;
		OverlayRelease(this->overlay[this->count]);
	}
}

FILE *OverlayManagerLoad(OverlayManager *this, const char *path, size_t *size) {
	/**
	 * Load a file from the first overlay that contains it. Overlays are
	 * searched top down (so more recently added overlays take precedence over
	 * old ones).
	 */
	
	for (size_t i = this->count; i != 0; i--) {
		Overlay *overlay = this->overlay[i-1];
		
		FILE *file = OverlayLoad(overlay, path, size);
		
		if (file) {
			return file;
		}
	}
	
	return NULL;
}

// Main instance of the overlay manager.
OverlayManager gOverlayMan;

/**
 * Code that acts as a shim between OverlayManagerLoad and QiFileInputStream::load
 */
bool KNOverlayLoad(QiFileInputStream *this, const char *path) {
	size_t size = 0;
	
	FILE *file = OverlayManagerLoad(&gOverlayMan, path, &size);
	
	if (file) {
		this->file = file;
		this->size = size;
		this->position = 0;
		this->androidAsset = NULL; // ignored if null
		memset(&this->path, 0, sizeof this->path);
	}
	
	return !!file;
}

/**
 * Directory type overlays, simple but useful.
 */
#define JoinPaths(Dest, S1, S2) char Dest[strlen(S1) + strlen(S2) + 2]; { strcpy(Dest, S1); strcat(Dest, "/"); strcat(Dest, S2); }

typedef struct DirOverlayState {
	char *directory;
} DirOverlayState;

FILE *DirOverlayLoad(Overlay *this, const char *path) {
	JoinPaths(physical_path, ((DirOverlayState *) this->context)->directory, path);
	return fopen(physical_path, "rb");
}

void DirOverlayRelease(Overlay *this) {
	free(((DirOverlayState *) this->context)->directory);
}

Overlay *DirOverlayCreate(const char *directory) {
	OverlayAllocate(this, DirOverlayState);
	((DirOverlayState *) this->context)->directory = strdup(directory);
	this->load = DirOverlayLoad;
	this->release = DirOverlayRelease;
	return this;
}

#undef JoinPaths

/**
 * ZIP-file based overlays, useful for loading resources in packs.
 */
#define theZip (&(((ZipOverlayState *) this->context)->zip))

typedef struct ZipOverlayState {
	mz_zip_archive zip;
} ZipOverlayState;

static size_t ZipOverlayWriteCallback(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n) {
	return fwrite(pBuf, 1, n, (FILE *) pOpaque);
}

FILE *ZipOverlayLoad(Overlay *this, const char *path) {
	int fileIndex = mz_zip_reader_locate_file(theZip, path, NULL, MZ_ZIP_FLAG_CASE_SENSITIVE);
	
	if (fileIndex == -1) {
		return NULL;
	}
	
	FILE *file = tmpfile();
	
	if (!file) {
		return NULL;
	}
	
	if (!mz_zip_reader_extract_to_callback(theZip, fileIndex, ZipOverlayWriteCallback, file, 0)) {
		fclose(file);
		return NULL;
	}
	
	fflush(file);
	rewind(file);
	
	return file;
}

void ZipOverlayRelease(Overlay *this) {
	mz_zip_reader_end(theZip);
}

Overlay *ZipOverlayCreate(const char *zip_path) {
	OverlayAllocate(this, ZipOverlayState);
	mz_zip_zero_struct(theZip);
	
	if (!mz_zip_reader_init_file(theZip, zip_path, 0)) {
		OverlayRelease(this);
		return NULL;
	}
	
	this->load = ZipOverlayLoad;
	this->release = ZipOverlayRelease;
	
	return this;
}

#undef theZip

/**
 * User defined callbacks - can be used to generate assets dynamically.
 * Currently limited to using the main menu's script since that will always be
 * available.
 */

#define LUA_OVERLAY_FUNCTION_NAME_MAX_CHARS 256

typedef struct LuaOverlayState {
	char function_name[LUA_OVERLAY_FUNCTION_NAME_MAX_CHARS];
} LuaOverlayState;

FILE *LuaOverlayLoad(Overlay *this, const char *path) {
	lua_State *L = *gGame->menuScene->script.state;
	const char *function_name = ((LuaOverlayState *) this->context)->function_name;
	
	lua_getglobal(L, function_name);
	
	if (!lua_isfunction(L, -1)) {
		LogE("Could not load asset %s: %s is not a function", path, function_name);
		lua_pop(L, 1);
		return NULL;
	}
	
	lua_pushstring(L, path);
	
	if (lua_pcall(L, 1, 1, 0) != 0) {
		const char *error_msg = lua_tostring(L, -1);
		LogE("Could not load asset %s backed by function %s: %s", path, function_name, error_msg);
		lua_pop(L, 1);
		return NULL;
	}
	
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return NULL;
	}
	
	if (!lua_isstring(L, -1)) {
		LogE("Could not load asset %s backed by function %s: Did not return a string", path, function_name);
		lua_pop(L, 1);
		return NULL;
	}
	
	size_t size;
	const char *data = lua_tolstring(L, -1, &size);
	
	FILE *file = tmpfile();
	
	if (fwrite(data, 1, size, file) != size) {
		LogE("Could not load asset %s backed by function %s: I/O Error", path, function_name);
		fclose(file);
		lua_pop(L, 1);
		return NULL;
	}
	
	fflush(file);
	rewind(file);
	
	lua_pop(L, 1);
	return file;
}

void LuaOverlayRelease(Overlay *this) {
	/* nop */
}

Overlay *LuaOverlayCreate(const char *function_name) {
	OverlayAllocate(this, LuaOverlayState);
	this->load = LuaOverlayLoad;
	this->release = LuaOverlayRelease;
	strncpy(((LuaOverlayState *) this->context)->function_name, function_name, LUA_OVERLAY_FUNCTION_NAME_MAX_CHARS);
	return this;
}

/**
 * Lua interface to the OverlayManager
 */

int knPushOverlay(lua_State *L) {
	const char *type = lua_tostring(L, 1);
	
	Overlay *overlay = NULL;
	
	if (!strcmp(type, "directory")) {
		overlay = DirOverlayCreate(lua_tostring(L, 2));
	}
	else if (!strcmp(type, "zip")) {
		overlay = ZipOverlayCreate(lua_tostring(L, 2));
	}
	else if (!strcmp(type, "callback")) {
		overlay = LuaOverlayCreate(lua_tostring(L, 2));
	}
	
	if (overlay) {
		lua_pushboolean(L, OverlayManagerPush(&gOverlayMan, overlay));
	}
	else {
		luaL_error(L, "Could not push %s overlay at %s", type, lua_tostring(L, 2));
		return 0;
	}
	
	return 1;
}

int knPopOverlay(lua_State *L) {
	OverlayManagerPop(&gOverlayMan);
	
	return 0;
}

int knEnableOverlay(lua_State *L) {
	knRegisterFunc(L, knPushOverlay);
	knRegisterFunc(L, knPopOverlay);
	
	return 0;
}

/**
 * ============================================================================
 * Boilerplate overlay init and hooking code
 * ============================================================================
 */
bool (*QiFileInputStream_open)(QiFileInputStream *this, const char *path);

bool QiFileInputStream_open_hook(QiFileInputStream *this, const char *path) {
	/**
	 * Hook which sits between QiFileInputStream::open() calls and tries to open
	 * files from overlays before moving on to the assets directory.
	 */
	
	char final_path[strlen(path) + 1];
	strcpy(final_path, path);
	
	// If the file path has the .mp3 suffix, we remove it.
	if (strlen(final_path) >= 4 && !strcmp(final_path + strlen(final_path) - 4, ".mp3")) {
		final_path[strlen(final_path) - 4] = '\0';
	}
	
	LogI("Want to find: %s", final_path);
	
	if (KNOverlayLoad(this, final_path)) {
		LogI("Found: %s", final_path);
		return true;
	}
	
	// Try real assets dir if that doesn't work
	return QiFileInputStream_open(this, path);
}

const char *KNOverlayInit(void) {
	// Hook file input stream open
	if (!QiFileInputStream_open) {
		QiFileInputStream_open = KNHookFunctionByName("_ZN17QiFileInputStream4openEPKc", QiFileInputStream_open_hook, false);
	}
	
	return NULL;
}
