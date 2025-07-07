# Changelog

## Release 13 (next version)

* KnShim now uses the version of Lua from Smash Hit instead of including its own
  * This fixed various bugs, including tables being used in `knInclude()`'d scripts leading to memory corruption
* Added functions:
  * `knGetAppSdk()` for getting the app's target SDK
  * `knGetDeviceSdk()` for getting the device's android version (sdk level)
  * `knGetAppVersion()` for getting the app's version string
  * `knJavaCommand()` for calling smash hit's java commands from lua
  * `knGetDeviceHz()` and `knSetFrameRate()` for adjusting the game's framerate
  * `knPatch()` for more easily making patches at runtime
* Changed `knLoadAsset()` and `knInclude()` to use Smash Hit's resource manager
  * This means you can no longer explicitly specify `.mp3` at the end of file names
* HTTP API now supporting headers and non-`GET`/`POST` methods
* Removed `knEnableReloading`; you can now just `knReload()` without enabling it
* Deprecated `knPeek()` and `knPoke()`, use `knPatch()` now
* Fix a bug where the game would crash if using `knHttpRelease()` explicitly
* Overlays have been made available for general use
* Upgrade Leaf
* New project logo

## Release 12

* Added `knListDir`, `knIsDir`, `knLoadAsset`, `knInclude`, `knHttpPostAsync`
* `knLog` no longer requires log level, does not crash when `msg` is not a string
* HTTP request objects will now be properly garbage collected, so `knHttpRelease` is not needed anymore
* Code cleanup
* Remove obfuscation related code
* Untested x86 support
