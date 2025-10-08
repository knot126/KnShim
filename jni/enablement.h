enum {
	KN_LOG_BIT = (1 << 0),
	KN_PATCHING_BIT = (1 << 1),
	KN_HTTP_BIT = (1 << 2),
	KN_SYSTEM_BIT = (1 << 3),
	KN_REGISTRY_BIT = (1 << 4),
	KN_DATABASE_BIT = (1 << 5),
	KN_FILE_BIT = (1 << 6),
	KN_GAMECTL_BIT = (1 << 7),
	KN_OVERLAY_BIT = (1 << 8),
	KN_SHADERS_BIT = (1 << 9),
};

#define KNSHIM_ENABLE() \
	int knEnableLog(lua_State *script);\
	if ((gDisabledModules & KN_LOG_BIT) == 0) { knEnableLog(script); }\
	int knEnablePatching(lua_State *script);\
	if ((gDisabledModules & KN_PATCHING_BIT) == 0) { knEnablePatching(script); }\
	int knEnableHttp(lua_State *script);\
	if ((gDisabledModules & KN_HTTP_BIT) == 0) { knEnableHttp(script); }\
	int knEnableSystem(lua_State *script);\
	if ((gDisabledModules & KN_SYSTEM_BIT) == 0) { knEnableSystem(script); }\
	int knEnableRegistry(lua_State *script);\
	if ((gDisabledModules & KN_REGISTRY_BIT) == 0) { knEnableRegistry(script); }\
	int knEnableDatabase(lua_State *script);\
	if ((gDisabledModules & KN_DATABASE_BIT) == 0) { knEnableDatabase(script); }\
	int knEnableFile(lua_State *script);\
	if ((gDisabledModules & KN_FILE_BIT) == 0) { knEnableFile(script); }\
	int knEnableGamectl(lua_State *script);\
	if ((gDisabledModules & KN_GAMECTL_BIT) == 0) { knEnableGamectl(script); }\
	int knEnableOverlay(lua_State *script);\
	if ((gDisabledModules & KN_OVERLAY_BIT) == 0) { knEnableOverlay(script); }\
	int knEnableShaders(lua_State *script);\
	if ((gDisabledModules & KN_SHADERS_BIT) == 0) { knEnableShaders(script); }\


#define KNSHIM_PUSH_ENABLE_ENUM() \
	knLuaPushEnum(script, KN_LOG_BIT);\
	knLuaPushEnum(script, KN_PATCHING_BIT);\
	knLuaPushEnum(script, KN_HTTP_BIT);\
	knLuaPushEnum(script, KN_SYSTEM_BIT);\
	knLuaPushEnum(script, KN_REGISTRY_BIT);\
	knLuaPushEnum(script, KN_DATABASE_BIT);\
	knLuaPushEnum(script, KN_FILE_BIT);\
	knLuaPushEnum(script, KN_GAMECTL_BIT);\
	knLuaPushEnum(script, KN_OVERLAY_BIT);\
	knLuaPushEnum(script, KN_SHADERS_BIT);\
