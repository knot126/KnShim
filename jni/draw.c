/**
 * Draw primitives not normally exposed to scripts by Smash Hit. Will probbably
 * expand to something much more in the future.
 */

#include <math.h>
#include <GLES2/gl2.h>
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

#define NOCAP ((size_t) -1)

typedef struct UIVertex {
	float x, y, z;
	float u, v;
	uint8_t r, g, b, a;
} UIVertex;

const uint32_t UINineSliceIndexes[54] = {
	// 1
	0, 1, 4,
	4, 5, 1,
	// 2
	2, 1, 5,
	5, 6, 2,
	// 3
	2, 3, 6,
	6, 7, 3,
	// 4
	4, 5, 8,
	9, 8, 5,
	// 5
	5, 6, 9,
	9, 10, 6,
	// 6
	7, 6, 10,
	10, 11, 7,
	// 7
	8, 9, 12,
	12, 13, 9,
	// 8
	9, 10, 13,
	13, 14, 10,
	// 9
	10, 14, 11,
	14, 15, 11,
};

static void UIGenerateNineSliceVerts(UIVertex verts[16], float sx, float sy, float top, float bottom, float left, float right, float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	/**
	 * Generate verticies for a nine slice scaled image with its bottom left corner
	 * at (0, 0).
	 */
	
	// Row 1
	verts[0] = (UIVertex) {0.0, sy, z, 0.0, 1.0, r, g, b, a};
	verts[1] = (UIVertex) {left, sy, z, left / sx, 1.0, r, g, b, a};
	verts[2] = (UIVertex) {sx - right, sy, z, (sx - right) / sx, 1.0, r, g, b, a};
	verts[3] = (UIVertex) {sx, sy, z, 1.0, 1.0, r, g, b, a};
	
	// Row 2
	verts[4] = (UIVertex) {0.0, (sy - top), z, 0.0, (sy - top) / sy, r, g, b, a};
	verts[5] = (UIVertex) {left, (sy - top), z, left / sx, (sy - top) / sy, r, g, b, a};
	verts[6] = (UIVertex) {sx - right, (sy - top), z, (sx - right) / sx, (sy - top) / sy, r, g, b, a};
	verts[7] = (UIVertex) {sx, (sy - top), z, 1.0, (sy - top) / sy, r, g, b, a};
	
	// Row 3
	verts[8] = (UIVertex) {0.0, bottom, z, 0.0, bottom / sy, r, g, b, a};
	verts[9] = (UIVertex) {left, bottom, z, left / sx, bottom / sy, r, g, b, a};
	verts[10] = (UIVertex) {sx - right, bottom, z, (sx - right) / sx, bottom / sy, r, g, b, a};
	verts[11] = (UIVertex) {sx, bottom, z, 1.0, bottom / sy, r, g, b, a};
	
	// Row 4
	verts[12] = (UIVertex) {0.0, 0.0, z, 0.0, 0.0, r, g, b, a};
	verts[13] = (UIVertex) {left, 0.0, z, left / sx, 0.0, r, g, b, a};
	verts[14] = (UIVertex) {sx - right, 0.0, z, (sx - right) / sx, 0.0, r, g, b, a};
	verts[15] = (UIVertex) {sx, 0.0, z, 1.0, 0.0, r, g, b, a};
}

int knGetViewport(lua_State *L) {
	GLint result[4];
	glGetIntegerv(GL_VIEWPORT, result);
	lua_pushinteger(L, result[0]);
	lua_pushinteger(L, result[1]);
	lua_pushinteger(L, result[2]);
	lua_pushinteger(L, result[3]);
	return 4;
}

int knEnableDraw(lua_State *L) {
	knRegisterFunc(L, knDrawLine);
	knRegisterFunc(L, knDrawRectangle);
	
	Gfx_drawLine = KNGetSymbolAddr("_ZN3Gfx8drawLineERK6QiVec3S2_RK7QiColorf");
	Gfx_drawRectangle = KNGetSymbolAddr("_ZN3Gfx13drawRectangleERK6QiVec2S2_RK7QiColor");
	
	return 0;
}
