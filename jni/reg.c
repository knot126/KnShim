#include <android_native_app_glue.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

// KEY-VALUE STORE IMPLEMENTATION
// TODO: Doesn't even use reallocation tricks, I just want a working implementation
// for the moment. I'd like to write a hash table using open addressing and do
// actual optimisation for malloc().
typedef char *KVString;
typedef const char *CKVString;

typedef struct {
    // For this really basic implementation I'm okay with relying on these
    // already being zero initialised by the loader
    KVString *data; // [2 * i + 0] = key, [2 * i + 1] = value. value may be NULL
    size_t count;
} KVStore;

KVStore gKvStore;

static void *KVRealloc(void *block, size_t size) {
    if (!block && size) {
        return malloc(size);
    }
    else if (block && !size) {
        free(block);
        return NULL;
    }
    else {
        return realloc(block, size);
    }
}

#define KV_KEY(STORE, INDEX) STORE->data[2 * (INDEX) + 0]
#define KV_VALUE(STORE, INDEX) STORE->data[2 * (INDEX) + 1]

static int64_t KVIndexOfKey(KVStore *this, CKVString key) {
    if (!this->data || !this->count) {
        return -1;
    }
    
    for (size_t i = 0; i < this->count; i++) {
        if ((KV_KEY(this, i) != NULL) && (!strcmp(KV_KEY(this, i), key))) {
            return i;
        }
    }
    
    return -1;
}

static int KVSet(KVStore *this, CKVString key, CKVString value) {
    /**
     * Set a key regardless if it exists, return 0 on fail, 1 on success
     */
    
    int64_t idx = KVIndexOfKey(this, key);
    
    // Allocate a new slot
    if (idx == -1) {
        this->count++;
        this->data = KVRealloc(this->data, 2 * this->count * sizeof *this->data);
        
        if (!this->data) {
            return 0;
        }
        
        KV_KEY(this, this->count - 1) = strdup(key);
        KV_VALUE(this, this->count - 1) = strdup(value);
        
        if (!KV_KEY(this, this->count - 1) || !KV_VALUE(this, this->count - 1)) {
            return 0;
        }
    }
    // Reuse an existing slot
    else {
        if (KV_VALUE(this, idx)) {
            free(KV_VALUE(this, idx));
        }
        
        KV_VALUE(this, idx) = strdup(value);
        
        if (!KV_VALUE(this, idx)) {
            return 0;
        }
    }
    
    return 1;
}

static KVString KVGet(KVStore *this, CKVString key) {
    /**
     * Get a value, returning the internal string for it. Or NULL if it doesn't exist.
     */
    
    int64_t idx = KVIndexOfKey(this, key);
    
    if (idx == -1) {
        return NULL;
    }
    else {
        return KV_VALUE(this, idx);
    }
}

static int KVHas(KVStore *this, CKVString key) {
    /**
     * Check if value exists, return 1 if it does, 0 if not
     */
    
    return KVGet(this, key) != NULL;
}

static void KVDelete(KVStore *this, CKVString key) {
    /**
     * Mark a value as deleted if it exists. May not actually delete the value
     * but you won't be able to access it even if it doesn't.
     */
    
    int64_t idx = KVIndexOfKey(this, key);
    
    if (idx != -1) {
        if (KV_VALUE(this, idx)) {
            free(KV_VALUE(this, idx));
        }
        
        KV_VALUE(this, idx) = NULL;
    }
}

int knRegSet(lua_State *script) {
    if (lua_gettop(script) < 2) {
        lua_pushnil(script);
        return 1;
    }
    
    const char *key = lua_tostring(script, 1);
    const char *value = lua_tostring(script, 2);
    
    if (!key || !value) {
        lua_pushboolean(script, 0);
        return 1;
    }
    
    lua_pushboolean(script, KVSet(&gKvStore, key, value));
    return 1;
}

int knRegGet(lua_State *script) {
    if (lua_gettop(script) < 1) {
        lua_pushnil(script);
        return 1;
    }
    
    const char *key = lua_tostring(script, 1);
    
    if (!key) {
        lua_pushnil(script);
        return 1;
    }
    
    const char *value = KVGet(&gKvStore, key);
    
    if (value) {
        lua_pushstring(script, value);
    }
    else {
        lua_pushnil(script);
    }
    
    return 1;
}

int knRegHas(lua_State *script) {
    if (lua_gettop(script) < 1) {
        lua_pushnil(script);
        return 1;
    }
    
    const char *key = lua_tostring(script, 1);
    
    if (!key) {
        lua_pushboolean(script, 0);
        return 1;
    }
    
    lua_pushboolean(script, KVHas(&gKvStore, key));
    return 1;
}

int knRegDelete(lua_State *script) {
    if (lua_gettop(script) < 1) {
        return 0;
    }
    
    const char *key = lua_tostring(script, 1);
    
    if (!key) {
        return 0;
    }
    
    KVDelete(&gKvStore, key);
    
    return 0;
}

int knRegCount(lua_State *script) {
    lua_pushinteger(script, gKvStore.count);
    return 1;
}

int knEnableRegistry(lua_State *script) {
    lua_register(script, "knRegSet", knRegSet);
    lua_register(script, "knRegGet", knRegGet);
    lua_register(script, "knRegHas", knRegHas);
    lua_register(script, "knRegDelete", knRegDelete);
    lua_register(script, "knRegCount", knRegCount);
    return 0;
}
