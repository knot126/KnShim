#include <android_native_app_glue.h>
#include <android/log.h>
#include "util.h"
#include "smashhit.h"

void (*gDebugLogFunc)(void *this, char *message, int type);

void custom_debug_log(void *this, char *message, int type) {
	__android_log_print(ANDROID_LOG_INFO, TAG, "Debug::log(): %s", message);
	
	// I probably don't need to call the original function, but I need to test
	// the hooking stuff soo... :3
	gDebugLogFunc(this, message, type);
}

void KNDebugLogInit(struct android_app *app, Leaf *leaf) {
	void *log = KNGetSymbolAddr("_ZN5Debug3logEPKci");
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Will direct calls to Debug::log() at <%p> to <%p>", log, custom_debug_log);
	
	KNHookFunction(log, custom_debug_log, (void**) &gDebugLogFunc);
	// KNHookFunction(log, &custom_debug_log, NULL);
}
