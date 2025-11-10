/**
 * Module to bypass anti-tamper checksum verification checks for Mediocre games
 * that have them.
 */

#include <string.h>
#include <android/log.h>
#include "../util.h"

bool KNLoadAssetLeanAndMean(const char *path, void **data, size_t *size);

void computeChecksum() {
	void *data = NULL;
	size_t size = 0;
	
	if (KNLoadAssetLeanAndMean("sig.mp3", &data, &size) && size >= 0x100) {
		memcpy(KNGetSymbolAddr("gChecksum"), data, 0x100);
	}
	else {
		LogW("APK checksum file not found or too small; the game may force close unless cracked. Create any 256 byte file called `sig.mp3` in the root of the assets directory to resolve this issue.");
	}
	
	free(data);
}

const char *KNAntitamperInit(void) {
	KNHookFunctionByName("_Z15computeChecksumRK8QiStringPc", computeChecksum, true);
	
	return NULL;
}
