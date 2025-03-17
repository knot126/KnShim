#!/usr/bin/env python3
import os
import pathlib
import shutil
import sys
from datetime import datetime

# Write new build date
pathlib.Path("jni/build_date.h").write_text(f"#define SHIM_BUILD_DATE {datetime.today().strftime('%Y%m%d')}")

# Build
status = os.system("ndk-build")

# Copy to test apk
if not status and len(sys.argv) > 1 and sys.argv[1] == "--upgrade":
	apks = os.listdir("/tmp/apk-editor-studio/apk")
	
	if len(apks) > 0:
		apk_path = f"/tmp/apk-editor-studio/apk/{apks[0]}"
		print(f"Upgrade apk at {apk_path}")
		shutil.copytree("./libs", f"{apk_path}/lib", dirs_exist_ok=True)
