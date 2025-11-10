/**
 * Various utility functions that don't belong elsewhere.
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

#ifndef _SHIM_UTIL_H
#define _SHIM_UTIL_H

#include <android_native_app_glue.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "extern/leaf.h"
#include "builtin/smashhit.h"

#ifdef GRANNY
#define KN_GAME_STRING "grannysmith"
#else
#define KN_GAME_STRING "smashhit"
#endif

#if defined(__arm__)
#define KN_ARCH_STRING "armeabi-v7a"
#define KN_RET 0xe12fff1e
typedef uint32_t shortop_t;
#elif defined(__aarch64__)
#define KN_ARCH_STRING "arm64-v8a"
#define KN_RET 0xd65f03c0
typedef uint32_t shortop_t;
#elif defined(__i386__)
#define KN_ARCH_STRING "x86"
#define KN_RET 0xc3
typedef uint8_t shortop_t;
#else
#define KN_ARCH_STRING "unknown"
#endif

extern Leaf *gLeaf;
extern struct android_app *gApp;

extern void *gLibAndroid;
extern void *gLibC;
extern Game **gGamePtr;

#define gGame (*gGamePtr)

typedef const char *(*ModuleInitFunc)(void);
typedef void *(*PthreadCallbackFunc)(void *arg);

int KNGetDeviceSDK(void);
int KNGetAppSDK(void);

void *KNGetSymbolAddr(const char *name);
bool KNPatch(size_t vaddr, const char *bytes, size_t size);

bool KNHookFunction(void *func, void *hook, void **orig);
void *KNHookFunctionByName(const char *name, void *hook, bool replace);
bool KNLoadAsset(const char *path, void **data, size_t *size);

bool KNPreformInBackground(PthreadCallbackFunc func, void *arg);
float KNGetRefreshRate(void);
bool KNGetAppVersion(char *buffer, size_t maxSize);

#define knRegisterFunc(SCRIPT, NAME) lua_register(SCRIPT, #NAME, NAME)
#define knLuaPushEnum(SCRIPT, ENUM_NAME) lua_pushinteger(SCRIPT, ENUM_NAME); lua_setglobal(SCRIPT, #ENUM_NAME);
#define knReturnNil(SCRIPT) lua_pushnil(SCRIPT); return 1;
#define KNLoadFunc(RET, NAME, SIG) RET (*NAME) SIG = KNGetSymbolAddr(#NAME);

#include "log.h"

#endif // _SHIM_UTIL_H
