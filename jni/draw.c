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

typedef struct UIBuffer {
	size_t size;
	size_t capacity;
	UIVertex *data;
	
	size_t ind_size;
	size_t ind_capacity;
	uint32_t *ind_data;
} UIBuffer;

static bool UIBufferInit(UIBuffer *self, UIVertex *vert_buf, uint32_t *ind_buf) {
	memset(self, 0, sizeof *self);
	
	if (vert_buf) {
		self->data = vert_buf;
		self->capacity = NOCAP;
	}
	
	if (ind_buf) {
		self->ind_data = ind_buf;
		self->ind_capacity = NOCAP;
	}
	
	return true;
}

static bool UIVertexAppend(UIBuffer *self, UIVertex *vert) {
	if (self->size + 1 > self->capacity) {
		size_t new_cap = 2 * self->capacity + 1;
		UIVertex *new_data = malloc(new_cap * sizeof *self->data);
		
		if (!new_data) {
			return false;
		}
		
		self->data = new_data;
		self->capacity = new_cap;
	}
	
	self->data[self->size++] = *vert;
	
	return true;
}

static bool UIIndexAppend(UIBuffer *self, uint32_t ind) {
	if (self->ind_size + 1 > self->ind_capacity) {
		size_t new_cap = 2 * self->ind_capacity + 1;
		uint32_t *new_data = malloc(new_cap * sizeof *self->ind_data);
		
		if (!new_data) {
			return false;
		}
		
		self->ind_data = new_data;
		self->ind_capacity = new_cap;
	}
	
	self->ind_data[self->ind_size++] = ind;
	
	return true;
}

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
