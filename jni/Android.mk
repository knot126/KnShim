LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := shim
LOCAL_SRC_FILES := util.c asset.c main.c script.c log.c patching.c http.c system.c reg.c nxarchive.c files.c gamectl.c overlay.c shaders.c input.c draw.c lua/loader.c extern/miniz.c
LOCAL_LDLIBS    := -ldl -llog -landroid -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_C_INCLUDES := extern

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
