/**
 * Game control (such as setting balls, streak, gamemode etc)
 */

#include <dlfcn.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "util.h"
#include "smashhit.h"

static inline Game *getGame(void) {
	Game **ppGame = KNGetSymbolAddr("gGame");
	return *ppGame;
}

static inline Level *getLevel(void) {
	return getGame()->level;
}

#define MakeQiString(CSTR) (QiString) { \
	.data = (char *) CSTR, \
	.allocated_size = strlen(CSTR), \
	.length = strlen(CSTR), \
}

/**
 * RAW GET/SET BALLS
 */
int knSetBalls(lua_State *script) {
	/**
	 * Set the player's ball count
	 */
	
	Game *game = getGame();
	game->player->balls = lua_tointeger(script, 1);
	return 0;
}

int knGetBalls(lua_State *script) {
	/**
	 * Get the player's ballcount. This is accurate even if knSetBalls was used.
	 */
	
	Game *game = getGame();
	lua_pushinteger(script, game->player->balls);
	return 1;
}

int knSetStreak(lua_State *script) {
	/**
	 * Set the player's streak
	 */
	
	Game *game = getGame();
	game->player->streak = lua_tointeger(script, 1);
	return 0;
}

int knGetStreak(lua_State *script) {
	/**
	 * Get the player's streak. This is accurate even if knSetStreak was used.
	 */
	
	Game *game = getGame();
	lua_pushinteger(script, game->player->streak);
	return 1;
}

/**
 * NO CLIP
 */
shortop_t gNoclipBufferedInstruction = KN_RET;

#define NOCLIP_IS_ON (gNoclipBufferedInstruction != KN_RET)

void swap_noclip_state(void) {
	shortop_t *hitSomething = KNGetSymbolAddr("_ZN5Level12hitSomethingEi");
	shortop_t currentInstr = hitSomething[0];
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

/**
 * LEVEL FUNCTION HELPERS
 */
int knLevelHitSomething(lua_State *script) {
	void (*hitSomething)(Level*, int) = KNGetSymbolAddr("_ZN5Level12hitSomethingEi");
	hitSomething(getLevel(), lua_tointeger(script, 1));
	return 0;
}

int knLevelStreakAbort(lua_State *script) {
	void (*streakAbort)(Level*, int) = KNGetSymbolAddr("_ZN5Level11streakAbortEi");
	streakAbort(getLevel(), lua_tointeger(script, 1));
	return 0;
}

int knLevelStreakInc(lua_State *script) {
	void (*streakInc)(Level*, int) = KNGetSymbolAddr("_ZN5Level9streakIncEi");
	streakInc(getLevel(), lua_tointeger(script, 1));
	return 0;
}

int knLevelAddScore(lua_State *script) {
	void (*addScore)(Level*, int, int) = KNGetSymbolAddr("_ZN5Level8addScoreEii");
	addScore(getLevel(), lua_tointeger(script, 1), lua_tointeger(script, 2));
	return 0;
}

int knLevelExplosion(lua_State *script) {
	Level *level = getLevel();
	
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

/**
 * NETWORKING WRAPPERS
 */
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

void *DupBuf(const void *buf, size_t size) {
	void *nbuf = malloc(size);
	
	if (!nbuf) return NULL;
	
	return memcpy(nbuf, buf, size);
}

struct HttpPostAsyncInfo {
	bool (*httpPost)(ResMan *this, QiString *url, const void *buffer, int size);
	QiString qUrl;
	char *buffer;
	int size;
};

void *knHttpPostAsync_thread(struct HttpPostAsyncInfo *info) {
	info->httpPost(NULL, &info->qUrl, info->buffer, info->size);
	free(info->qUrl.data);
	free(info->buffer);
	free(info);
	return NULL;
}

int knHttpPostAsync(lua_State *script) {
	/**
	 * (none) knHttpPostAsync((string) url, (string) data)
	 * 
	 * This is similar to knHttpPost, but does it in a new thread discarding the
	 * return result. This means it might fail without any indication, but will
	 * not block the main thread.
	 */
	
	struct HttpPostAsyncInfo *info = malloc(sizeof *info);
	
	if (!info) {
		return 0;
	}
	
	const char *lua_url = lua_tostring(script, 1);
	
	if (!lua_url) {
		free(info);
		return 0;
	}
	
	char *urlbuf = DupBuf(lua_url, strlen(lua_url) + 1);
	
	if (!urlbuf) {
		free(info);
		return 0;
	}
	
	info->httpPost = KNGetSymbolAddr("_ZN6ResMan8httpPostERK8QiStringPKvi");
	info->qUrl = MakeQiString(urlbuf);
	
	size_t size;
	const char *buffer = lua_tolstring(script, 2, &size);
	
	if (!buffer) {
		free(urlbuf);
		free(info);
		return 0;
	}
	
	info->buffer = DupBuf(buffer, size);
	
	if (!info->buffer) {
		free(urlbuf);
		free(info);
		return 0;
	}
	
	info->size = size;
	
	KNPreformInBackground((PthreadCallbackFunc) knHttpPostAsync_thread, info);
	
	return 0;
}

/**
 * ASSET SERVER CONTROL
 */
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
 * MAIN MENU RELOADING
 * 
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

int knReload(lua_State *script) {
	/**
	 * knReload()
	 * 
	 * Reload the main menu or level on the next frame.
	 */
	
	if (!gWasKeyPressedFunc) {
		KNHookFunction(KNGetSymbolAddr("_ZNK7QiInput13wasKeyPressedEi"), KNReload_WasKeyPressedHook, (void **) &gWasKeyPressedFunc);
	}
	
	gWantsReload = true;
	return 0;
}

int knEnableGamectl(lua_State *script) {
	// Cheats
	knRegisterFunc(script, knSetBalls);
	knRegisterFunc(script, knGetBalls);
	knRegisterFunc(script, knSetStreak);
	knRegisterFunc(script, knGetStreak);
	knRegisterFunc(script, knSetNoclip);
	knRegisterFunc(script, knGetNoclip);
	
	// Wrappers of built-in HTTP functions
	knRegisterFunc(script, knDownloadFile);
	knRegisterFunc(script, knHttpPost);
	knRegisterFunc(script, knHttpPostAsync);
	
	// Asset server
	knRegisterFunc(script, knConnectAssetServer);
	knRegisterFunc(script, knDisconnectAssetServer);
	knRegisterFunc(script, knIsConnectedToAssetServer);
	
	// Menu reloading
	knRegisterFunc(script, knReload);
	
	// Level methods
	knRegisterFunc(script, knLevelHitSomething);
	knRegisterFunc(script, knLevelStreakAbort);
	knRegisterFunc(script, knLevelStreakInc);
	knRegisterFunc(script, knLevelAddScore);
	knRegisterFunc(script, knLevelExplosion);
	
	// Game methods
	
	return 0;
}
