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

#define MakeQiString(CSTR) { \
	.data = (char *) CSTR, \
	.allocated_size = strlen(CSTR), \
	.length = strlen(CSTR), \
}

int knSetBalls(lua_State *script) {
	/**
	 * Set the player's ball count
	 */
	
	Game *game = gamectl_get_game();
	game->player->balls = lua_tointeger(script, 1);
	return 0;
}

int knSetStreak(lua_State *script) {
	/**
	 * Set the player's streak
	 */
	
	Game *game = gamectl_get_game();
	game->player->streak = lua_tointeger(script, 1);
	return 0;
}

int knGetBalls(lua_State *script) {
	/**
	 * Get the player's ballcount. This is accurate even if knSetBalls was used.
	 */
	
	Game *game = gamectl_get_game();
	lua_pushinteger(script, game->player->balls);
	return 1;
}

int knGetStreak(lua_State *script) {
	/**
	 * Get the player's streak. This is accurate even if knSetStreak was used.
	 */
	
	Game *game = gamectl_get_game();
	lua_pushinteger(script, game->player->streak);
	return 1;
}

uint32_t gNoclipBufferedInstruction = KN_RET;

#define NOCLIP_IS_ON (gNoclipBufferedInstruction != KN_RET)

void swap_noclip_state(void) {
	uint32_t *hitSomething = KNGetSymbolAddr("_ZN5Level12hitSomethingEi");
	uint32_t currentInstr = hitSomething[0];
	hitSomething[0] = gNoclipBufferedInstruction;
	gNoclipBufferedInstruction = currentInstr;
}

int knSetNoclip(lua_State *script) {
	/**
	 * Set noclip as enabled or disabled.
	 */
	
	// Swap if states don't match
	if ((NOCLIP_IS_ON) != lua_toboolean(script, 1)) {
		swap_noclip_state();
	}
	
	return 0;
}

int knGetNoclip(lua_State *script) {
	/**
	 * Get the current noclip state.
	 */
	
	lua_pushboolean(script, NOCLIP_IS_ON);
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
		.z = -lua_tonumber(script, 3) - level->offsetZ,
	};
	
	float power = lua_tonumber(script, 4);
	
	void (*explosion)(Level*, QiVec3*, float) = KNGetSymbolAddr("_ZN5Level9explosionERK6QiVec3f");
	explosion(level, &pos, power);
	
	return 0;
}

int knDownloadFile(lua_State *script) {
	/**
	 * (bool) success = knDownloadFile((string) url, (string) path)
	 * 
	 * Download a file over HTTP to the given resource manager path. For
	 * example:
	 * 
	 * if knDownloadFile("http://myserver.com/tomount.zip", "user://tomount.zip") then
	 *   -- success
	 * else
	 *   -- failure
	 * end
	 * 
	 * NOTE: This is a blocking operation, so the game will freeze until it is
	 * complete. If you want to download a file without blocking, use the HTTP
	 * module provided by KnShim.
	 */
	
	bool (*downloadFile)(void *_httpThread, QiString *url, QiString *path) = KNGetSymbolAddr("_ZN10HttpThread12downloadFileE8QiStringRKS0_");
	
	QiString qUrl = MakeQiString(lua_tostring(script, 1));
	QiString qPath = MakeQiString(lua_tostring(script, 2));
	
	if (!qUrl.data || !qPath.data) {
		lua_toboolean(script, false);
		return 1;
	}
	
	// The `this` pointer is never actually used, so it's okay to pass NULL here.
	bool status = downloadFile(NULL, &qUrl, &qPath);
	
	lua_toboolean(script, status);
	return 1;
}

int knHttpPost(lua_State *script) {
	/**
	 * (bool) success = knHttpPost((string) url, (string) data)
	 * 
	 * POST the data to the URL. For example:
	 * 
	 * if knHttpPost("http://myserver.com/leaderboard/", "distance=12345") then
	 *   -- success
	 * else
	 *   -- failure
	 * end
	 */
	
	bool (*httpPost)(ResMan *this, QiString *url, const void *buffer, int size) = KNGetSymbolAddr("_ZN6ResMan8httpPostERK8QiStringPKvi");
	
	QiString qUrl = MakeQiString(lua_tostring(script, 1));
	
	size_t size;
	const char *buffer = lua_tolstring(script, 2, &size);
	
	// Again, it appears `this` is never used so this is probably fine.
	bool success = httpPost(NULL, &qUrl, buffer, size);
	
	lua_pushboolean(script, success);
	return 1;
}

int knConnectAssetServer(lua_State *script) {
	/**
	 * (bool) success = knConnectAssetServer((string) host, (float) timeout)
	 * 
	 * Connect to an asset server, waiting up to `timeout` seconds for a
	 * connection to be formed.
	 */
	
	bool (*connectAssetServer)(QiString *host, float timeout) = KNGetSymbolAddr("_ZN6ResMan18connectAssetServerERK8QiStringf");
	
	const char *host_cstr = lua_tostring(script, 1);
	float timeout = lua_tonumber(script, 2);
	
	QiString host_qstr = MakeQiString(host_cstr);
	
	lua_pushboolean(script, connectAssetServer(&host_qstr, timeout));
	
	return 1;
}

int knDisconnectAssetServer(lua_State *script) {
	/**
	 * knDisconnectAssetServer()
	 * 
	 * Disconnect from an asset server, if currently connected.
	 */
	
	void (*disconnectAssetServer)(void) = KNGetSymbolAddr("_ZN6ResMan21disconnectAssetServerEv");
	disconnectAssetServer();
	return 0;
}

int knIsConnectedToAssetServer(lua_State *script) {
	/**
	 * (bool) isConnected = knIsConnectedToAssetServer()
	 * 
	 * Check if the game is currently connected to an asset server.
	 */
	
	void *sAssetSocket = *(void **)KNGetSymbolAddr("_ZN6ResMan12sAssetSocketE");
	lua_pushboolean(script, sAssetSocket != NULL);
	return 1;
}

/**
 * Support reloading the main menu by simulating a press of the R debug key when
 * reloading is wanted.
 */
bool gWantsReload = false;
bool (*gWasKeyPressedFunc)(QiInput *this, int ch);

bool KNReload_WasKeyPressedHook(QiInput *this, int ch) {
	/**
	 * Simulate pressing the 'R' key if a reload is wanted.
	 */
	
	if (gWantsReload && ch == 'r') {
		gWantsReload = false;
		return true;
	}
	
	return gWasKeyPressedFunc(this, ch);
}

int knEnableReloading(lua_State *script) {
	/**
	 * knEnableReloading()
	 * 
	 * Do the initial setup required for reloading. Only needs to be called once
	 * when the game is started.
	 */
	
	if (!gWasKeyPressedFunc) {
		KNHookFunction(KNGetSymbolAddr("_ZNK7QiInput13wasKeyPressedEi"), KNReload_WasKeyPressedHook, (void **) &gWasKeyPressedFunc);
	}
	
	return 0;
}

int knReload(lua_State *script) {
	/**
	 * knReload()
	 * 
	 * Reload the main menu or level on the next frame.
	 */
	
	gWantsReload = true;
	return 0;
}

int knShitpost(lua_State *script) {
	/**
	 * result = knShitpost()
	 * 
	 * Return a funny string.
	 */
	
	static const char *sShitposts[] = {
		"Smash Hit Cdde Mode",
		"Speedrun this Game!",
		"<aitiktokvoice>99% of people cannot beat the first level in this ultra-challenging game. It's called Smash Hit and is free on Google PlayStore. Download now!</aitiktokvoice>",
		"coffee stain more like shit stain",
		"Miles 'Tails' Prower is god",
		"SHN sucks, and that's a fact",
		"KnShim (C) 2024 - 2025 Knot126",
		"I really like the idea of the string 'Fur Affinity' being in KnShim, so here it is",
	};
	
	lua_pushstring(script, sShitposts[rand() % (sizeof(sShitposts) / sizeof(sShitposts[0]))]);
	return 1;
}

int knEnableGamectl(lua_State *script) {
	// Cheats
	lua_register(script, "knSetBalls", knSetBalls);
	lua_register(script, "knGetBalls", knGetBalls);
	lua_register(script, "knSetStreak", knSetStreak);
	lua_register(script, "knGetStreak", knGetStreak);
	lua_register(script, "knSetNoclip", knSetNoclip);
	lua_register(script, "knGetNoclip", knGetNoclip);
	
	// Wrappers of built-in HTTP functions
	knRegisterFunc(script, knDownloadFile);
	knRegisterFunc(script, knHttpPost);
	
	// Asset server
	knRegisterFunc(script, knConnectAssetServer);
	knRegisterFunc(script, knDisconnectAssetServer);
	knRegisterFunc(script, knIsConnectedToAssetServer);
	
	// Menu reloading
	knRegisterFunc(script, knEnableReloading);
	knRegisterFunc(script, knReload);
	
	// Mule made me do this
	knRegisterFunc(script, knShitpost);
	
	// Level methods
	lua_register(script, "knLevelHitSomething", knLevelHitSomething);
	lua_register(script, "knLevelStreakAbort", knLevelStreakAbort);
	lua_register(script, "knLevelStreakInc", knLevelStreakInc);
	lua_register(script, "knLevelAddScore", knLevelAddScore);
	lua_register(script, "knLevelExplosion", knLevelExplosion);
	
	// Game methods
	
	return 0;
}
