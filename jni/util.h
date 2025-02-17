#ifndef _SHIM_UTIL_H
#define _SHIM_UTIL_H

#include <android_native_app_glue.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "andrleaf.h"

#define TAG "smashshim"

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

typedef void (*ModuleInitFunc)(struct android_app *app, Leaf *leaf);
typedef void *(*PthreadCallbackFunc)(void *arg);

void *KNGetSymbolAddr(const char *name);
int invert_branch(void *addr);
int replace_function(void *from, void *to);

bool KNHookFunction(void *func, void *hook, void **orig);
bool KNLoadAsset(const char *path, void **data, size_t *size);
bool KNPreformInBackground(PthreadCallbackFunc func, void *arg);
float KNGetRefreshRate(void);

#define knRegisterFunc(SCRIPT, NAME) lua_register(SCRIPT, #NAME, NAME)
#define knLuaPushEnum(SCRIPT, ENUM_NAME) lua_pushinteger(SCRIPT, ENUM_NAME); lua_setglobal(SCRIPT, #ENUM_NAME);
#define knReturnNil(SCRIPT) lua_pushnil(SCRIPT); return 1;

#endif // _SHIM_UTIL_H
