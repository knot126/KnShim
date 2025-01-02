#include <android_native_app_glue.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

#define KHASHTABLE_IMPLEMENTATION
#include "hashtable.h"

/** Registry **/
KH_Dict *gRegistry;

static KH_Dict *GetReg(void) {
	/**
	 * Get the registry dict, create if it doesn't exist
	 */
	
	if (!gRegistry) {
		gRegistry = KH_CreateDict();
	}
	
	return gRegistry;
}

#define knToString(BASESYM, INDEX) size_t BASESYM ## _size; const char * BASESYM = lua_tolstring(script, INDEX, &BASESYM ## _size);
#define knBufToBlob(BASESYM) KH_CreateBlob((const uint8_t *) BASESYM, BASESYM ## _size)

int knRegSet(lua_State *script) {
	if (lua_gettop(script) < 2) {
		knReturnNil(script);
	}
	
	knToString(key, 1);
	knToString(value, 2);
	
	if (!key || !value) {
		knReturnNil(script);
	}
	
	lua_pushboolean(script, KH_DictSet(GetReg(), knBufToBlob(key), knBufToBlob(value)));
	return 1;
}

int knRegGet(lua_State *script) {
	if (lua_gettop(script) < 1) {
		knReturnNil(script);
	}
	
	knToString(key, 1);
	
	if (!key) {
		knReturnNil(script);
	}
	
	KH_Blob *value = KH_DictGet(GetReg(), knBufToBlob(key));
	
	if (!value) {
		knReturnNil(script);
	}
	
	lua_pushlstring(script, (const char *)value->data, value->length);
	return 1;
}

int knRegHas(lua_State *script) {
	if (lua_gettop(script) < 1) {
		knReturnNil(script);
	}
	
	knToString(key, 1);
	
	if (!key) {
		knReturnNil(script);
	}
	
	lua_pushboolean(script, KH_DictHas(GetReg(), knBufToBlob(key)));
	return 1;
}

int knRegDelete(lua_State *script) {
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	knToString(key, 1);
	
	if (!key) {
		return 0;
	}
	
	KH_DictDelete(GetReg(), knBufToBlob(key));
	
	return 0;
}

int knRegCount(lua_State *script) {
	lua_pushinteger(script, KH_DictLen(GetReg()));
	return 1;
}

int knRegKeys(lua_State *script) {
	// TODO: Smash Hit absolutely HATES using the default table functions, and
	// heap memory will corrupt shortly after trying to use them. Figure out
	// what SH has changed about Lua such that it crashes unless we lookup the
	// symbol, which is slower...
	((void (*)(lua_State *, int, int)) KNGetSymbolAddr("lua_createtable"))(script, 0, 0);
	
	for (size_t i = 0; i < KH_DictLen(GetReg()); i++) {
		lua_pushinteger(script, i + 1);
		
		KH_Blob *blob = KH_DictKeyIter(GetReg(), i);
		
		lua_pushlstring(script, (const char *) blob->data, blob->length);
		((void (*)(lua_State *, int)) KNGetSymbolAddr("lua_settable"))(script, 1);
	}
	
	return 1;
}

int knEnableRegistry(lua_State *script) {
	lua_register(script, "knRegSet", knRegSet);
	lua_register(script, "knRegGet", knRegGet);
	lua_register(script, "knRegHas", knRegHas);
	lua_register(script, "knRegDelete", knRegDelete);
	lua_register(script, "knRegCount", knRegCount);
	lua_register(script, "knRegKeys", knRegKeys);
	return 0;
}

/** Database **/
KH_Dict *gDatabase;
char *gDatabasePath;

#define KN_DATABASE_MAGIC ('K' | ('N' << 8) | ('O' << 16) | ('T' << 24))

static bool WriteInt(FILE *file, uint32_t data) {
	// Return true on error
	return fwrite(&data, sizeof data, 1, file) == 0;
}

static bool WriteData(FILE *file, size_t size, const void *buffer) {
	// Return true on error
	return fwrite(buffer, size, 1, file) == 0;
}

static uint32_t ReadInt(FILE *file) {
	uint32_t data;
	fread(&data, 1, sizeof data, file);
	return data;
}

static void *ReadData(FILE *file, size_t size) {
	void *data = malloc(size);
	
	if (!data) {
		return NULL;
	}
	
	if (fread(data, 1, size, file) != size) {
		free(data);
		return NULL;
	}
	
	return data;
}

static bool SaveDict(KH_Dict *dict, const char *path) {
	// To make sure we don't overwrite a good albeit outdated version with a
	// corrupt version, we first write to a new file, then rename over the old
	// one if successful.
	char *temp_path = malloc(strlen(path) + strlen(".new") + 1);
	
	if (!temp_path) {
		return false;
	}
	
	strcpy(temp_path, path);
	strcat(temp_path, ".new");
	
	// Open temp file for writing
	FILE *file = fopen(temp_path, "wb");
	
	if (!file) {
		free(temp_path);
		return false;
	}
	
	bool error = false;
	
	// Write header
	error |= WriteInt(file, KN_DATABASE_MAGIC);
	size_t length = KH_DictLen(dict);
	error |= WriteInt(file, length);
	
	// Write keys and values
	for (size_t i = 0; i < length; i++) {
		// Key
		KH_Blob *blob = KH_DictKeyIter(dict, i);
		error |= WriteInt(file, blob->length);
		error |= WriteData(file, blob->length, blob->data);
		
		// Value
		blob = KH_DictValueIter(dict, i);
		error |= WriteInt(file, blob->length);
		error |= WriteData(file, blob->length, blob->data);
	}
	
	// Close the file
	fclose(file);
	
	// If no error occured, replace the database file with the new one. Otherwise,
	// just leave things as they are.
	if (!error) {
		int success = rename(temp_path, path);
		free(temp_path);
		return success == 0;
	}
	else {
		free(temp_path);
		return false;
	}
}

static bool LoadDict(KH_Dict *dict, const char *path) {
	FILE *file = fopen(path, "rb");
	
	if (!file) {
		return false;
	}
	
	if (ReadInt(file) != KN_DATABASE_MAGIC) {
		fclose(file);
		return false;
	}
	
	size_t length = ReadInt(file);
	
	for (size_t i = 0; i < length; i++) {
		size_t key_len = ReadInt(file);
		void *key_data = ReadData(file, key_len);
		
		size_t val_len = ReadInt(file);
		void *val_data = ReadData(file, val_len);
		
		KH_DictSet(dict, KH_CreateBlob(key_data, key_len), KH_CreateBlob(val_data, val_len));
		
		free(key_data);
		free(val_data);
	}
	
	fclose(file);
	
	return true;
}

static KH_Dict *GetDB(void) {
	if (!gDatabase) {
		gDatabase = KH_CreateDict();
		
		if (gDatabase) {
			LoadDict(gDatabase, gDatabasePath);
		}
	}
	
	return gDatabase;
}

int knDbSet(lua_State *script) {
	if (lua_gettop(script) < 2) {
		knReturnNil(script);
	}
	
	knToString(key, 1);
	knToString(value, 2);
	
	if (!key || !value) {
		knReturnNil(script);
	}
	
	bool success = KH_DictSet(GetDB(), knBufToBlob(key), knBufToBlob(value));
	
	if (success) {
		success = SaveDict(GetDB(), gDatabasePath);
	}
	
	lua_pushboolean(script, success);
	
	return 1;
}

int knDbGet(lua_State *script) {
	if (lua_gettop(script) < 1) {
		knReturnNil(script);
	}
	
	knToString(key, 1);
	
	if (!key) {
		knReturnNil(script);
	}
	
	KH_Blob *value = KH_DictGet(GetDB(), knBufToBlob(key));
	
	if (!value) {
		knReturnNil(script);
	}
	
	lua_pushlstring(script, (const char *)value->data, value->length);
	return 1;
}

int knDbHas(lua_State *script) {
	if (lua_gettop(script) < 1) {
		knReturnNil(script);
	}
	
	knToString(key, 1);
	
	if (!key) {
		knReturnNil(script);
	}
	
	lua_pushboolean(script, KH_DictHas(GetDB(), knBufToBlob(key)));
	return 1;
}

int knDbDelete(lua_State *script) {
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	knToString(key, 1);
	
	if (!key) {
		return 0;
	}
	
	KH_DictDelete(GetDB(), knBufToBlob(key));
	
	lua_pushboolean(script, SaveDict(GetDB(), gDatabasePath));
	
	return 1;
}

int knEnableDatabase(lua_State *script) {
	knRegisterFunc(script, knDbSet);
	knRegisterFunc(script, knDbGet);
	knRegisterFunc(script, knDbHas);
	knRegisterFunc(script, knDbDelete);
	return 0;
}

void KNDatabaseInit(struct android_app *app, Leaf *leaf) {
	// Init database save path
	const char *internal_path = app->activity->internalDataPath;
	const char *base_path = "database.kn";
	
	gDatabasePath = malloc(strlen(internal_path) + 1 + strlen(base_path) + 1);
	
	strcpy(gDatabasePath, internal_path);
	strcat(gDatabasePath, "/");
	strcat(gDatabasePath, base_path);
}
