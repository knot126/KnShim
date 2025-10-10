/**
 * Draw primitives not normally exposed to scripts by Smash Hit. Will probbably
 * expand to something much more in the future.
 */

#include <math.h>
#include "util.h"
#include "lua_utils.h"

void (*Gfx_drawLine)(Gfx *this,QiVec3 *pointA,QiVec3 *pointB,QiColor *colour,float param_5);
void (*Gfx_drawRectangle)(Gfx *this,QiVec2 *param_1,QiVec2 *param_2,QiColor *param_3);

int knDrawLine(lua_State *L) {
	QiVec3 a = knLuaToVec3(L, 1);
	QiVec3 b = knLuaToVec3(L, 2);
	QiColor color = knLuaToColor(L, 3);
	Gfx_drawLine(gGame->gfx, &a, &b, &color, 1.0);
	return 0;
}

int knDrawRectangle(lua_State *L) {
	QiVec2 a = knLuaToVec2(L, 1);
	QiVec2 b = knLuaToVec2(L, 1);
	QiColor color = knLuaToColor(L, 3);
	Gfx_drawRectangle(gGame->gfx, &a, &b, &color);
	return 0;
}

int knEnableDraw(lua_State *L) {
	knRegisterFunc(L, knDrawLine);
	knRegisterFunc(L, knDrawRectangle);
	
	Gfx_drawLine = KNGetSymbolAddr("_ZN3Gfx8drawLineERK6QiVec3S2_RK7QiColorf");
	Gfx_drawRectangle = KNGetSymbolAddr("_ZN3Gfx13drawRectangleERK6QiVec2S2_RK7QiColor");
	
	return 0;
}
