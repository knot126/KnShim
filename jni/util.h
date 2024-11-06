#ifndef _SHIM_UTIL_H
#define _SHIM_UTIL_H

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

enum {
	KN_MEM_READ_WRITE = PROT_READ | PROT_WRITE,
	KN_MEM_READ_RUN = PROT_READ | PROT_EXEC,
	KN_MEM_READ_ONLY = PROT_READ,
};

#ifndef USE_LEAF
extern void *gLibsmashhitHandle;
#else
extern Leaf *gLeaf;
#endif

int set_memory_protection(void *addr, size_t length, int protection);
int unprotect_memory(void *addr, size_t length);
void *KNGetSymbolAddr(const char *name);
int invert_branch(void *addr);
int replace_function(void *from, void *to);

#define knLuaPushEnum(SCRIPT, ENUM_NAME) lua_pushinteger(SCRIPT, ENUM_NAME); lua_setglobal(SCRIPT, #ENUM_NAME);

#endif // _SHIM_UTIL_H
