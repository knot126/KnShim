#ifndef _SHIM_UTIL_H
#define _SHIM_UTIL_H

#include <android_native_app_glue.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "andrleaf.h"

#define TAG "smashshim"

#if defined(__ARM_ARCH_7A__)
#define KN_ARCH_STRING "armeabi-v7a"
#elif defined(__aarch64__)
#define KN_ARCH_STRING "arm64-v8a"
#else
#define KN_ARCH_STRING "unknown"
#endif

typedef struct KNHookManager {
	void *code;
	size_t code_alloced;
	size_t hook_count;
	size_t bytes_per_longjump;
} KNHookManager;

typedef void (*ModuleInitFunc)(struct android_app *app, Leaf *leaf);

void *KNGetSymbolAddr(const char *name);
int invert_branch(void *addr);
int replace_function(void *from, void *to);

int KNHookManagerInit(KNHookManager *self);
int KNHookManagerHookFunction(KNHookManager *self, void *func_to_hook, void *new_func, void **hooked_func_start);

void KNHookInit(void);
void KNHookFunction(void *func, void *hook, void **orig);

#define knLuaPushEnum(SCRIPT, ENUM_NAME) lua_pushinteger(SCRIPT, ENUM_NAME); lua_setglobal(SCRIPT, #ENUM_NAME);

#endif // _SHIM_UTIL_H
