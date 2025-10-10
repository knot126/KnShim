/**
 * Get touchscreen input
 */

#include <math.h>
#include "util.h"
#include "lua_utils.h"

#define gInput (gGame->input)

int (*QiInput_getTouchCount)(QiInput *this);
bool (*QiInput_hasTouch)(QiInput *this, int index);
int (*QiInput_getTouchPosX)(QiInput *this, int index);
int (*QiInput_getTouchPosY)(QiInput *this, int index);

int knGetTouchCount(lua_State *L) {
	lua_pushinteger(L, QiInput_getTouchCount(gInput));
	return 1;
}

int knHasTouch(lua_State *L) {
	lua_pushboolean(L, QiInput_hasTouch(gInput, lua_tointeger(L, 1)));
	return 1;
}

int knGetTouchPos(lua_State *L) {
	int i = lua_tointeger(L, 1);
	int x = QiInput_getTouchPosX(gInput, i);
	int y = QiInput_getTouchPosY(gInput, i);
	knLuaPushVec2(L, (QiVec2) {x, y});
	return 1;
}

int knEnableInput(lua_State *L) {
	knRegisterFunc(L, knGetTouchCount);
	knRegisterFunc(L, knHasTouch);
	knRegisterFunc(L, knGetTouchPos);
	
	QiInput_getTouchCount = KNGetSymbolAddr("_ZNK7QiInput13getTouchCountEv");
	QiInput_hasTouch = KNGetSymbolAddr("_ZNK7QiInput8hasTouchEi");
	QiInput_getTouchPosX = KNGetSymbolAddr("_ZNK7QiInput12getTouchPosXEi");
	QiInput_getTouchPosY = KNGetSymbolAddr("_ZNK7QiInput12getTouchPosYEi");
	
	return 0;
}
