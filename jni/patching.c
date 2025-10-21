/**
 * Memory patching utilities
 */

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"

int knPatch(lua_State *script) {
	/**
	 * (bool) success = knPatch((int) vaddr, (string) data)
	 */
	
	if (lua_gettop(script) < 2) {
		lua_pushboolean(script, 0);
		return 1;
	}
	
	size_t vaddr = lua_tointeger(script, 1);
	
	size_t size;
	const char *data = lua_tolstring(script, 2, &size);
	
	lua_pushboolean(script, data && KNPatch(vaddr, data, size));
	return 1;
}

int knEnablePatching(lua_State *script) {
	knRegisterFunc(script, knPatch);
	
	return 0;
}
