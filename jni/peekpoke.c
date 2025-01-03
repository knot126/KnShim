#include <android_native_app_glue.h>
#include <android/log.h>
#include <dlfcn.h>
#include <string.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

#define knGetAddress(SCRIPT, INDEX) (lua_isstring(SCRIPT, INDEX) ? (size_t)KNGetSymbolAddr(lua_tostring(SCRIPT, INDEX)) : (size_t)lua_tointeger(SCRIPT, INDEX))

// MEMORY
enum {
	KN_TYPE_ADDR = 1,
	KN_TYPE_BOOL,
	KN_TYPE_SHORT,
	KN_TYPE_INT,
	KN_TYPE_FLOAT,
	KN_TYPE_STRING,
	KN_TYPE_BYTES,
};

int knSymbolAddr(lua_State *script) {
	/**
	 * addr = knSymbolAddr(symbolName)
	 * 
	 * Returns the address of the symbol named in the first parameter.
	 * Note that if using C++ functions, you must used the mangled symbol names.
	 * For example, use "_ZN5Level12hitSomethingEi" instead of "Level::hitSomething".
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	size_t sym = (size_t) KNGetSymbolAddr(lua_tostring(script, 1));
	lua_pushinteger(script, sym);
	return 1;
}

int knPeek(lua_State *script) {
	/**
	 * value = knPeek(addr, type, [size])
	 * 
	 * Reads the value at `addr` of type `type` and returns it.
	 * 
	 * If type is KN_TYPE_BYTES, then a third argument (size) is required and is
	 * the number of bytes to read.
	 */
	
	if (lua_gettop(script) < 2) {
		lua_pushnil(script);
		return 1;
	}
	
	size_t addr = knGetAddress(script, 1);
	int type = lua_tointeger(script, 2);
	
	switch (type) {
		case KN_TYPE_ADDR:
			lua_pushinteger(script, *(size_t *)addr);
			return 1;
		case KN_TYPE_BOOL:
			lua_pushboolean(script, *(unsigned char *)addr);
			return 1;
		case KN_TYPE_SHORT:
			lua_pushinteger(script, *(short *)addr);
			return 1;
		case KN_TYPE_INT:
			lua_pushinteger(script, *(int *)addr);
			return 1;
		case KN_TYPE_FLOAT:
			lua_pushnumber(script, *(float *)addr);
			return 1;
		case KN_TYPE_STRING:
			lua_pushstring(script, (const char *)addr);
			return 1;
		case KN_TYPE_BYTES:
			if (lua_gettop(script) < 3) {
				lua_pushnil(script);
				return 1;
			}
			lua_pushlstring(script, (const char *)addr, lua_tointeger(script, 3));
			return 1;
		default: {
			lua_pushnil(script);
			return 1;
		}
	}
}

int knPoke(lua_State *script) {
	/**
	 * addrOrNil = knPoke(addr, type, value)
	 * 
	 * Write the `value` of type `type` to the given address and return the
	 * address written to, if successful.
	 * 
	 * Note that you may need to call `knUnprotect(addr, sizeOfType)` before
	 * writing any values to ensure that the values are actually writable.
	 */
	
	if (lua_gettop(script) < 2) {
		lua_pushnil(script);
		return 1;
	}
	
	size_t addr = knGetAddress(script, 1);
	int type = lua_tointeger(script, 2);
	
	switch (type) {
		case KN_TYPE_ADDR:
			*((size_t *)addr) = (size_t) lua_tointeger(script, 3);
			break;
		case KN_TYPE_BOOL:
			*((unsigned char *)addr) = (unsigned char) lua_toboolean(script, 3);
			break;
		case KN_TYPE_SHORT:
			*((short *)addr) = (short) lua_tointeger(script, 3);
			break;
		case KN_TYPE_INT:
			*((int *)addr) = (int) lua_tointeger(script, 3);
			break;
		case KN_TYPE_FLOAT:
			*((float *)addr) = (float) lua_tonumber(script, 3);
			break;
		case KN_TYPE_STRING:
			strcpy((char *)addr, lua_tostring(script, 3));
			break;
		case KN_TYPE_BYTES: {
			size_t size;
			const char *string = lua_tolstring(script, 3, &size);
			memcpy((char *)addr, string, size);
			break;
		}
		default:
			lua_pushnil(script);
			return 1;
	}
	
	lua_pushinteger(script, addr);
	return 1;
}

int knSystemAbi(lua_State *script) {
	/**
	 * abi = knSystemAbi()
	 * 
	 * Return the system ABI/CPU architecture as a string.
	 * 
	 * * ARMv7 is "armeabi-v7a"
	 * * ARMv8 is "arm64-v8a"
	 * * x86 is "x86"
	 * * Anything else returns "unknown"
	 */
	
	lua_pushstring(script, KN_ARCH_STRING);
	
	return 1;
}

int knInvertBranch(lua_State *script) {
	/**
	 * addrOrZero = knInvertBranch(addr)
	 * 
	 * Inverts the branch at the given address. For example, b.ne <imm> will
	 * become b.eq <imm> on ARMv8. Note that for ARMv8, only immidate branches
	 * are supported. On ARMv7, *any* instruction that is already conditional
	 * can be inverted.
	 * 
	 * On success, this returns the address of the branch that was changed. On
	 * failure this returns nil.
	 */
	
	if (lua_gettop(script) < 1) {
		lua_pushnil(script);
		return 1;
	}
	
	size_t addr = knGetAddress(script, 1);
	
	if (invert_branch((void *)addr)) {
		lua_pushnil(script);
	}
	else {
		lua_pushinteger(script, addr);
	}
	
	return 1;
}
// END MEMORY

int knEnablePeekPoke(lua_State *script) {
	lua_register(script, "knSymbolAddr", knSymbolAddr);
	lua_register(script, "knPeek", knPeek);
	lua_register(script, "knPoke", knPoke);
	lua_register(script, "knSystemAbi", knSystemAbi);
	lua_register(script, "knInvertBranch", knInvertBranch);
	
	// Types
	knLuaPushEnum(script, KN_TYPE_ADDR);
	knLuaPushEnum(script, KN_TYPE_BOOL);
	knLuaPushEnum(script, KN_TYPE_SHORT);
	knLuaPushEnum(script, KN_TYPE_INT);
	knLuaPushEnum(script, KN_TYPE_FLOAT);
	knLuaPushEnum(script, KN_TYPE_STRING);
	knLuaPushEnum(script, KN_TYPE_BYTES);
	
	return 0;
}
