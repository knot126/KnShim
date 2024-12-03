#include <android_native_app_glue.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#define KHASHTABLE_IMPLEMENTATION
#include "hashtable.h"

KH_Dict *gRegistry;
KH_Dict *gDatabase;

KH_Dict *knGetReg(void) {
	/**
	 * Get the registry dict, create if it doesn't exist
	 */
	
	if (gRegistry) {
		gRegistry = KH_CreateDict();
	}
	
	return gRegistry;
}

#define knToString(BASESYM, INDEX) size_t BASESYM ## _size; const char * BASESYM = lua_tolstring(script, INDEX, &BASESYM ## _size);
#define knBufToBlob(BASESYM) KH_CreateBlob((const uint8_t *) BASESYM, BASESYM ## _size)

int knRegSet(lua_State *script) {
	if (lua_gettop(script) < 2) {
		return 0;
	}
	
	knToString(key, 1);
	knToString(value, 2);
	
	if (!key || !value) {
		return 0;
	}
	
	lua_pushboolean(script, KH_DictSet(knGetReg(), knBufToBlob(key), knBufToBlob(value)));
	return 1;
}

int knRegGet(lua_State *script) {
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	knToString(key, 1);
	
	if (!key) {
		return 0;
	}
	
	KH_Blob *value = KH_DictGet(knGetReg(), knBufToBlob(key));
	
	if (!value) {
		return 0;
	}
	
	lua_pushlstring(script, (const char *)value->data, value->length);
	return 1;
}

int knRegHas(lua_State *script) {
	if (lua_gettop(script) < 1) {
		return 0;
	}
	
	knToString(key, 1);
	
	if (!key) {
		return 0;
	}
	
	lua_pushboolean(script, KH_DictHas(knGetReg(), knBufToBlob(key)));
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
	
	KH_DictDelete(knGetReg(), knBufToBlob(key));
	
	return 0;
}

int knRegCount(lua_State *script) {
	lua_pushinteger(script, KH_DictLen(knGetReg()));
	return 1;
}

int knRegKeys(lua_State *script) {
	lua_newtable(script);
	
	for (size_t i = 0; i < KH_DictLen(knGetReg()); i++) {
		lua_pushinteger(script, i + 1);
		
		KH_Blob *blob = KH_DictKeyIter(knGetReg(), i);
		
		lua_pushlstring(script, (const char *) blob->data, blob->length);
		lua_settable(script, 1);
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
