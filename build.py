#!/usr/bin/env python3
import os
import shutil
import sys
from datetime import datetime
from pathlib import Path

if "--help" in sys.argv:
	print(f"""{sys.argv[0]} [OPTIONS] -- build KnShim

Options:
    --game <game id>    Build KnShim for a certian game. Available games are
                        "smashhit" and "grannysmith".
    --no-regen-header   Do not regenerate headers
    --package <version> Package the shim with the version number <version>.
    --no-tls            Disable HTTPS support and don't build MbedTLS
    --upgrade           Automatically upgrade apk open in apk editor studio
""")
	sys.exit()

game = "smashhit" if "--game" not in sys.argv else sys.argv[sys.argv.index("--game")+1]

if "--no-regen-header" not in sys.argv:
	if "--package" in sys.argv:
		version = sys.argv[sys.argv.index("--package")+1]
		new_data = f"#define SHIM_VERSION \"{version}\"\n"
		Path("jni/shim/version.h").write_text(new_data)
	
	with open("jni/shim/modules.txt", "r") as f:
		enum = ""
		enables = ""
		pushenum = ""
		i = 0
		
		for line in f.readlines():
			name = line.strip().split()[0]
			
			if game in line:
				enum += f"\tKN_{name.upper()}_BIT = (1 << {i}),\n"
				enables += f"\tint knEnable{name}(lua_State *script);\\\n"
				enables += f"\tif ((gDisabledModules & KN_{name.upper()}_BIT) == 0) {{ knEnable{name}(script); }}\\\n"
				pushenum += f"\tknLuaPushEnum(script, KN_{name.upper()}_BIT);\\\n"
				i += 1
		
		Path("jni/shim/enablement.h").write_text(f"""enum {{
{enum}}};

#define KNSHIM_ENABLE() \\
{enables}

#define KNSHIM_PUSH_ENABLE_ENUM() \\
{pushenum}""")

# Kill me
granny_define = "LOCAL_CFLAGS += -DGRANNY"

if game == "grannysmith":
	mk = Path("jni/Android.mk").read_text()
	
	if f"# {granny_define}" in mk:
		Path("jni/Android.mk").write_text(mk.replace(f"# {granny_define}", granny_define))
else:
	mk = Path("jni/Android.mk").read_text()
	
	if f"# {granny_define}" not in mk:
		Path("jni/Android.mk").write_text(mk.replace(granny_define, f"# {granny_define}"))

ndk_build_args = ""

if "--no-tls" in sys.argv:
	ndk_build_args += " DISABLE_TLS=true"

status = os.system(f"ndk-build{ndk_build_args}")

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
		shutil.make_archive(f"knshim-r{version}-{game}-libs", "zip", "./libs")
