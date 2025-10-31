/**
 * KnShim's public API for use by native extensions.
 */

#include <string.h>
#include <stdlib.h>

#include "knshim.h"

typedef enum KnRegType {
	KN_REG_CFUNCTION = 1,
	KN_REG_INTEGER = 2,
	KN_REG_NUMBER = 3,
	KN_REG_STRING = 4,
} KnRegType;

typedef struct KnLuaRegistryEntry {
	const char *identifier;
	union {
		lua_CFunction cfunc;
		lua_Integer integer;
		lua_Number number;
		const char *string;
	};
	KnRegType type;
} KnLuaRegistryEntry;

typedef struct KnLuaRegistry {
	KnLuaRegistryEntry *entries;
	size_t length;
	size_t capacity;
} KnLuaRegistry;

static inline int KnLuaRegistry_Realloc(KnLuaRegistry *self, size_t n) {
	/**
	 * Reallocate the lua registry table so it can fit at least n more entries
	 */
	
	while (this->length + n > this->capacity) {
		const size_t cap = self->capacity + (self->capacity >> 2) + 1;
		KnLuaRegistryEntry * const ents = realloc(this->entries, cap * sizeof *this->entries);
		if (!ents) {
			return 0;
		}
		this->entries = ents;
		this->capacity = cap;
	}
	return 1;
}

static inline int KnLuaRegistry_Append(KnLuaRegistry *self, KnLuaRegistryEntry ent) {
	/**
	 * Append one entry to the Lua registry table
	 */
	
	if (!KnLuaRegistry_Realloc(self, 1)) {
		return 0;
	}
	
	this->entries[this->length++] = ent;
	
	return 1;
}

KnLuaRegistry gLuaRegistry;

bool KnRegisterLuaScriptFunction(const char *name, lua_CFunction function) {
	/**
	 * Register a lua_CFunction for registration in all future scripts. The name
	 * shall be a valid Lua identifier, and function shall be a valid pointer to
	 * a Lua C function.
	 */
	
	return KnLuaRegistry_Append(&gLuaRegistry, (KnLuaRegistryEntry) {
		.identifier = name,
		.cfunc = function,
		.type = KN_REG_CFUNCTION,
	});
}

bool KnRegisterLuaScriptGlobalInt(const char *name, lua_Integer value) {
	/**
	 * Register a lua_Integer for registration in all future scripts.
	 */
	
	return KnLuaRegistry_Append(&gLuaRegistry, (KnLuaRegistryEntry) {
		.identifier = name,
		.integer = value,
		.type = KN_REG_INTEGER,
	});
}

bool KnRegisterLuaScriptGlobalString(const char *name, const char *value) {
	/**
	 * Register a string for registration in all future scripts.
	 */
	
	return KnLuaRegistry_Append(&gLuaRegistry, (KnLuaRegistryEntry) {
		.identifier = name,
		.string = value,
		.type = KN_REG_STRING,
	});
}

const char *KNPublicAPIInit(void) {
	KnLuaRegistry_Init(KnLuaRegistry *self)
}
