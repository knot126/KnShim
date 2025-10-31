/**
 * This header contains all declarations for the KnShim API for plug-in
 * developers.
 */

#ifndef _KNSHIM_API_H_
#define _KNSHIM_API_H_

#include <stdlib.h>

#include "lua/lua.h"

// Architecture ID
typedef enum KnArch {
	KN_ARCH_ALL = 0,
	KN_ARCH_AARCH32 = 7,
	KN_ARCH_AARCH64 = 8,
} KnArch;

// Scripting extension API
bool KnRegisterLuaScriptFunction(const char *name, lua_CFunction function);
bool KnRegisterLuaScriptGlobalInt(const char *name, lua_Integer value);
bool KnRegisterLuaScriptGlobalString(const char *name, const char *value);

// Function hooking utilities
bool KnReplaceNativeFunction(const char *symbol, void *replacement);
void *KnHookNativeFunction(const char *symbol, void *hook);

// Patching
bool KnPatch(size_t vaddr, const void *bytes, size_t size, KnArch arch);

#endif
