#ifdef BUILD_CIPHER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>
// #include <sys/stat.h>
#include "smashhit.h"
#include "util.h"

struct android_app *gApp;

// bool LoadObfuscatedAsset(const char *path, size_t *size, void **buffer)
#include "../../KnShimCipher/new/cipher_output.c"

bool (*ResMan_load)(ResMan *resman, QiString *path, QiOutputStream *output);
void (*QiOutputStream_writeBuffer)(QiOutputStream *this, void *buffer, size_t length);

bool cstr_starts_with(const char *str, const char *prefix) {
	return strncmp(prefix, str, strlen(prefix));
}

bool ResManLoadHook(ResMan *resman, QiString *path, QiOutputStream *output) {
	// File path as C string
	const char *cc_path = path->data ? path->data : path->cached;
	
	// Just let resman load these classes of asset
	if (cstr_starts_with(cc_path, "user://") || cstr_starts_with(cc_path, "http://")) {
		return ResMan_load(resman, path, output);
	}
	
	// Load obfuscated asset
	size_t asset_size;
	void *asset_buffer;
	bool result = LoadObfuscatedAsset(cc_path, &asset_size, &asset_buffer);
	
	if (!result) {
		// Try to load obfuscated asset but with .mp3 extension
		char path_mp3[strlen(cc_path) + 5];
		strcpy(path_mp3, cc_path);
		strcat(path_mp3, ".mp3");
		result = LoadObfuscatedAsset(path_mp3, &asset_size, &asset_buffer);
	}
	
	if (!result) {
		// Try loading with the "real" resman
		result = ResMan_load(resman, path, output);
	}
	else {
		// Write contents to output stream and free asset buffer
		QiOutputStream_writeBuffer(output, asset_buffer, asset_size);
		free(asset_buffer);
	}
	
	return result;
}

void KNCipherInit(struct android_app *app, Leaf *leaf) {
	void *pResManLoad = KNGetSymbolAddr("_ZN6ResMan4loadERK8QiStringR14QiOutputStream");
	KNHookFunction(pResManLoad, &ResManLoadHook, (void **) &ResMan_load);
	
	QiOutputStream_writeBuffer = KNGetSymbolAddr("_ZN14QiOutputStream11writeBufferEPKvm"); // TODO 32bit
	
	gApp = app;
}
#endif
