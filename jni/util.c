#include <android_native_app_glue.h>
#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "extern/leaf.h"
#include "util.h"

struct android_app *gApp;

Leaf *gLeaf;

void *gLibAndroid;
void *gLibC;

bool KNInit(void) {
	/**
	 * Initialise some core stuff the shim needs
	 */
	
	// dynamically load libandroid.so for functions that might not be available
	// in older api levels and thus cannot be statically linked if we want to
	// keep running on these older versions.
	gLibAndroid = dlopen("libandroid.so", RTLD_NOW | RTLD_GLOBAL);
	
	if (!gLibAndroid) {
		return false;
	}
	
	// same goes for libc
	gLibC = dlopen("libc.so", RTLD_NOW | RTLD_GLOBAL);
	
	if (!gLibC) {
		return false;
	}
	
	return true;
}

#define LOAD_LIBANDROID_FUNC(RET, NAME, SIG) RET (*NAME)SIG = dlsym(gLibAndroid, #NAME);

int KNGetDeviceSDK(void) {
	/**
	 * Get the SDK level of the device this app is running on. If less than 24,
	 * this returns -1.
	 */
	
	LOAD_LIBANDROID_FUNC(int, android_get_device_api_level, (void));
	
	if (android_get_device_api_level) {
		return android_get_device_api_level();
	}
	else {
		return -1;
	}
}

int KNGetAppSDK(void) {
	/**
	 * Get the target SDK of the currently running app. This may return -1 if
	 * the system is running target SDK less than 24.
	 */
	
	LOAD_LIBANDROID_FUNC(int, android_get_application_target_sdk_version, (void));
	
	if (android_get_application_target_sdk_version) {
		return android_get_application_target_sdk_version();
	}
	else {
		return -1;
	}
}

void *KNGetSymbolAddr(const char *name) {
	/**
	 * Get the address of a symbol in libsmashhit.so, regardless of the loader
	 * type used.
	 */
	
	return LeafSymbolAddr(gLeaf, name);
}

int invert_branch(void *addr) {
	/**
	 * Invert the branch at the given address.
	 * 
	 * On armv7 this also allows to invert any conditional isntruction but is
	 * not supported when cond=AL (hex E, bin 0b1110) since that would require
	 * NOP'ing out the instruction, making it impossible to invert again.
	 * 
	 * On armv8 this may only work for instructions of the form b.COND and
	 * bc.COND with immidate values.
	 * 
	 * Return 0 on success, nonzero on error.
	 */
	
	#if defined(__ARM_ARCH_7A__)
	uint32_t instr = *(uint32_t *)addr;
	
	// Nicely, we're allowed to just flip bit 28 and get the inverse
	// branch for pretty much any type of condition :D
	// See ARMv7 manual A5.1 and A8.3
	if ((instr >> 29) != 0b111) {
		instr ^= 0x10000000;
		*(uint32_t *)addr = instr;
		return 0;
	}
	// The exception is for cond=AL or cond=1111, for which the first
	// is unconditional (and we'd have to nop out which would technically
	// work but means destroying what was there) and the second is also
	// unconditional but reserved for another set of unconditional
	// instructions.
	else {
		return 1;
	}
	#elif defined(__aarch64__)
	uint32_t instr = *(uint32_t *)addr;
	
	// Check that this is either B.cond or BC.cond - that's all we support here!
	// See C6.2.27 and C6.2.28 of ARMv8 reference manual. Note that bit 4 doesnt
	// matter for our purposes! Also see C4.2.1 for what cond bits are, same as
	// ARMv7 basically, so we can again just flip the bits though we don't need
	// to worry about if cond=1110 or 1111 since they're both the same anyways.
	if ((instr >> 24) == 0b01010100) {
		instr ^= 1;
		*(uint32_t *)addr = instr;
		return 0;
	}
	else {
		return 1;
	}
	#else
	return 1;
	#endif
}

#if defined(__ARM_ARCH_7A__)
#define LH_AARCH32
#elif defined(__aarch64__)
#define LH_AARCH64
#endif
#define LEAFHOOK_IMPLEMENTATION
#include "extern/leafhook.h"

LHHooker *gHooker;

static bool KNHookInit(void) {
	gHooker = LHHookerCreate();
	
	return !!gHooker;
}

bool KNHookFunction(void *func, void *hook, void **orig) {
	bool success;
	
	if (!gHooker) {
		success = KNHookInit();
		
		if (!success) {
			__android_log_print(ANDROID_LOG_ERROR, TAG, "Could not init leafhook hooker");
			return success;
		}
	}
	
	success = LHHookerHookFunction(gHooker, func, hook, orig);
	
	if (!success) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Error hooking function!");
	}
	
	return success;
}

bool KNLoadAsset(const char *path, void **data, size_t *size) {
	/**
	 * Load an asset, with a extra NUL byte at the end (not counted as part of
	 * length). Return true on success or false on failure. Size pointer is
	 * optional.
	 * 
	 * You will need to free() the pointer returned in data if successful.
	 */
	
	AAssetManager *asset_manager = gApp->activity->assetManager;
	AAsset *asset = AAssetManager_open(asset_manager, path, AASSET_MODE_BUFFER);
	
	// Try again with .mp3 suffix
	if (!asset) {
		char path_mp3[strlen(path) + 5];
		strcpy(path_mp3, path);
		strcat(path_mp3, ".mp3");
		asset = AAssetManager_open(asset_manager, path_mp3, AASSET_MODE_BUFFER);
	}
	
	if (!asset) {
		return false;
	}
	
	size_t t_size = AAsset_getLength(asset);
	const void *t_data = AAsset_getBuffer(asset);
	
	// Duplicate asset data with NUL at end
	char *f_data = malloc(t_size + 1);
	
	if (!f_data) {
		AAsset_close(asset);
		return false;
	}
	
	// Copy android's data buffer to ours
	memcpy(f_data, t_data, t_size);
	
	// Set NUL at end
	f_data[t_size] = '\0';
	
	// Write our results
	*data = f_data;
	if (size) { *size = t_size; }
	
	// Clean up
	AAsset_close(asset);
	
	return true;
}

bool KNPreformInBackground(PthreadCallbackFunc func, void *arg) {
	/**
	 * Call func with argument as arg in a new background thread. It will be
	 * detached immidately.
	 */
	
	pthread_t thrd;
	
	int result = pthread_create(&thrd, NULL, func, arg);
	
	if (result) {
		return false;
	}
	
	pthread_detach(thrd);
	
	return true;
}

#define JNI_EXCEPTION_ABORT(JNI, ...) {\
	if ((*JNI)->ExceptionCheck(JNI) == JNI_TRUE) {\
		(*JNI)->ExceptionDescribe(JNI);\
		__android_log_print(ANDROID_LOG_FATAL, TAG, __VA_ARGS__);\
		abort();\
	}\
}

static jmethodID jni_get_method_id(JNIEnv *jni, const char *className, const char *methodName, const char *methodSignature) {
	jclass theClass = (*jni)->FindClass(jni, className);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_method_id(%p, %s, %s, %s): Exception pending, fuck! Maybe the class wasn't found?", jni, className, methodName, methodSignature);
	
	jmethodID theMethod = (*jni)->GetMethodID(jni, theClass, methodName, methodSignature);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_method_id(%p, %s, %s, %s): Exception pending, fuck! Maybe the method wasn't found?", jni, className, methodName, methodSignature);
	
	return theMethod;
}

static jfieldID jni_get_field_id(JNIEnv *jni, const char *className, const char *fieldName, const char *fieldSignature) {
	jclass theClass = (*jni)->FindClass(jni, className);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_field_id(%p, %s, %s, %s): Exception pending, fuck! Maybe the class wasn't found?", jni, className, fieldName, fieldSignature);
	
	jfieldID theField = (*jni)->GetFieldID(jni, theClass, fieldName, fieldSignature);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_field_id(%p, %s, %s, %s): Exception pending, fuck! Maybe the method wasn't found?", jni, className, fieldName, fieldSignature);
	
	return theField;
}

float KNGetRefreshRate(void) {
	/**
	 * Get the default display's native framerate.
	 */
	
	// Get JNI and current activity references (we shall need them later)
	JavaVM *vm = gApp->activity->vm;
	JNIEnv *jni = NULL;
	
	if ((*vm)->GetEnv(vm, (void **)&jni, JNI_VERSION_1_6) != JNI_OK) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "JNI not okay, go fuck yourself :)");
		abort();
	}
	
	jobject nativeActivityInstance = gApp->activity->clazz;
	
	// Method ID for getSystemService
	jmethodID getSystemService = jni_get_method_id(jni, "android/app/NativeActivity", "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	
	// Window string (needed for getSystemService)
	jstring window = (*jni)->NewStringUTF(jni, "window");
	
	JNI_EXCEPTION_ABORT(jni, "pending exception after creating window string");
	
	// getSystemService("window")
	jobject windowService = (*jni)->CallObjectMethod(jni, nativeActivityInstance, getSystemService, window);
	
	JNI_EXCEPTION_ABORT(jni, "pending exception after nativeActivityInstance.getSystemService('window')");
	
	// Method ID for getDefaultDisplay
	jmethodID getDefaultDisplay = jni_get_method_id(jni, "android/view/WindowManager", "getDefaultDisplay", "()Landroid/view/Display;");
	
	// .getDefaultDisplay()
	jobject defaultDisplay = (*jni)->CallObjectMethod(jni, windowService, getDefaultDisplay);
	
	JNI_EXCEPTION_ABORT(jni, "pending exception after windowService.getDefaultDisplay()");
	
	// Method ID for getRefreshRate
	jmethodID getRefreshRate = jni_get_method_id(jni, "android/view/Display", "getRefreshRate", "()F");
	
	// .getRefreshRate()
	float refreshRate = (*jni)->CallFloatMethod(jni, defaultDisplay, getRefreshRate);
	
	JNI_EXCEPTION_ABORT(jni, "pending exception after defaultDisplay.getRefreshRate()");
	
	return refreshRate;
}

bool KNGetAppVersion(char *buffer, size_t maxSize) {
	/**
	 * Query the app's version string
	 */
	
	JavaVM *vm = gApp->activity->vm;
	JNIEnv *jni = NULL;
	
	if ((*vm)->GetEnv(vm, (void **)&jni, JNI_VERSION_1_6) != JNI_OK) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "JNI not okay, go fuck yourself :)");
		abort();
	}
	
	jobject nativeActivityInstance = gApp->activity->clazz;
	
	// Method IDs
	jmethodID getPackageName = jni_get_method_id(jni, "android/app/NativeActivity", "getPackageName", "()Ljava/lang/String;");
	jmethodID getPackageManager = jni_get_method_id(jni, "android/app/NativeActivity", "getPackageManager", "()Landroid/content/pm/PackageManager;");
	jmethodID getPackageInfo = jni_get_method_id(jni, "android/content/pm/PackageManager", "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
	
	// Field ID
	jfieldID versionName = jni_get_field_id(jni, "android/content/pm/PackageInfo", "versionName", "Ljava/lang/String;");
	
	jobject packageName = (*jni)->CallObjectMethod(jni, nativeActivityInstance, getPackageName);
	JNI_EXCEPTION_ABORT(jni, "pending exception after nativeActivity.getPackageName()");
	
	jobject packageManager = (*jni)->CallObjectMethod(jni, nativeActivityInstance, getPackageManager);
	JNI_EXCEPTION_ABORT(jni, "pending exception after nativeActivity.getPackageManager()");
	
	jobject packageInfo = (*jni)->CallObjectMethod(jni, packageManager, getPackageInfo, packageName, 0);
	JNI_EXCEPTION_ABORT(jni, "pending exception after packageManager.getPackageInfo(packageName, 0)");
	
	jstring vn = (*jni)->GetObjectField(jni, packageInfo, versionName);
	JNI_EXCEPTION_ABORT(jni, "pending exception after vn = packageInfo.versionName");
	
	const char *theString = (*jni)->GetStringUTFChars(jni, vn, NULL);
	
	if (!theString) {
		buffer[0] = '\0';
		return false;
	}
	
	strncpy(buffer, theString, maxSize);
	
	(*jni)->ReleaseStringUTFChars(jni, vn, theString);
	
	return true;
}
