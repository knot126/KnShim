/**
 * Load assets from ZIP files.
 */

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

#ifndef NEW_OVERLAYS
// Zip reading related
mz_zip_archive *gZip;

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
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Overlay initialised: %s", path);
	
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

static size_t KNOverlayWriteFileCallback(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n) {
	return fwrite(pBuf, 1, n, (FILE *) pOpaque);
}

FILE *KNExtractOverlayToTempfile(mz_zip_archive *archive, const char *path, size_t *size_out) {
	/**
	 * Extract a file from the current overlay to a tempfile and return the file
	 * handle.
	 */
	
	int fileIndex = mz_zip_reader_locate_file(archive, path, NULL, MZ_ZIP_FLAG_CASE_SENSITIVE);
	
	if (fileIndex == -1) {
		return NULL;
	}
	
	mz_zip_archive_file_stat stat;
	
	if (!mz_zip_reader_file_stat(archive, fileIndex, &stat)) {
		return NULL;
	}
	
	*size_out = stat.m_uncomp_size;
	
	// TODO: Perhaps we should switch to using memfd_create()? fmemopen() just
	// doesn't work, but perhaps this would since it creates a real file
	// descriptor.
	// SEE: https://www.man7.org/linux/man-pages/man2/memfd_create.2.html
	FILE *file = tmpfile();
	
	if (!file) {
		return NULL;
	}
	
	// Extract data
	if (!mz_zip_reader_extract_to_callback(archive, fileIndex, KNOverlayWriteFileCallback, file, 0)) {
		fclose(file);
		return NULL;
	}
	
	fflush(file);
	rewind(file);
	
	return file;
}

bool KNOverlayLoad(QiFileInputStream *this, const char *path) {
	size_t size = 0;
	
	FILE *fi = KNExtractOverlayToTempfile(gZip, path, &size);
	
	if (fi) {
		this->file = fi;
		this->size = size;
		this->position = 0;
		this->androidAsset = NULL; // ignored if null
		memset(&this->path, 0, sizeof this->path);
	}
	
	return !!fi;
}
#else
#include <sys/stat.h>

/**
 * A single, abstract overlay. The actual backing implementation may be a
 * physical directory, archive file, script callback or even something else but
 * the basic interface is still the same.
 */
struct Overlay;

typedef bool (*OverlayExistsFunc)(struct Overlay *this, const char *path);
typedef bool (*OverlayLoadFunc)(struct Overlay *this, const char *path, FILE *file);
typedef void (*OverlayReleaseFunc)(struct Overlay *this);

struct Overlay {
	void *context;
	OverlayExistsFunc exists;
	OverlayLoadFunc load;
	OverlayReleaseFunc release;
}

bool OverlayExists(Overlay *this, const char *path) {
	/**
	 * Check if this overlay has a file at the given path.
	 */
	
	return this->exists(this, path);
}

FILE *OverlayLoad(Overlay *this, const char *path, size_t *size) {
	/**
	 * Load a file from the overlay into a temporary file. This also takes care
	 * of things like opening the file in the first place, getting the length,
	 * and calling rewind on it once it's been loaded.
	 */
	
	FILE *file = tmpfile();
	
	if (!this->load(this, path, file)) {
		fclose(file);
		return NULL;
	}
	
	fflush(file);
	rewind(file);
	
	// Getting file length, the POSIX(tm) way.(tm)
	int fd = fileno(file);
	struct stat file_stat;
	
	if (fstat(fd, &file_stat)) {
		fclose(file);
		return NULL;
	}
	
	*size = file_stat->st_size;
	
	return file;
}

void OverlayRelease(Overlay *this) {
	/**
	 * Release all resources related to an overlay.
	 */
	
	this->release(this);
	free(this);
}

struct OverlayManager {
	Overlay **overlay;
	size_t count;
};

inline static void *xrealloc(void *block, size_t size) {
	if (size == 0) { free(block); return NULL; }
	else { return realloc(block, size); }
}

bool OverlayManagerPush(OverlayManager *this, Overlay *overlay) {
	/**
	 * Push a new overlay on top of the stack.
	 */
	
	this->count++;
	Overlay *overlay_stack = xrealloc(this->overlay, sizeof *this->overlay * this->count);
	
	if (!overlay_stack) {
		return false;
	}
	else {
		this->overlay = overlay_stack;
		return true;
	}
}

void OverlayManagerPop(OverlayManager *this, Overlay *overlay) {
	/**
	 * Pop an overlay from the top of the stack, releasing it.
	 */
	
	this->count--;
	OverlayRelease(this->overlay[this->count]);
}

FILE *OverlayManagerLoad(OverlayManager *this, const char *path, size_t *size) {
	/**
	 * Load a file from the first overlay that contains it. Overlays are
	 * searched top down (so newer overlays take precedence over old ones).
	 */
	
	for (size_t i = this->count; i != 0; i--) {
		Overlay *overlay = this->overlay[i-1];
		
		if (OverlayExists(overlay, path)) {
			return OverlayLoad(overlay, path, size);
		}
	}
	
	return NULL;
}
#endif

/**
 * ============================================================================
 * Lua functions
 * ============================================================================
 */

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

int knEnableOverlay(lua_State *script) {
	knRegisterFunc(script, knMountOverlay);
	knRegisterFunc(script, knUnmountOverlay);
	
	return 0;
}

/**
 * ============================================================================
 * Boilerplate overlay init and hooking code
 * ============================================================================
 */
bool (*QiFileInputStream_open)(QiFileInputStream *this, char *path);

bool QiFileInputStream_open_hook(QiFileInputStream *this, char *path) {
	/**
	 * Hook which sits between QiFileInputStream::open() calls and tries to open
	 * files from overlays before moving on to the assets directory.
	 */
	
	// Try loading from overlay first
	if (gZip) {
		if (KNOverlayLoad(this, path)) {
			return true;
		}
		
		// Also try loading without .mp3 (this is really needed ig...)
		char path_no_mp3[strlen(path)+1];
		strcpy(path_no_mp3, path);
		path_no_mp3[strlen(path)-4] = '\0';
		
		if (KNOverlayLoad(this, path_no_mp3)) {
			return true;
		}
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
