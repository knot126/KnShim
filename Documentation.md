# Knot's Shim Docs

> **Note**
> 
> This documentation is, and always will be, incomplete.

## Installation

> **Note**
> 
> This only applies to the native loader variant of the shim, not the one that uses Leaf. If you use the leaf variant, then instead of step two, you will need to copy the files in `lib` to a folder named `assets/native`, then you will need to delete the folders in `lib` and place the folders from `libs` in the shim there.

To start using the shim in your mod, you will either need to download prebuilt binaries or build it yourself using the Android NDK. We will assume you've downloaded the ZIP file for whatever the latest version is.

**Step 1.** You will need to patch `libsmashhit.so` for both architectures so that anti-tamper is removed. You can use Shatter's libsmashhit patching feature to do this.

**Step 2.** Copy the library to the APK. Open the `lib` folder in your APK (the one where you should see subfolders with names like `arm64-v8a` and `armeabi-v7a`). Then, open the ZIP file to the folder that has the same structure as the `lib` folder (e.g. `arm64-v8a` and etc), and copy those folders into the `lib` folder, merging them.

When you're done with this step, every subfolder in `lib` should have two files: `libshim.so` and `libsmashhit.so`.

**Step 3.** You need to change it so that `libshim.so` is loaded *before* `libsmashhit.so`. Open your APK's `AndroidManifest.xml` in a text editor and look for this line:

```xml
<meta-data android:name="android.app.lib_name" android:value="smashhit"/>
```

> **Hint**: it's probably line 11

Now change `smashhit` to `shim`: 

```xml
<meta-data android:name="android.app.lib_name" android:value="shim"/>
```

... save it (and your APK), and you're done! You should now have access to all of the functions provided below in any Lua script.

## Logging

One new function is provided: `knLog(level, msg)`. It logs to the android debug stream, accessible with `adb logcat`. Level can be any one of:

* `LOG_INFO`
* `LOG_WARN`
* `LOG_ERROR`

And `msg` is any string to log. For example:

```lua
knLog(LOG_INFO, "Hello, world!")
```

## Files

The shim provides a few extra functions to make working with files easier.

### `knWriteFile(path, content)`

Write a file with the given contents, which may contain embedded zeros. Returns true on success, or false on failure.

### `knReadFile(path)`

Read the contents of the given file. Returns a string with the contents on success, even if the file is empty, or nil on failure.

### `knRenameFile(oldPath, newPath)`

Renames the file at `oldPath` to `newPath`, moving the file if needed. Returns `true` on success and `false` on failure.

### `knDeleteFile(path)`

Deletes the file at the given path. Returns `true` on success and `false` on failure.

### `knIsFile(path)`

Check if the file at the given path exists and can be read. Returns `true` if so, or `false` if not.

### `knMakeDir(dirName)`

Creates the directory at `dirName` using `mkdir(dirName, 0777)`. Returns `true` on success, `false` on failure. Note that if the directory already exists, this will return `false`.

## Registry

The registry is functionally similar to `mgSet` and `mgGet` but you are able to write any values you want.

### `knRegSet(key, value)`

Writes or replaces the value assocaited with `key` with the given value. Returns boolean indicating success.

### `knRegGet(key)`

Get the value assocaited with `key` from the registry.

### `knRegHas(key)`

Checks if the registry has a given key. Return `true` if there is a value assocaited with the given `key`, otherwise return `false`.

### `knRegDelete(key)`

Removes a given key-value pair from the registry, when given its key.

### `knRegKeys()`

Returns an array-like table containing all of the keys in the registry.

## Game Control

The game control features allow you to control aspects of the gameplay.

### `knSetBalls(balls)`

Set the player's number of balls to `balls`.

For example, to set the player's number of balls to 100:

```lua
knSetBalls(100)
```

### `knSetStreak(streak)`

Set the player's streak.

For example, to set a four-ball multiball plus halfway to a five-ball multiball:

```lua
knSetStreak(35)
```

### `knGetBalls()`

Gets the current number of balls. This is different from using mgGet(), since it is updated even if you use knSetBalls().

### `knGetStreak()`

Gets the current streak. This is different from using mgGet(), since it is updated even if you use knSetStreak().

### `knSetNoclip(mode)`

If mode is `true`, then the noclip cheat is enabled if not already enabled. If mode is `false`, then noclip is disabled if not already disabled.

### `knGetNoclip()`

Return `true` if currently in noclip mode, or `false` otherwise.

### `knLevelHitSomething()`

Causes the player to crash and loose balls.

### `knLevelStreakAbort()`

Aborts the player's streak "properly", e.g. plays the sound in addition to dropping the streak.

### `knLevelStreakInc()`

Increments the player's streak "properly".

### `knLevelAddScore(score)`

Adds balls to the player "properly".

## System and Misc utilities

### `knGetShimVersion()`

Return the shim version as an integer. Newer versions MUST always return higher values than older ones.

### `knGetInternalDataPath()`

Return the absolute path to the internal data directory (where the savegames and the like are stored - equivlent to `user://`) with no trailing slash.

### `knGetExternalDataPath()`

Return the absolute path to the external data directory. This isn't used for anything in the game but is provided by Android so it's included for completeness.

## HTTP

The HTTP extension allows making non-blocking HTTP requests.

**Note:** Due to the limited HTTP library being used, this does not support custom request headers or reading response headers and may have unexpected behaviour when network errors occur.

### `knHttpRequest(url, [data])`

Initiate an GET or POST request to the given URL. If `data` is specified, then this is assumed to be a POST request, where data is the POST body. If not, this is assumed to be a GET request.

Returns a value of type `userdata` (the request object) on success or `nil` on failure.

Note: `data` is allowed to contain embedded zeros.

### `knHttpRequest(request)`

Reads any new data and further process the request, possibly finalising it. Returns:

* `KN_HTTP_PENDING` if the request is still pending;
* `KN_HTTP_ERROR` if the request has finished in error (status > 400 or network errors);
* `KN_HTTP_DONE` if the request has succeeded.

This function must be called in a function like `tick()` or `draw()` (that is, every so often) until it no longer returns `KN_HTTP_PENDING`. When it does finish, it is recommended to do any processing, then release the request.

**Note:** Request memory is not automatically released and results in a memory leak if not manually freed, since I don't want to fuck with metatables ATM. Hopefully this will change later as it is stupid.

### `knHttpData(request)`

Returns the response data for a finished request as a string.

### `knHttpDataSize(request)`

Returns the size of the response data in bytes.

### `knHttpContentType(request)`

Returns the MIME type of the response data for a finished request, as a string.

### `knHttpError(request)`

Return a string describing the HTTP error, or nil if there is none. Note that even if there is an error this may return nil, for example due to a lower-level network error.

### `knHttpErrorCode(request)`

Return an integer which is the HTTP status code of the response, or `0` if there is not one. This may be `0` even if there was some kind of error.

### `knHttpRelease(request)`

Release the memory associated with an HTTP request. Any further functions called on this request may return nil regardless of their documentation.

**Tip:** A common pattern for global request objects is to the variable they were stored in to nil after releasing them:

```lua
function finishRequest()
    someImportantThing = knHttpData(globalRequestObject)
    knHttpRelease(globalRequestObject)
    globalRequestObject = nil
end
```

## PeekPoke

The PeekPoke functions provide a way to read and write raw memory. This is probably the most powerful module as it basically allows making any changes to the game at runtime.

**Note:** Keep in mind that using this extension requires some knowledge of the phone's hardware details which can even vary (for example, ARMv7 vs ARMv8).

### Types

PeekPoke has some concept of typing so that you do not have to use bytes for every peek or poke operation. The following types are supported:

| Type | C Type | Lua type |
| ---- | ------ | -------- |
| `KN_TYPE_ADDR` | `void *` | `integer` |
| `KN_TYPE_BOOL` | `bool` (as `char`) | `boolean` |
| `KN_TYPE_SHORT` | `short` | `integer` |
| `KN_TYPE_INT` | `int` | `integer` |
| `KN_TYPE_FLOAT` | `float` | `number` |
| `KN_TYPE_STRING` | `char *` | `string` (no embedded `0`s) |
| `KN_TYPE_BYTES` | `void *` + size | `string` (allows embedded `0`s) |

### `knSymbolAddr(symbolName)`

Get the address of the symbol named by `symbolName`. This is useful for finding pointers to functions and global game data which can be used as a basis for reading and writing values in memory.

Returns the address assocaited with the symbol as an integer.

### `knPeek(addr, type, [size])`

Read a value of any Supported Type from `addr`. The size argument is required when the types is bytes and is the number of bytes to read from memory. Note that passing invalid memory addresses will result in a crash.

Returns the value requested.

### `knPoke(addr, type, value)`

Write the value of any Supported Type to the address `addr`. Note that passing an invalid memory address will result in a crash, and even writing to valid memory addresses which you have access to may still crash the game if it corrupts structures.

Returns the address of memory written on success or `nil` on failure.

### `knSystemAbi()`

Returns the CPU/ABI that this system is using as a string.

| ABI | Result value |
| --- | ------------ |
| ARMv8/64-bit | `arm64-v8a` |
| ARMv7/32-bit | `armeabi-v7a` |
| Others | `unknown` |

### `knInvertBranch(addr)`

Invert the conditional branch (contional jump) at `addr` such that it preforms the logical *NOT* of the comparison it is making. Note that this does NOT work for unconditional branches.

#### Implementation differences

| Arch | Notes |
| ---- | ----- |
| ARMv8 | On ARM v8, this function can invert branches of the forms `B.cond` and `BC.cond` (i.e. immidiate branches) only. |
| ARMv7 | On ARM v7, this function can be used to invert any conditional instruction. |
