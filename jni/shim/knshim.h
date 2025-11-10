/**
 * This header contains all declarations for the KnShim API for plug-in
 * developers.
 * 
 * -----------------------------------------------------------------------------
 * 
 * This file is part of KnShim. Copyright (c) 2024 - 2025 Knot126.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _KNSHIM_API_H_
#define _KNSHIM_API_H_

#include <stdbool.h>
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
