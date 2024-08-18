#include <stdio.h>
#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

int knWriteFile(lua_State *script) {
    /**
     * (nil|boolean) success = knWriteFile((string) path, (string) contents)
     * 
     * Write a files contents to the given path. Returns either nil or false on
     * failure or true on success.
     */
    
    if (lua_gettop(script) < 2) {
        return 0;
    }
    
    const char *path = lua_tostring(script, 1);
    size_t size;
    const char *data = lua_tolstring(script, 2, &size);
    
    if (!path || !data) {
        return 0;
    }
    
    FILE *file = fopen(path, "wb");
    
    if (!file) {
        return 0;
    }
    
    size_t written = fwrite(data, 1, size, file);
    
    fclose(file);
    
    lua_pushboolean(script, written == size);
    return 1;
}

int knReadFile(lua_State *script) {
    /**
     * (string|nil) contents = knReadFile((string) path)
     * 
     * Read the file at path, return nil if failed or empty string if not.
     */
    
    if (lua_gettop(script) < 1) {
        return 0;
    }
    
    const char *path = lua_tostring(script, 1);
    
    if (!path) {
        return 0;
    }
    
    FILE *file = fopen(path, "rb");
    
    if (!file) {
        return 0;
    }
    
    // Return codes? Who cares :P
    // Reads the size of the file since there's no way to do this normally
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Empty strings can take a shortcut, also bypasses the behaviour of malloc
    // not really being defined.
    if (size == 0) {
        lua_pushliteral(script, "");
        fclose(file);
        return 1;
    }
    
    // Allocate memory for contents
    char *data = malloc(size);
    
    if (!data) {
        fclose(file);
        return 0;
    }
    
    // Read contents into memory
    size_t readen = fread(data, 1, size, file);
    fclose(file); // close the file
    
    // Push it as lua data
    if (readen == size) {
        lua_pushlstring(script, data, size);
    }
    else {
        lua_pushnil(script);
    }
    
    free(data); // always free the data
    
    return 1;
}

int knEnableFile(lua_State *script) {
    lua_register(script, "knWriteFile", knWriteFile);
    lua_register(script, "knReadFile", knReadFile);
    
    return 0;
}
