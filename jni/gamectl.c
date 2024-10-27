/**
 * Game control (such as setting balls, streak, gamemode etc)
 */

#include <dlfcn.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"
#include "smashhit.h"

static inline Game *gamectl_get_game(void) {
	Game **ppGame = dlsym(gLibsmashhitHandle, "gGame");
	return *ppGame;
}

int knSetBalls(lua_State *script) {
	Game *game = gamectl_get_game();
	game->player->balls = lua_tointeger(script, 1);
	return 0;
}

int knSetStreak(lua_State *script) {
	Game *game = gamectl_get_game();
	game->player->streak = lua_tointeger(script, 1);
	return 0;
}

int knEnableGamectl(lua_State *script) {
	lua_register(script, "knSetBalls", knSetBalls);
	lua_register(script, "knSetStreak", knSetStreak);
	
	return 0;
}
