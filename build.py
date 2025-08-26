#!/usr/bin/env python3
import os
import shutil
import sys
from datetime import datetime
from pathlib import Path

# Write new build date
if "--no-regen-header" not in sys.argv:
	new_data = f"#define SHIM_BUILD_DATE {datetime.today().strftime('%Y%m%d')}\n"
	Path("jni/build_date.h").write_text(new_data)

# Build
status = os.system("ndk-build")

# Copy to test apk
if not status and len(sys.argv) > 1 and "--upgrade" in sys.argv:
	apks = os.listdir("/tmp/apk-editor-studio/apk")
	
	if len(apks) > 0:
		apk_path = f"/tmp/apk-editor-studio/apk/{apks[0]}"
		print(f"Upgrade apk at {apk_path}")
		shutil.copytree("./libs", f"{apk_path}/lib", dirs_exist_ok=True)
	else:
		print(f"No APKs to upgrade")
