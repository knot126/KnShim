# Knot's Shim Docs

> **Note**
> 
> This documentation is, and always will be, incomplete.

## Known bugs

Unfortunately, KnShim introduces a few bugs to *Smash Hit* due to the way it is loaded.

* On some devices, paticularly emulators, Smash Hit won't be able to open *at all*.
  * If you are using an emulator or an x86-based device, this probably means ARM emulation is borked rather than an issue with KnShim. Try using a different emulator.
  * If you are using a real, non-x86 device, then there is likely some other issue which needs to be fixed. Please report this issue in KnShim's GitHub repo.

## Logging

One new function is provided: `knLog([level], msg)`. It logs to the android debug stream, accessible with `adb logcat`. Level can be any one of:

* `LOG_INFO`
* `LOG_WARN`
* `LOG_ERROR`

Or not included at all for a default of `LOG_INFO`, and `msg` is any string to log. For example:

```lua
knLog(LOG_INFO, "Hello, world!")
knLog("What a wonderful day to mod Smash Hit!") -- defaults to LOG_INFO
knLog(LOG_ERROR, "Whoops! Crashed.")
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

### `knMakeDir(path)`

Creates the directory at `path` using `mkdir(path, 0777)`. Returns `true` on success, `false` on failure. Note that if the directory already exists, this will return `false`.

### `knListDir(path)`

Return a list of file names in the directory as a table, not including the special entries `.` and `..`.

### `knIsDir(path)`

Check if the file system node at `path` is a directory and is readable. Return `true` if it is, and `false` if it is not.

### `knLoadAsset(path)`

Load the contents of an asset from the APK's assets directory. This does not use Smash Hit's asset manager, so some things will not work (e.g. trying to load an .gz.mp3 will not decompress it automatically), though for convience it will also try loading the path with a `.mp3` suffix if the initial path fails to load.

Returns the contents of the asset as a string, or `nil` if the asset could not be loaded.

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

## Database

The database is essentially the same as the registry, except it is saved across restarts of the game and is suitible for things like save games.

The database saves all data to a file named `database.kn` in the user data folder.

### `knDbSet(key, value)`

Create a mapping from the `key` to the `value`, and save the database.

Returns `true` if the mapping was created and the database was saved, or `false` if either the mapping was not created or the database was not saved successfully.

### `knDbGet(key)`

Return the value assocaited with the key.

### `knDbHas(key)`

Return `true` if there is a mapping of the given key in the database, or `false` if there is not.

### `knDbDelete(key)`

Delete the mapping assocaited with the key, and save the database.

Returns `true` if the database was saved successfully, or `false` if it was not.

## Game Control

The game control features allow you to control aspects of the gameplay and use internal utility functions directly from Lua.

### Cheats

#### `knSetBalls(balls)`

Set the player's number of balls to `balls`.

For example, to set the player's number of balls to 100:

```lua
knSetBalls(100)
```

#### `knGetBalls()`

Gets the current number of balls. This is different from using mgGet(), since it is updated even if you use knSetBalls().

#### `knSetStreak(streak)`

Set the player's streak.

For example, to set a four-ball multiball plus halfway to a five-ball multiball:

```lua
knSetStreak(35)
```

#### `knGetStreak()`

Gets the current streak. This is different from using mgGet(), since it is updated even if you use knSetStreak().

#### `knSetNoclip(mode)`

If mode is `true`, then the noclip cheat is enabled if not already enabled. If mode is `false`, then noclip is disabled if not already disabled.

#### `knGetNoclip()`

Return `true` if currently in noclip mode, or `false` otherwise.

### Level

#### `knLevelHitSomething()`

Causes the player to crash and loose balls.

#### `knLevelStreakAbort()`

Aborts the player's streak "properly", e.g. plays the sound in addition to dropping the streak.

#### `knLevelStreakInc()`

Increments the player's streak "properly".

#### `knLevelAddScore(score)`

Adds balls to the player "properly".

### Internal HTTP function wrappers

#### `knDownloadFile(url, path)`

This is a Lua wrapper of `HttpThread::downloadFile()`. It downloads a file from an HTTP URL and saves it at the path. The path is a resource manager path, so prepending it with `knGetInternalDataPath()` is not needed.

Note that this function is blocking, so the game will freeze until the request completes.

Returns true on success, or false on failure.

##### Example

```lua
if knDownloadFile("http://myserver.com/mylevel.zip", "mylevel.zip") then
    -- success
else
    -- failure
end
```

#### `knHttpPost(url, data)`

This is a Lua wrapper of `ResMan::httpPost()`. It sends an HTTP POST request to the given URL with the given data. The HTTP `Content-Type` header will always be `application/octet-stream`.

Note that this function is blocking, so the game will freeze until the request completes.

Returns true on success, or false on failure.

##### Example

```lua
if knHttpPost("http://myserver.com/highscore/", "score=12345") then
    -- success
else
    -- failure
end
```

### Asset server

#### `knConnectAssetServer(host, timeout)`

Connects to the Smash Hit asset server at `host`, waiting up to `timeout` seconds to make a connection. Returns `true` on success, or `false` on failure.

#### `knDisconnectAssetServer()`

Disconnect from the current asset server.

#### `knIsConnectedToAssetServer()`

Returns `true` if an asset server is currently connected, for `false` if one is not.

### Menu reloading

#### `knEnableReloading()`

Install the hooks required to use `knReload()`. This only needs to be called once per game launch.

#### `knReload()`

If on the main menu, this reloads the main menu on the next frame. The main menu script will continue to run as normal until then.

If in a level, this is nearly equivlent to `mgCommand("level.restart")`.

## System and Misc utilities

### `knGetShimVersion()`

Return the shim version as an integer. Newer versions MUST always return higher values than older ones.

### `knGetInternalDataPath()`

Return the absolute path to the internal data directory (where the savegames and the like are stored - equivlent to `user://`) with no trailing slash.

### `knGetExternalDataPath()`

Return the absolute path to the external data directory. This isn't used for anything in the game but is provided by Android so it's included for completeness.

### `knInclude(path)`

Similar to lua's `dofile()` but loads from the APK's asset directory.

#### Using lua libraries

If you have a pure lua library you would like to use with Smash Hit, you could download the file, put it in a folder where you keep lua libraries, then `knInclude()` it.

For example, let's say we want to use a JSON parser library with the file name `json.lua`. You could copy that to a file in your assets directory called `lualibs/json.lua.mp3`, then load it with:

```lua
local json = knInclude("lualibs/json.lua")
```

If your library is multipule files, you might need to fix up the `require` calls to use `knInclude` and use `assets`-relative paths.

## HTTP

The HTTP extension allows more advanced HTTP requests, though it is still limited.

**Note:** Due to the limited HTTP library being used, this does not support custom request headers or reading response headers and may have unexpected behaviour when network errors occur.

The recommended way to use this library is to create two functions around your request: one to start it and another to process it. When you want to start an HTTP request you would call your custom `startRequest()` function, and you would call your custom `processRequest()` from a function that runs regularly like `drawWorld()`.

### Example

Here is an outline for submitting a high score to a server:

```lua
function handleCommand(cmd)
    -- ...
    
    -- Start the request when the 'submitscore' command is run
    if cmd == "submitscore" then
        startHighScoreSubmission()
    end
    
    -- ...
end

function startHighScoreSubmission()
    highScoreRequest = knHttpRequest("http://myserver.com/highscore/", getHighScore())
    
    if not highScoreRequest then
        -- handle failing to initialise the request
    end
end

function processHighScoreSubmission()
    if highScoreRequest then
        -- update the request with recieved data and get the status
        local status = knHttpUpdate(highScoreRequest)
        
        if status == KN_HTTP_PENDING then
            -- we can't really do anything while pending, leave everything as is
        else
            if status == KN_HTTP_ERROR then
                -- handle the error case, maybe show a dialogue to the user
                -- about the error
            else
                local data = knHttpData(highScoreRequest)
                -- handle the successful case with the response data, maybe
                -- show something to confirm the score was submitted. note that
                -- if your server returns something like 200 OK for certian
                -- types of errors this will still technically be an error and
                -- you will want to handle that accordingly
            end
            
            -- Since the request is finished, don't hang on to the object
            -- anymore and replace it with nil so we know it's no longer needed
            highScoreRequest = nil
        end
    end
end
```

### `knHttpRequest(url, [data])`

Initiate an GET or POST request to the given URL. If `data` is specified, then this is assumed to be a POST request, where data is the POST body. If not, this is assumed to be a GET request.

Returns a value of type `userdata` (the request object) on success or `nil` on failure.

Note: `data` is allowed to contain embedded zeros.

### `knHttpUpdate(request)`

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

**Note:** As of Shim r12, knHttpRelease() is now automatically called when request objects are garbage collected. knHttpRelease() can still be used to collect the bulk of their contents immediately, but is no longer required.

## Overlays

> **Warnings**
> 
> * Overlays are only meant for Hyperspace at the moment, and they aren't in a normal build of KnShim.
> * Gzip-compressed assets (`.gz.mp3`) are not supported. The assets can still be compressed within the ZIP file itself.
> * Files must not have the `.mp3` suffix inside the ZIP.

Overlays allow you to load assets from a ZIP file as if it were the assets directory, loading assets from the mod's real assets directory if not found in the ZIP file.

For example, if you mount a zip file `forest.zip` which has a level:

```
forest.zip
    levels/
        forest.xml
    rooms/
        forest.lua
    segments/
        forest.xml
```

then playing the level `forest` will load from the ZIP file.

If a segment then depends on an obstacle like `scoretop`, but it is not in the ZIP file, then it is loaded from the mod's assets directory.

### `knMountOverlay(path)`

Mounts an overlay given the ZIP file path. Returns `false` on failure, or `true` on success.

### `knUnmountOverlay()`

Unmounts the currently mounted overlay. Returns `false` if an overlay wasn't mounted, or `true` if an overlay was mounted and has been unmounted.

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
