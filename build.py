#!/usr/bin/env python3
import os
import pathlib
from datetime import datetime

pathlib.Path("jni/build_date.h").write_text(f"#define SHIM_BUILD_DATE {datetime.today().strftime('%Y%m%d')}")
os.system("ndk-build")
