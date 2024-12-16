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
// void MyCipher(const char *path, void *buffer, size_t length, size_t counter, uint32_t mode)
// mode is 0 if read, 1 if write
// path could be used as a primitive nonce
#include "../../KnShimCipher/cipher.h"

static char *KNCipher_MemDup(char *buffer, size_t length) {
	char *new = malloc(length);
	
	if (new) {
		memcpy(new, buffer, length);
	}
	
	return new;
}

static const char *KNCipher_GetQiStringData(QiString *string) {
	return string->data ? string->data : string->cached;
}

size_t KNCipher_readInternal(QiFileInputStream *stream, char *buffer, size_t length) {
	int pos = stream->headpos;
	
	if (fread(buffer, 1, length, stream->file) != length) {
		return 0;
	}
	
	MyCipher(KNCipher_GetQiStringData(&stream->path), buffer, length, pos, 0);
	
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
	
	MyCipher(KNCipher_GetQiStringData(&stream->path), buffer_crypt, length, pos, 1);
	
	if (fwrite(buffer_crypt, 1, length, stream->file) != length) {
		free(buffer_crypt);
		return 0;
	}
	
	free(buffer_crypt);
	return 1;
}

void KNCipherInit(struct android_app *app, Leaf *leaf) {
	// TODO these names vary by arch
#if defined(__ARM_ARCH_7A__)
	void *readInternal = KNGetSymbolAddr("_ZN17QiFileInputStream12readInternalEPcj");
	void *writeInternal = KNGetSymbolAddr("_ZN18QiFileOutputStream13writeInternalEPKcj");
#elif defined(__aarch64__)
	void *readInternal = KNGetSymbolAddr("_ZN17QiFileInputStream12readInternalEPcm");
	void *writeInternal = KNGetSymbolAddr("_ZN18QiFileOutputStream13writeInternalEPKcm");
#else
#error Unsupported platform
#endif
	
	KNHookFunction(readInternal, &KNCipher_readInternal, NULL);
	KNHookFunction(writeInternal, &KNCipher_writeInternal, NULL);
}
#endif
