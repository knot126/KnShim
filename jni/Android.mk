LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE  := arm
LOCAL_MODULE    := shim
LOCAL_SRC_FILES := util.c shim.c script.c log.c peekpoke.c http.c system.c reg.c nxarchive.c files.c gamectl.c obfuscate.c debuglog.c lua/lapi.c lua/lcode.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/ltable.c lua/ltm.c lua/lundump.c lua/lvm.c lua/lzio.c lua/lauxlib.c lua/lbaselib.c lua/ldblib.c lua/liolib.c lua/lmathlib.c lua/loslib.c lua/ltablib.c lua/lstrlib.c lua/loadlib.c lua/linit.c
LOCAL_LDLIBS    := -ldl -llog -landroid
LOCAL_STATIC_LIBRARIES := android_native_app_glue

# Uncomment this to enable obfuscation
LOCAL_CFLAGS    += -DBUILD_CIPHER=1

# Uncomment these to enable hyperspace extensions
# LOCAL_SRC_FILES += overlay.c extern/miniz.c
# LOCAL_CFLAGS    += -DHYPERSPACE=1

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
