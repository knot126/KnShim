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
	Game **ppGame = KNGetSymbolAddr("gGame");
	return *ppGame;
}

static inline Level *gamectl_get_level(void) {
	return gamectl_get_game()->level;
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

int knGetBalls(lua_State *script) {
	Game *game = gamectl_get_game();
	lua_pushinteger(script, game->player->balls);
	return 1;
}

int knGetStreak(lua_State *script) {
	Game *game = gamectl_get_game();
	lua_pushinteger(script, game->player->streak);
	return 1;
}

int knLevelHitSomething(lua_State *script) {
	void (*hitSomething)(Level*, int) = KNGetSymbolAddr("_ZN5Level12hitSomethingEi");
	hitSomething(gamectl_get_level(), lua_tointeger(script, 1));
	return 0;
}

int knLevelStreakAbort(lua_State *script) {
	void (*streakAbort)(Level*, int) = KNGetSymbolAddr("_ZN5Level11streakAbortEi");
	streakAbort(gamectl_get_level(), lua_tointeger(script, 1));
	return 0;
}

int knLevelStreakInc(lua_State *script) {
	void (*streakInc)(Level*, int) = KNGetSymbolAddr("_ZN5Level9streakIncEi");
	streakInc(gamectl_get_level(), lua_tointeger(script, 1));
	return 0;
}

int knLevelAddScore(lua_State *script) {
	void (*addScore)(Level*, int, int) = KNGetSymbolAddr("_ZN5Level8addScoreEii");
	addScore(gamectl_get_level(), lua_tointeger(script, 1), lua_tointeger(script, 2));
	return 0;
}

int knLevelExplosion(lua_State *script) {
	Level *level = gamectl_get_level();
	
	QiVec3 pos = {
		.x = lua_tonumber(script, 1),
		.y = lua_tonumber(script, 2),
		.z = lua_tonumber(script, 3) - level->offsetZ,
	};
	
	float power = lua_tonumber(script, 4);
	
	void (*explosion)(Level*, QiVec3*, float) = KNGetSymbolAddr("_ZN5Level9explosionERK6QiVec3f");
	explosion(level, &pos, power);
	
	return 0;
}

int knEnableGamectl(lua_State *script) {
	lua_register(script, "knSetBalls", knSetBalls);
	lua_register(script, "knSetStreak", knSetStreak);
	lua_register(script, "knGetBalls", knSetBalls);
	lua_register(script, "knGetStreak", knSetStreak);
	
	// Level methods
	lua_register(script, "knLevelHitSomething", knLevelHitSomething);
	lua_register(script, "knLevelStreakAbort", knLevelStreakAbort);
	lua_register(script, "knLevelStreakInc", knLevelStreakInc);
	lua_register(script, "knLevelAddScore", knLevelAddScore);
	lua_register(script, "knLevelExplosion", knLevelExplosion);
	
	// Game methods
	
	return 0;
}
