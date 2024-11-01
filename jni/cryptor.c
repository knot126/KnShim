#ifdef BUILD_CIPHER
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <sys/stat.h>
#include "smashhit.h"
#include "util.h"

// Should define a function with the declaration:
// void MyCipher(void *buffer, size_t length, size_t counter, uint32_t mode)
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
	
	MyCipher(buffer, length, pos, 0);
	
	stream->headpos += length;
	
	return 1;
}

size_t KNCipher_writeInternal(QiFileOutputStream *stream, char *buffer, size_t length) {
	size_t pos = 0;
	
	// Get counter position -- since this is a write only stream its fine to
	// just stat() it.
	struct stat stream_stats;
	if (fstat(fileno(stream->file), &stream_stats) == 0) {
		pos = stream_stats.st_size;
	}
	
	char *buffer_crypt = KNCipher_MemDup(buffer, length);
	
	if (!buffer_crypt) { return 0; }
	
	MyCipher(buffer_crypt, length, pos, 1);
	
	if (fwrite(buffer_crypt, 1, length, stream->file) != length) {
		free(buffer_crypt);
		return 0;
	}
	
	free(buffer_crypt);
	return 1;
}

void KNCipherInit(void *libsmashhit) {
	// TODO these names vary by arch
#if defined(__ARM_ARCH_7A__)
	void *readInternal = dlsym(libsmashhit, "_ZN17QiFileInputStream12readInternalEPcj");
	void *writeInternal = dlsym(libsmashhit, "_ZN18QiFileOutputStream13writeInternalEPKcj");
#elif defined(__aarch64__)
	void *readInternal = dlsym(libsmashhit, "_ZN17QiFileInputStream12readInternalEPcm");
	void *writeInternal = dlsym(libsmashhit, "_ZN18QiFileOutputStream13writeInternalEPKcm");
#else
#error Unsupported platform
#endif
	
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
