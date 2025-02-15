# Release 12 (current development version)

* Added `knListDir`, `knIsDir`, `knLoadAsset`, `knInclude`, `knHttpPostAsync`
* `knLog` no longer requires log level, does not crash when `msg` is not a string
* HTTP request objects will now be properly garbage collected, so `knHttpRelease` is not needed anymore
* Code cleanup
* Remove obfuscation related code
* Untested x86 support
