#include <android_native_app_glue.h>
#include <android/log.h>
#include "util.h"
#include "smashhit.h"

void (*gDebugLogFunc)(void *this, char *message, int type);

// struct {
// 	void *_unk0;
// 	void *_unk1;
// 	void (*print)(void *this, char *message);
// } gDebugPrintStream;

void custom_debug_log(void *this, char *message, int type) {
	int level;
	
	switch (type) {
		case 1:
			level = ANDROID_LOG_INFO;
			break;
		case 2:
			level = ANDROID_LOG_WARN;
			break;
		case 4:
			level = ANDROID_LOG_ERROR;
			break;
		default:
			level = ANDROID_LOG_UNKNOWN;
			break;
	}
	
	__android_log_print(level, "smashhit", "%s", message);
	
	// I probably don't need to call the original function, but I need to test
	// the hooking stuff soo... :3
	gDebugLogFunc(this, message, type);
}

// void custom_debug_log_2(void *this, char *message) {
// 	__android_log_print(ANDROID_LOG_INFO, "smashhit", "%s", message);
// }
// 
// void *custom_get_print_stream(void *this) {
// 	return &gDebugPrintStream;
// }

void KNDebugLogInit(struct android_app *app, Leaf *leaf) {
	// void *log = KNGetSymbolAddr("_ZN5Debug3logEPKci");
	
	// KNHookFunction(log, custom_debug_log, (void**) &gDebugLogFunc);
	
	// __android_log_print(ANDROID_LOG_INFO, TAG, "Will direct calls to Debug::log() at <%p> to <%p> with orig stub at <%p>", log, custom_debug_log, gDebugLogFunc);
	
	// some stuff looks inlined except for getPrintStream which uses a virtual
	// function or smth so we have a little hack to make that work
	// gDebugPrintStream.print = custom_debug_log_2;
	// KNHookFunction(KNGetSymbolAddr("_ZN7QiDebug14getPrintStreamEv"), custom_get_print_stream, NULL);
}
