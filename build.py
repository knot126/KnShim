#!/usr/bin/env python3
import os
import shutil
import sys
from datetime import datetime
from pathlib import Path

game = "G" if "--granny" in sys.argv else "S"

if "--no-regen-header" not in sys.argv:
	if "--package" in sys.argv:
		version = sys.argv[sys.argv.index("--package")+1]
		new_data = f"#define SHIM_VERSION \"{version}\"\n"
		Path("jni/version.h").write_text(new_data)
	
	with open("jni/modules.txt", "r") as f:
		enum = ""
		enables = ""
		pushenum = ""
		i = 0
		
		for line in f.readlines():
			info = line.strip().split()
			name = info[0]
			
			if game in info[1]:
				enum += f"\tKN_{name.upper()}_BIT = (1 << {i}),\n"
				enables += f"\tint knEnable{name}(lua_State *script);\\\n"
				enables += f"\tif ((gDisabledModules & KN_{name.upper()}_BIT) == 0) {{ knEnable{name}(script); }}\\\n"
				pushenum += f"\tknLuaPushEnum(script, KN_{name.upper()}_BIT);\\\n"
				i += 1
		
		Path("jni/enablement.h").write_text(f"""enum {{
{enum}}};

#define KNSHIM_ENABLE() \\
{enables}

#define KNSHIM_PUSH_ENABLE_ENUM() \\
{pushenum}""")

# Kill me
granny_define = "LOCAL_CFLAGS += -DGRANNY"

if "--granny" in sys.argv:
	mk = Path("jni/Android.mk").read_text()
	
	if f"# {granny_define}" in mk:
		Path("jni/Android.mk").write_text(mk.replace(f"# {granny_define}", granny_define))
else:
	mk = Path("jni/Android.mk").read_text()
	
	if f"# {granny_define}" not in mk:
		Path("jni/Android.mk").write_text(mk.replace(granny_define, f"# {granny_define}"))

status = os.system("ndk-build")

if not status:
	if "--upgrade" in sys.argv:
		apks = os.listdir("/tmp/apk-editor-studio/apk")
		
		if len(apks) > 0:
			apk_path = f"/tmp/apk-editor-studio/apk/{apks[0]}"
			print(f"Upgrade apk at {apk_path}")
			shutil.copytree("./libs", f"{apk_path}/lib", dirs_exist_ok=True)
		else:
			print(f"No APKs to upgrade")
	
	if "--package" in sys.argv:
		version = sys.argv[sys.argv.index("--package")+1]
		shutil.make_archive(f"knshim-r{version}-libs", "zip", "./libs")
