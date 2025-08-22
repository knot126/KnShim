LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := shim
LOCAL_SRC_FILES := util.c asset.c main.c script.c log.c patching.c http.c system.c reg.c nxarchive.c files.c gamectl.c overlay.c lua/loader.c extern/miniz.c
LOCAL_LDLIBS    := -ldl -llog -landroid
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
