#ifdef BUILD_CIPHER
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include "smashhit.h"
#include "util.h"

// Should define a function with the declaration:
// void MyCipher(void *buffer, size_t length, size_t counter)
#include "../../KnShimCipher/cipher.h"

char *KNCipher_MemDup(char *buffer, size_t length) {
	char *new = malloc(length);
	
	if (new) {
		memcpy(new, buffer, length);
	}
	
	return new;
}

size_t KNCipher_readInternal(QiFileInputStream *stream, char *buffer, size_t length) {
	int pos = stream->headpos;
	
	if (fread(buffer, 1, length, stream->file) != length) {
		return 0;
	}
	
	MyCipher(buffer, length, pos);
	
	return 1;
}

size_t KNCipher_writeInternal(QiFileOutputStream *stream, char *buffer, size_t length) {
	char *buffer_crypt = KNCipher_MemDup(buffer, length);
	
	if (!buffer_crypt) { return 0; }
	
	MyCipher(buffer_crypt, length, ftell(stream->file));
	
	if (fwrite(buffer_crypt, 1, length, stream->file) != length) {
		free(buffer_crypt);
		return 0;
	}
	
	free(buffer_crypt);
	return 1;
}

void KNCipherInit(void *libsmashhit) {
	// TODO these names vary by arch
	void *readInternal = dlsym(libsmashhit, "_ZN17QiFileInputStream12readInternalEPcm");
	void *writeInternal = dlsym(libsmashhit, "_ZN18QiFileOutputStream13writeInternalEPKcm");
	
	// WARNING set_memory_protection with exec doesnt work on modern android even
	// if its RX, why!?
	set_memory_protection(readInternal, 16, KN_MEM_READ_WRITE);
	replace_function(readInternal, &KNCipher_readInternal);
	set_memory_protection(readInternal, 16, KN_MEM_READ_RUN);
	
	set_memory_protection(writeInternal, 16, KN_MEM_READ_WRITE);
	replace_function(writeInternal, &KNCipher_writeInternal);
	set_memory_protection(writeInternal, 16, KN_MEM_READ_RUN);
}
#endif
