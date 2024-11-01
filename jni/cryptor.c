#ifdef BUILD_CIPHER
#include <stdio.h>
#include <dlfcn.h>
#include <android/log.h>
#include "smashhit.h"
#include "util.h"

// Should define a function with the declaration:
// int MyCipher(void *buffer, size_t length, size_t counter, int mode)
#include "../../KnShimCipher/cipher.h"

size_t KNCipher_readInternal(QiFileInputStream *stream, char *buffer, size_t length) {
	__android_log_write(ANDROID_LOG_INFO, "SmashHitCryptor", "doing custom readInternal");
	
	if (fread(buffer, 1, length, stream->file) != length) {
		__android_log_write(ANDROID_LOG_INFO, "SmashHitCryptor", "custom asset reading failed");
		return 0;
	}
	else {
		__android_log_write(ANDROID_LOG_INFO, "SmashHitCryptor", "read success");
	}
	
	return 1;
}

size_t KNCipher_writeInternal(QiFileOutputStream *stream, char *buffer, size_t length) {
	__android_log_write(ANDROID_LOG_INFO, "SmashHitCryptor", "doing custom writeInternal");
	
	if (fwrite(buffer, 1, length, stream->file) != length) {
		__android_log_write(ANDROID_LOG_INFO, "SmashHitCryptor", "custom write failed");
		return 0;
	}
	else {
		__android_log_write(ANDROID_LOG_INFO, "SmashHitCryptor", "write success");
	}
	
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
