#ifndef _SHIM_UTIL_H
#define _SHIM_UTIL_H

#include <stdlib.h>
#include <sys/mman.h>

#define TAG "smashshim"

enum {
	KN_MEM_READ_WRITE = PROT_READ | PROT_WRITE,
	KN_MEM_READ_RUN = PROT_READ | PROT_EXEC,
	KN_MEM_READ_ONLY = PROT_READ,
};

extern void *gLibsmashhitHandle;

int set_memory_protection(void *addr, size_t length, int protection);
int unprotect_memory(void *addr, size_t length);
int invert_branch(void *addr);
int replace_function(void *from, void *to);

#define knLuaPushEnum(SCRIPT, ENUM_NAME) lua_pushinteger(SCRIPT, ENUM_NAME); lua_setglobal(SCRIPT, #ENUM_NAME);

#endif // _SHIM_UTIL_H
