/**
 * Utility functions that require the use of the JNI.
 * 
 * -----------------------------------------------------------------------------
 * 
 * This file is part of KnShim. Copyright (c) 2024 - 2025 Knot126.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <android_native_app_glue.h>
#include <string.h>

#include "loader.h"
#include "log.h"

#define JNI_EXCEPTION_ABORT(JNI, ...) {\
	if ((*JNI)->ExceptionCheck(JNI) == JNI_TRUE) {\
		(*JNI)->ExceptionDescribe(JNI);\
		LogF(__VA_ARGS__);\
		abort();\
	}\
}

static jmethodID KnShim_GetMethodID(JNIEnv *jni, const char *className, const char *methodName, const char *methodSignature) {
	jclass theClass = (*jni)->FindClass(jni, className);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_method_id(%p, %s, %s, %s): Exception pending error", jni, className, methodName, methodSignature);
	
	jmethodID theMethod = (*jni)->GetMethodID(jni, theClass, methodName, methodSignature);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_method_id(%p, %s, %s, %s): Exception pending error", jni, className, methodName, methodSignature);
	
	return theMethod;
}

static jfieldID KnShim_GetFieldID(JNIEnv *jni, const char *className, const char *fieldName, const char *fieldSignature) {
	jclass theClass = (*jni)->FindClass(jni, className);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_field_id(%p, %s, %s, %s): Exception pending error", jni, className, fieldName, fieldSignature);
	
	jfieldID theField = (*jni)->GetFieldID(jni, theClass, fieldName, fieldSignature);
	
	JNI_EXCEPTION_ABORT(jni, "jni_get_field_id(%p, %s, %s, %s): Exception pending error", jni, className, fieldName, fieldSignature);
	
	return theField;
}

#define JNI_GET_ENV() \
	JavaVM *vm = gApp->activity->vm; \
	JNIEnv *jni = NULL; \
	\
	if ((*vm)->GetEnv(vm, (void **) &jni, JNI_VERSION_1_6) != JNI_OK) { \
		LogF("The JNI is not okay!"); \
		abort(); \
	}

char *KnShim_GetPackageCodePath(void) {
	/**
	 * Get the path to the application's primary APK (which is itself a ZIP
	 * file). Similar to Context#getPackageCodePath.
	 */
	
	JNI_GET_ENV();
	
	jobject nativeActivity = gApp->activity->clazz;
	jmethodID getPackageCodePath = KnShim_GetMethodID(jni, "android/app/NativeActivity", "getPackageCodePath", "()Ljava/lang/String;");
	
	jstring codePathObj = (*jni)->CallObjectMethod(jni, nativeActivity, getPackageCodePath);
	JNI_EXCEPTION_ABORT(jni, "pending exception after nativeActivity.getPackageName()");
	
	const char *codePathChars = (*jni)->GetStringUTFChars(jni, codePathObj, NULL);
	
	if (!codePathChars) {
		return NULL;
	}
	
	char *result = strdup(codePathChars);
	
	(*jni)->ReleaseStringUTFChars(jni, codePathObj, codePathChars);
	
	return result;
}
