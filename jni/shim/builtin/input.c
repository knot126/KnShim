/**
 * Get touchscreen input
 */

#include <math.h>
#include <android/native_activity.h>
#include "../util.h"
#include "lua_utils.h"

#define gInput (gGame->input)

int (*QiInput_getTouchCount)(QiInput *this);
bool (*QiInput_hasTouch)(QiInput *this, int index);
int (*QiInput_getTouchPosX)(QiInput *this, int index);
int (*QiInput_getTouchPosY)(QiInput *this, int index);
bool (*QiInput_wasTouchPressed)(QiInput *this, int index);
bool (*QiInput_wasTouchReleased)(QiInput *this, int index);

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
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 2;
}

int knWasTouchPressed(lua_State *L) {
	lua_pushboolean(L, QiInput_wasTouchPressed(gInput, lua_tointeger(L, 1)));
	return 1;
}

int knWasTouchReleased(lua_State *L) {
	lua_pushboolean(L, QiInput_wasTouchReleased(gInput, lua_tointeger(L, 1)));
	return 1;
}

/*
TODO: Doesn't work! ANativeActivity_showSoftInput is broken.
int knShowKeyboard(lua_State *L) {
	bool forced = lua_toboolean(L, 1);
	ANativeActivity_showSoftInput(gApp->activity, forced ? ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED : ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT);
	return 0;
}
*/

int knEnableInput(lua_State *L) {
	knRegisterFunc(L, knGetTouchCount);
	knRegisterFunc(L, knHasTouch);
	knRegisterFunc(L, knGetTouchPos);
	knRegisterFunc(L, knWasTouchPressed);
	knRegisterFunc(L, knWasTouchReleased);
	
	QiInput_getTouchCount = KNGetSymbolAddr("_ZNK7QiInput13getTouchCountEv");
	QiInput_hasTouch = KNGetSymbolAddr("_ZNK7QiInput8hasTouchEi");
	QiInput_getTouchPosX = KNGetSymbolAddr("_ZNK7QiInput12getTouchPosXEi");
	QiInput_getTouchPosY = KNGetSymbolAddr("_ZNK7QiInput12getTouchPosYEi");
	QiInput_wasTouchPressed = KNGetSymbolAddr("_ZNK7QiInput15wasTouchPressedEi");
	QiInput_wasTouchReleased = KNGetSymbolAddr("_ZNK7QiInput16wasTouchReleasedEi");
	
	return 0;
}
