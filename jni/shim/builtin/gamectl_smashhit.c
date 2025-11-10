/**
 * Game control (such as setting balls, streak, gamemode etc)
 */

#ifndef GRANNY

#include <dlfcn.h>
#include <math.h>

#include "lua_utils.h"

#include "../util.h"
#include "smashhit.h"

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
	
	gGame->player->balls = lua_tointeger(script, 1);
	return 0;
}

int knGetBalls(lua_State *script) {
	/**
	 * Get the player's ballcount. This is accurate even if knSetBalls was used.
	 */
	
	lua_pushinteger(script, gGame->player->balls);
	return 1;
}

int knSetStreak(lua_State *script) {
	/**
	 * Set the player's streak
	 */
	
	gGame->player->streak = lua_tointeger(script, 1);
	return 0;
}

int knGetStreak(lua_State *script) {
	/**
	 * Get the player's streak. This is accurate even if knSetStreak was used.
	 */
	
	lua_pushinteger(script, gGame->player->streak);
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
	hitSomething(gGame->level, lua_tointeger(script, 1));
	return 0;
}

int knLevelStreakAbort(lua_State *script) {
	void (*streakAbort)(Level*, int) = KNGetSymbolAddr("_ZN5Level11streakAbortEi");
	streakAbort(gGame->level, lua_tointeger(script, 1));
	return 0;
}

int knLevelStreakInc(lua_State *script) {
	void (*streakInc)(Level*, int) = KNGetSymbolAddr("_ZN5Level9streakIncEi");
	streakInc(gGame->level, lua_tointeger(script, 1));
	return 0;
}

int knLevelAddScore(lua_State *script) {
	void (*addScore)(Level*, int, int) = KNGetSymbolAddr("_ZN5Level8addScoreEii");
	addScore(gGame->level, lua_tointeger(script, 1), lua_tointeger(script, 2));
	return 0;
}

int knLevelExplosion(lua_State *script) {
	Level *level = gGame->level;
	
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

void (*Game_loadTemplates)(Game *this);

int knLoadTemplates(lua_State *script) {
	/**
	 * (bool) success = knLoadTemplates()
	 * 
	 * Reload templates.
	 */
	
	if (!Game_loadTemplates) {
		Game_loadTemplates = KNGetSymbolAddr("_ZN4Game13loadTemplatesEv");
	}
	
	if (gGame) {
		Game_loadTemplates(gGame);
		lua_pushboolean(script, 1);
	}
	else {
		lua_pushboolean(script, 0);
	}
	
	return 1;
}

void (*Gfx_construct)(Gfx *this, ResMan *resMan);
void (*Gfx_destruct)(Gfx *this);
void (*Gfx_load1)(Gfx *this, ResMan *resMan);
void (*Gfx_load2)(Gfx *this, ResMan *resMan);

int knLoadGfx(lua_State *script) {
	/**
	 * knLoadGfx()
	 * 
	 * Reload all of the game's hardcoded shaders and textures.
	 */
	
	if (!Gfx_load1 || !Gfx_load2) {
		Gfx_construct = KNGetSymbolAddr("_ZN3GfxC2EP6ResMan");
		Gfx_destruct = KNGetSymbolAddr("_ZN3GfxD2Ev");
		Gfx_load1 = KNGetSymbolAddr("_ZN3Gfx5load1EP6ResMan");
		Gfx_load2 = KNGetSymbolAddr("_ZN3Gfx5load2EP6ResMan");
	}
	
	if (!gGame) {
		return 0;
	}
	
	Gfx_destruct(gGame->gfx);
	Gfx_construct(gGame->gfx, gGame->resman);
	Gfx_load1(gGame->gfx, gGame->resman);
	Gfx_load2(gGame->gfx, gGame->resman);
	
	return 0;
}

/**
 * Refresh rate changing
 */
int knGetDeviceHz(lua_State *script) {
	/**
	 * (float) hz = knGetDeviceHz()
	 * 
	 * Get the native refresh rate for the default display.
	 */
	
	lua_pushnumber(script, KNGetRefreshRate());
	return 1;
}

#define ROUND(x) (floor(x * 10000.0f) / 10000.0f)

int knSetFrameRate(lua_State *script) {
	/**
	 * (bool) success = knSetFrameRate([(float) frameRate, [(int) sleepTime]])
	 * 
	 * Set the game framerate and adjust the physics time step to match. Smash
	 * Hit normally runs at 60hz, even if the device has a higher refresh rate
	 * display. This will make adjustments so that the game runs at a higher
	 * refresh rate, if available.
	 * 
	 * Calling knSetFrameRate() with no arguments will set the game to match the
	 * refresh rate of the device. This is recommended if you're going to change
	 * the refresh rate at all.
	 * 
	 * Only available on 64-bit ARM devices.
	 */
	
#if defined(__aarch64__)
	int argc = lua_gettop(script);
	
	float targetFPS;
	
	if (argc > 0) {
		targetFPS = lua_tonumber(script, 1);
	}
	else {
		targetFPS = KNGetRefreshRate();
	}
	
	float timeStep = 1.0 / targetFPS;
	float sleepTime = ROUND(timeStep);
	
	if (argc > 1) {
		sleepTime = lua_tonumber(script, 2);
	}
	
	float *v1 = (KNGetSymbolAddr("_ZN4GameC2EP6Deviceii") + 0xd68);
	float *v2 = (KNGetSymbolAddr("_ZN4Game6updateEv") + 0x300);
	float *v3 = (KNGetSymbolAddr("android_main") + 0xd9c);
	
	*v1 = timeStep;
	*v2 = timeStep;
	*v3 = sleepTime;
	
	lua_pushboolean(script, true);
#else
	lua_pushboolean(script, false);
#endif
	return 1;
}

/**
 * Misc utilities
 */
int knJavaCommand(lua_State *script) {
	KNLoadFunc(QiString, _Z11javaCommandRK8QiString, (QiString *command));
	
	const char *cmd = lua_tostring(script, 1);
	cmd = cmd ? cmd : "";
	QiString qCmd = MakeQiString(cmd);
	QiString result = _Z11javaCommandRK8QiString(&qCmd);
	lua_pushstring(script, result.data ? result.data : result.cached);
	return 1;
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
	
	// Reloading
	knRegisterFunc(script, knReload);
	knRegisterFunc(script, knLoadTemplates);
	knRegisterFunc(script, knLoadGfx);
	
	// Level methods
	knRegisterFunc(script, knLevelHitSomething);
	knRegisterFunc(script, knLevelStreakAbort);
	knRegisterFunc(script, knLevelStreakInc);
	knRegisterFunc(script, knLevelAddScore);
	knRegisterFunc(script, knLevelExplosion);
	
	// Refresh rate
	knRegisterFunc(script, knGetDeviceHz);
	knRegisterFunc(script, knSetFrameRate);
	
	// Misc utilities
	knRegisterFunc(script, knJavaCommand);
	
	return 0;
}

#endif // GRANNY
