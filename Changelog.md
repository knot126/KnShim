# Changelog

## Release 13 (next version)

* Added `knGetAppSdk` for getting the app's target SDK and `knGetDeviceSdk` for getting the device's android version (sdk level)
* Added `knJavaCommand` for calling smash hit's java commands from lua
* Added `knGetDeviceHz` and `knSetFrameRate`
* Better HTTP API; replaced `knHttpContentType()` with `knHttpGetHeader()`
* Removed `knEnableReloading`; you can now just `knReload()` without enabling it
* Fix a bug where the game would crash if using `knHttpRelease()` explicitly

## Release 12

* Added `knListDir`, `knIsDir`, `knLoadAsset`, `knInclude`, `knHttpPostAsync`
* `knLog` no longer requires log level, does not crash when `msg` is not a string
* HTTP request objects will now be properly garbage collected, so `knHttpRelease` is not needed anymore
* Code cleanup
* Remove obfuscation related code
* Untested x86 support
