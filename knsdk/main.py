"""
This file is part of KnShim. Copyright (c) 2025 Knot126.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import argparse
import sys
import os
import tomllib
import shutil

from pathlib import Path

SDK_DIR = str(Path(__file__).parent.parent)

def create():
	print("Welcome to the KnShim SDK extension creator.")
	print("Please fill in some information about your project.")
	print()
	game = input("Game name: ")
	name = input("Project name: ")
	desc = input("Description: ")
	print()
	
	print("Generate info...")
	os.makedirs(f"jni/{name}", exist_ok=True)
	
	Path(f"jni/{name}/extinfo.c").write_text(f"""// Automatically generated information about this module.

const char ModName[] = "{name.encode('unicode-escape').decode('utf-8')}";
const char ModDescription[] = "{desc.encode('unicode-escape').decode('utf-8')}";
const char ModGame[] = "{game.encode('unicode-escape').decode('utf-8')}";
const int ModVersion = 10000;
""")
	
	Path(f"jni/{name}/main.c").write_text("#include <knshim/knshim.h>\n\nconst char *ModInit(void) {\n\treturn NULL;\n}")
	
	print("Generate makefiles...")
	Path(f"jni/Application.mk").write_text("APP_ABI := arm64-v8a armeabi-v7a\nAPP_PLATFORM := android-26\n")
	Path(f"jni/Android.mk").write_text(f"""LOCAL_PATH := $(call my-dir)

# Setup KnShim related stuff
include $(CLEAR_VARS)
LOCAL_MODULE := shim-prebuilt
LOCAL_SRC_FILES := shim/$(TARGET_ARCH_ABI)/libshim.so
include $(PREBUILT_SHARED_LIBRARY)

# This is the setup for YOUR project!
include $(CLEAR_VARS)

LOCAL_ARM_MODE  := arm

# Your module's name
LOCAL_MODULE    := {name}

# The source files for your module. Don't remove extinfo.c; it's required!
LOCAL_SRC_FILES := {name}/extinfo.c \\\n\t{name}/main.c

# Link against any extra libraries you might need here
# LOCAL_LDLIBS     := -llog -landroid -lGLESv2

# Link against KnShim itself
LOCAL_SHARED_LIBRARIES := shim-prebuilt

# Include KnShim's headers
LOCAL_C_INCLUDES := shim

# Consider providing C flags
# LOCAL_CFLAGS     := -DDUMMY

include $(BUILD_SHARED_LIBRARY)""")
	
	print("Copy pre-built libraries and headers...")
	os.makedirs("jni/shim/knshim", exist_ok=True)
	shutil.copyfile(f"{SDK_DIR}/jni/shim/knshim.h", f"jni/shim/knshim/knshim.h")
	shutil.copytree(f"{SDK_DIR}/libs/", f"jni/shim", dirs_exist_ok=True)

def build():
	os.system('ndk-build')

def main():
	if len(sys.argv) < 2:
		print("Second argument should be one of: 'create', 'build', 'update'")
		return
	
	match sys.argv[1]:
		case 'create':
			create()
		case 'build':
			build()

if __name__ == "__main__":
	main()
