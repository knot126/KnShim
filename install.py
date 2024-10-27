#!/usr/bin/env python3
import argparse
import shutil
import os
from pathlib import Path

LIB_DIR = str(Path(__file__).parent) + "/libs"

def install(path):
	shutil.copytree(LIB_DIR, f"{path}/lib", dirs_exist_ok=True)
	amx = Path(f"{path}/AndroidManifest.xml")
	amx.write_bytes(amx.read_bytes().replace(b'android:name="android.app.lib_name" android:value="smashhit"', b'android:name="android.app.lib_name" android:value="shim"'))
	
	try:
		import patcher
		
		try:
			patcher.patch_binary(f"{path}/lib/armeabi-v7a/libsmashhit.so", {"antitamper": []})
			patcher.patch_binary(f"{path}/lib/arm64-v8a/libsmashhit.so", {"antitamper": []})
		except:
			print("Warning: Failed to patch some binaries.")
	except ImportError:
		print("Note: The patcher could not be found, so you will need to patch libsmashhit.so against anti-tamper manually.")

def main():
	args = argparse.ArgumentParser()
	args.add_argument("file", help="Install KnShim to an extracted Smash Hit APK directory")
	args = args.parse_args()
	
	install(args.file)

if __name__ == "__main__":
	main()
