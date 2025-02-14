#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

int knWriteFile(lua_State *script) {
	/**
	 * (nil|boolean) success = knWriteFile((string) path, (string) contents)
	 * 
	 * Write a files contents to the given path. Returns either nil or false on
	 * failure or true on success.
	 */
	
	if (lua_gettop(script) < 2) {
		return 0;
	}
	
	const char *path = lua_tostring(script, 1);
	size_t size;
	const char *data = lua_tolstring(script, 2, &size);
	
	if (!path || !data) {
		return 0;
	}
	
	FILE *file = fopen(path, "wb");
	
	if (!file) {
		return 0;
	}
	
	size_t written = fwrite(data, 1, size, file);
	
	fclose(file);
	
	lua_pushboolean(script, written == size);
	return 1;
}

int knReadFile(lua_State *script) {
	/**
	 * (string|nil) contents = knReadFile((string) path)
	 * 
	 * Read the file at path, return nil if failed or a string with the contents
	 * of the file if not.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *path = lua_tostring(script, 1);
	
	if (!path) {
		return 0;
	}
	
	FILE *file = fopen(path, "rb");
	
	if (!file) {
		return 0;
	}
	
	// Return codes? Who cares :P
	// Reads the size of the file since there's no way to do this normally
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	// Empty strings can take a shortcut, also bypasses the behaviour of malloc
	// not really being defined.
	if (size == 0) {
		lua_pushliteral(script, "");
		fclose(file);
		return 1;
	}
	
	// Allocate memory for contents
	char *data = malloc(size);
	
	if (!data) {
		fclose(file);
		return 0;
	}
	
	// Read contents into memory
	size_t readen = fread(data, 1, size, file);
	fclose(file); // close the file
	
	// Push it as lua data
	if (readen == size) {
		lua_pushlstring(script, data, size);
	}
	else {
		lua_pushnil(script);
	}
	
	free(data); // always free the data
	
	return 1;
}

int knRenameFile(lua_State *script) {
	/**
	 * (bool) success = knRenameFile((string) old_path, (string) new_path)
	 * 
	 * Rename the file at old_path to new_path. Returns true on success, or
	 * false on failure.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *old_path = lua_tostring(script, 1);
	const char *new_path = lua_tostring(script, 2);
	
	if (!old_path || !new_path) {
		return 0;
	}
	
	int status = rename(old_path, new_path);
	
	lua_pushboolean(script, status == 0);
	
	return 1;
}

int knDeleteFile(lua_State *script) {
	/**
	 * (bool) success = knDeleteFile((string) path)
	 * 
	 * Delete a file at the given path, returning true if successful or false
	 * if not.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *path = lua_tostring(script, 1);
	
	if (!path) {
		return 0;
	}
	
	int status = remove(path);
	
	lua_pushboolean(script, status == 0);
	
	return 1;
}

int knIsFile(lua_State *script) {
	/**
	 * (bool) is_file = knIsFile((string) path)
	 * 
	 * Check if the file at path can be opened in read mode, e.g. that the path
	 * exists, is a file and the user has permission to read it.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *path = lua_tostring(script, 1);
	
	if (!path) {
		return 0;
	}
	
	FILE *f = fopen(path, "rb");
	
	lua_pushboolean(script, f != 0);
	
	if (f) {
		fclose(f);
	}
	
	return 1;
}

int knMakeDir(lua_State *script) {
	/**
	 * (bool) success = knMakeDir((string) path)
	 * 
	 * Create a directory.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *dirname = lua_tostring(script, 1);
	
	if (!dirname) {
		lua_pushboolean(script, 0);
		return 1;
	}
	
	int status = mkdir(dirname, 0777);
	
	lua_pushboolean(script, status == 0);
	
	return 1;
}

int knListDir(lua_State *script) {
	/**
	 * (table|nil) entries = knListDir((string) path)
	 */
	
	const char *dirname = lua_tostring(script, 1);
	
	if (!dirname) {
		lua_pushnil(script);
		return 1;
	}
	
	DIR *dir = opendir(dirname);
	
	if (!dir) {
		lua_pushnil(script);
		return 1;
	}
	
	void (*sh_lua_createtable)(lua_State *, int, int) = KNGetSymbolAddr("lua_createtable");
	void (*sh_lua_settable)(lua_State *, int) = KNGetSymbolAddr("lua_settable");
	
	sh_lua_createtable(script, 0, 0);
	
	size_t i = 1;
	struct dirent *ent;
	
	while ((ent = readdir(dir))) {
		if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
			lua_pushinteger(script, i++);
			lua_pushstring(script, ent->d_name);
			sh_lua_settable(script, 1);
		}
	}
	
	closedir(dir);
	
	return 1;
}

int knIsDir(lua_State *script) {
	/**
	 * (bool) is_dir = knIsDir((string) path)
	 * 
	 * Check if the node at the given path is a directory and can be read.
	 */
	
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	const char *dirname = lua_tostring(script, 1);
	
	if (!dirname) {
		lua_pushboolean(script, 0);
		return 1;
	}
	
	DIR *dir = opendir(dirname);
	
	if (dir) {
		closedir(dir);
		lua_pushboolean(script, 1);
	}
	else {
		lua_pushboolean(script, 0);
	}
	
	return 1;
}

int knLoadAsset(lua_State *script) {
	/**
	 * (string|nil) content = knLoadAsset((string) path)
	 * 
	 * Load the contents of an asset from the APK. For now, this doesn't use
	 * Smash Hit's asset manager so things like auto-decompressing .gz.mp3 files
	 * won't work, though for convience checking for the name with `.mp3`
	 * appended is supported.
	 * 
	 * If loading succedes, a string containing the contents of the asset is
	 * returned. If asset loading fails, it returns nil.
	 */
	
	const char *path = lua_tostring(script, 1);
	char *data = NULL;
	size_t length = 0;
	
	if (!path) {
		return luaL_error(script, "path is null or not a string");
	}
	
	bool success = KNLoadAsset(path, (void **) &data, &length);
	
	if (success) {
		lua_pushlstring(script, data, length);
		free(data);
	}
	else {
		lua_pushnil(script);
	}
	
	return 1;
}

int knEnableFile(lua_State *script) {
	// Files
	knRegisterFunc(script, knWriteFile);
	knRegisterFunc(script, knReadFile);
	knRegisterFunc(script, knRenameFile);
	knRegisterFunc(script, knDeleteFile);
	knRegisterFunc(script, knIsFile);
	
	// Directories
	knRegisterFunc(script, knMakeDir);
	knRegisterFunc(script, knListDir);
	knRegisterFunc(script, knIsDir);
	
	// Assets
	knRegisterFunc(script, knLoadAsset);
	
	return 0;
}
