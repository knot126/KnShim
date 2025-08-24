#define GL_GLES_PROTOTYPES 1
#include <GLES2/gl2.h>

#include "lua/lua.h"
#include "util.h"

enum {
	GAME_SHADER_2D = 0,
	GAME_SHADER_2DTEX,
	GAME_SHADER_BALLREFLECTION,
	GAME_SHADER_BASICGLASS,
	GAME_SHADER_BASIC,
	GAME_SHADER_BLITFBOBLUR,
	GAME_SHADER_BLITFBO,
	GAME_SHADER_BLURH,
	GAME_SHADER_BLURV,
	GAME_SHADER_BODYDEPTH,
	GAME_SHADER_BODYREFLECTION,
	GAME_SHADER_BODYTEX,
	GAME_SHADER_CLEAR,
	GAME_SHADER_DOFALPHA,
	GAME_SHADER_DOF,
	GAME_SHADER_FONT,
	GAME_SHADER_GLASS,
	GAME_SHADER_GLASSLOW,
	GAME_SHADER_GLASSTEX,
	GAME_SHADER_GLASSTEXLOW,
	GAME_SHADER_HIGHLIGHT,
	GAME_SHADER_MENUSMOKE,
	GAME_SHADER_METAL,
	GAME_SHADER_METALLOW,
	GAME_SHADER_PARTICLES,
	GAME_SHADER_ROOM,
	GAME_SHADER_ROOMLOW,
	GAME_SHADER_ROOMMENU,
	GAME_SHADER_ROOMMENULOW,
	GAME_SHADER_SOFTSHADOW,
	GAME_SHADER_SPRITE,
	GAME_SHADER_SPRITELOW,
	GAME_SHADER_SPRITEREFLECTION,
	GAME_SHADER_WATER,
	GAME_SHADER_WIRE,
};

#if defined(__aarch64__)

#define GAME_TO_GFX_OFFSET 0x38
#define RES_TO_TYPE_OFFSET 0x40
#define RES_TO_PTR_OFFSET 0x38
#define QISHADER_TO_PROGRAM_OFFSET 0x48
static const size_t gShaderResOffsets[] = {
	[GAME_SHADER_2D]               = 0x710,
	[GAME_SHADER_2DTEX]            = 0x7A0,
	[GAME_SHADER_BALLREFLECTION]   = 0xE60,
	[GAME_SHADER_BASICGLASS]       = 0xD88,
	[GAME_SHADER_BASIC]            = 0xD40,
	[GAME_SHADER_BLITFBOBLUR]      = 0x878,
	[GAME_SHADER_BLITFBO]          = 0x830,
	[GAME_SHADER_BLURH]            = 0x8C0,
	[GAME_SHADER_BLURV]            = 0x908,
	[GAME_SHADER_BODYDEPTH]        = 0xB90,
	[GAME_SHADER_BODYREFLECTION]   = 0x9E0,
	[GAME_SHADER_BODYTEX]          = 0xA28,
	[GAME_SHADER_CLEAR]            = 0x758,
	[GAME_SHADER_DOFALPHA]         = 0x998,
	[GAME_SHADER_DOF]              = 0x950,
	[GAME_SHADER_FONT]             = 0x7E8,
	[GAME_SHADER_GLASS]            = 0xB48,
	[GAME_SHADER_GLASSLOW]         = 0xEA8,
	[GAME_SHADER_GLASSTEX]         = 0x1010,
	[GAME_SHADER_GLASSTEXLOW]      = 0x1058,
	[GAME_SHADER_HIGHLIGHT]        = 0xBD8,
	[GAME_SHADER_MENUSMOKE]        = 0x10A0,
	[GAME_SHADER_METAL]            = 0xDD0,
	[GAME_SHADER_METALLOW]         = 0xFC8,
	[GAME_SHADER_PARTICLES]        = 0xE18,
	[GAME_SHADER_ROOM]             = 0xCB0,
	[GAME_SHADER_ROOMLOW]          = 0xF38,
	[GAME_SHADER_ROOMMENU]         = 0xEF0,
	[GAME_SHADER_ROOMMENULOW]      = 0xCF8,
	[GAME_SHADER_SOFTSHADOW]       = 0xA70,
	[GAME_SHADER_SPRITE]           = 0xC20,
	[GAME_SHADER_SPRITELOW]        = 0xF80,
	[GAME_SHADER_SPRITEREFLECTION] = 0xC68,
	[GAME_SHADER_WATER]            = 0xAB8,
	[GAME_SHADER_WIRE]             = 0xB00,
};

#elif defined(__arm__) || defined(__i386__)

#define GAME_TO_GFX_OFFSET 0x1C
#define RES_TO_TYPE_OFFSET 0x34
#define RES_TO_PTR_OFFSET 0x30
#define QISHADER_TO_PROGRAM_OFFSET 0x48
static const size_t gShaderResOffsets[] = {
	[GAME_SHADER_2D]               = 0x5E8,
	[GAME_SHADER_2DTEX]            = 0x658,
	[GAME_SHADER_BALLREFLECTION]   = 0xB98,
	[GAME_SHADER_BASICGLASS]       = 0xAF0,
	[GAME_SHADER_BASIC]            = 0xAB8,
	[GAME_SHADER_BLITFBOBLUR]      = 0x700,
	[GAME_SHADER_BLITFBO]          = 0x6C8,
	[GAME_SHADER_BLURH]            = 0x738,
	[GAME_SHADER_BLURV]            = 0x770,
	[GAME_SHADER_BODYDEPTH]        = 0x968,
	[GAME_SHADER_BODYREFLECTION]   = 0x818,
	[GAME_SHADER_BODYTEX]          = 0x850,
	[GAME_SHADER_CLEAR]            = 0x620,
	[GAME_SHADER_DOFALPHA]         = 0x7E0,
	[GAME_SHADER_DOF]              = 0x7A8,
	[GAME_SHADER_FONT]             = 0x690,
	[GAME_SHADER_GLASS]            = 0x930,
	[GAME_SHADER_GLASSLOW]         = 0xBD0,
	[GAME_SHADER_GLASSTEX]         = 0xCE8,
	[GAME_SHADER_GLASSTEXLOW]      = 0xD20,
	[GAME_SHADER_HIGHLIGHT]        = 0x9A0,
	[GAME_SHADER_MENUSMOKE]        = 0xD58,
	[GAME_SHADER_METAL]            = 0xB28,
	[GAME_SHADER_METALLOW]         = 0xCB0,
	[GAME_SHADER_PARTICLES]        = 0xB60,
	[GAME_SHADER_ROOM]             = 0xA48,
	[GAME_SHADER_ROOMLOW]          = 0xC40,
	[GAME_SHADER_ROOMMENU]         = 0xC08,
	[GAME_SHADER_ROOMMENULOW]      = 0xA80,
	[GAME_SHADER_SOFTSHADOW]       = 0x888,
	[GAME_SHADER_SPRITE]           = 0x9D8,
	[GAME_SHADER_SPRITELOW]        = 0xC78,
	[GAME_SHADER_SPRITEREFLECTION] = 0xA10,
	[GAME_SHADER_WATER]            = 0x8C0,
	[GAME_SHADER_WIRE]             = 0x8F8,
};

#else
#  warning "no offset for architecture"
#endif

static GLint gOldProgram;
static void *gVBuffer;
static size_t gVBufferCapacity;

static int getShaderEnumByName(const char *name) {
	if (strcmp(name, "2d") == 0)               return GAME_SHADER_2D;
	if (strcmp(name, "2dtex") == 0)            return GAME_SHADER_2DTEX;
	if (strcmp(name, "ballreflection") == 0)   return GAME_SHADER_BALLREFLECTION;
	if (strcmp(name, "basicglass") == 0)       return GAME_SHADER_BASICGLASS;
	if (strcmp(name, "basic") == 0)            return GAME_SHADER_BASIC;
	if (strcmp(name, "blitfboblur") == 0)      return GAME_SHADER_BLITFBOBLUR;
	if (strcmp(name, "blitfbo") == 0)          return GAME_SHADER_BLITFBO;
	if (strcmp(name, "blurh") == 0)            return GAME_SHADER_BLURH;
	if (strcmp(name, "blurv") == 0)            return GAME_SHADER_BLURV;
	if (strcmp(name, "bodydepth") == 0)        return GAME_SHADER_BODYDEPTH;
	if (strcmp(name, "bodyreflection") == 0)   return GAME_SHADER_BODYREFLECTION;
	if (strcmp(name, "bodytex") == 0)          return GAME_SHADER_BODYTEX;
	if (strcmp(name, "clear") == 0)            return GAME_SHADER_CLEAR;
	if (strcmp(name, "dofalpha") == 0)         return GAME_SHADER_DOFALPHA;
	if (strcmp(name, "dof") == 0)              return GAME_SHADER_DOF;
	if (strcmp(name, "font") == 0)             return GAME_SHADER_FONT;
	if (strcmp(name, "glass") == 0)            return GAME_SHADER_GLASS;
	if (strcmp(name, "glasslow") == 0)         return GAME_SHADER_GLASSLOW;
	if (strcmp(name, "glasstex") == 0)         return GAME_SHADER_GLASSTEX;
	if (strcmp(name, "glasstexlow") == 0)      return GAME_SHADER_GLASSTEXLOW;
	if (strcmp(name, "highlight") == 0)        return GAME_SHADER_HIGHLIGHT;
	if (strcmp(name, "menusmoke") == 0)        return GAME_SHADER_MENUSMOKE;
	if (strcmp(name, "metal") == 0)            return GAME_SHADER_METAL;
	if (strcmp(name, "metallow") == 0)         return GAME_SHADER_METALLOW;
	if (strcmp(name, "particles") == 0)        return GAME_SHADER_PARTICLES;
	if (strcmp(name, "room") == 0)             return GAME_SHADER_ROOM;
	if (strcmp(name, "roomlow") == 0)          return GAME_SHADER_ROOMLOW;
	if (strcmp(name, "roommenu") == 0)         return GAME_SHADER_ROOMMENU;
	if (strcmp(name, "roommenulow") == 0)      return GAME_SHADER_ROOMMENULOW;
	if (strcmp(name, "softshadow") == 0)       return GAME_SHADER_SOFTSHADOW;
	if (strcmp(name, "sprite") == 0)           return GAME_SHADER_SPRITE;
	if (strcmp(name, "spritelow") == 0)        return GAME_SHADER_SPRITELOW;
	if (strcmp(name, "spritereflection") == 0) return GAME_SHADER_SPRITEREFLECTION;
	if (strcmp(name, "water") == 0)            return GAME_SHADER_WATER;
	if (strcmp(name, "wire") == 0)             return GAME_SHADER_WIRE;
	return -1;
}

static void reserveVBufferSpace(size_t req_size) {
	if (gVBufferCapacity < req_size) {
		while (gVBufferCapacity < req_size) {
			gVBufferCapacity = gVBufferCapacity == 0 ? 256 : gVBufferCapacity * 2;
		}
		gVBuffer = realloc(gVBuffer, gVBufferCapacity);
	}
}

static size_t readFloatsToVBuffer(lua_State *script, int index, int stride) {
	size_t table_len = lua_objlen(script, index);
	size_t el_count = table_len / stride;
	size_t val_count = el_count * stride;
	size_t req_size = val_count * sizeof(GLfloat);

	reserveVBufferSpace(req_size);

	GLfloat *values = gVBuffer;

	for (size_t i = 0; i < val_count; i++) {
		lua_pushinteger(script, i);
		lua_gettable(script, index);
		values[i] = lua_tonumber(script, -1);
		lua_pop(script, 1);
	}

	return el_count;
}

static size_t readIntsToVBuffer(lua_State *script, int index, int stride) {
	size_t table_len = lua_objlen(script, index);
	size_t el_count = table_len / stride;
	size_t val_count = el_count * stride;
	size_t req_size = val_count * sizeof(GLint);

	reserveVBufferSpace(req_size);

	GLint *values = gVBuffer;

	for (size_t i = 0; i < val_count; i++) {
		lua_pushinteger(script, i);
		lua_gettable(script, index);
		values[i] = lua_tointeger(script, -1);
		lua_pop(script, 1);
	}

	return el_count;
}

int knBeginShaders(lua_State *script) {
	/**
	 * knBeginShaders()
	 *
	 * Stores Smash Hit's current shader in internal state
	 */

	glGetIntegerv(GL_CURRENT_PROGRAM, &gOldProgram);

	return 0;
}

int knEndShaders(lua_State *script) {
	/**
	 * knEndShaders()
	 *
	 * Restores Smash Hit's current shader previously saved by knBeginShaders()
	 */

	glUseProgram(gOldProgram);

	return 0;
}

int knGetGameShader(lua_State *script) {
	/**
	 * (nil|number) shader = knGetGameShader((string) name)
	 *
	 * Returns Smash Hit's shader program id by name
	 */

	if (lua_gettop(script) < 1) {
		goto returnnil;
	}

	const char *shader_name = lua_tostring(script, 1);

	int shader_enum = getShaderEnumByName(shader_name);

	if (shader_enum < 0) {
		goto returnnil;
	}

	size_t res_offset = gShaderResOffsets[shader_enum];

	char *gGame_ptr = KNGetSymbolAddr("gGame");
	if (gGame_ptr == NULL) {
		goto returnnil;
	}

	char *game_ptr = *(void **)gGame_ptr;
	if (game_ptr == NULL) {
		goto returnnil;
	}

	char *gfx_ptr = *(void **)(game_ptr + GAME_TO_GFX_OFFSET);
	char *res_ptr = gfx_ptr + res_offset;

	char *res_type_ptr = res_ptr + RES_TO_TYPE_OFFSET;
	int res_type = *(int *)res_type_ptr;
	if (res_type != 3) {
		goto returnnil;
	}

	char *qishader_ptr = *(void **)(res_ptr + RES_TO_PTR_OFFSET);
	if (qishader_ptr == NULL) {
		goto returnnil;
	}

	char *program_ptr = qishader_ptr + QISHADER_TO_PROGRAM_OFFSET;

	GLuint program = *(GLuint *)program_ptr;

	if (program == 0) {
		goto returnnil;
	}

	lua_pushinteger(script, program);
	return 1;

returnnil:
	lua_pushnil(script);
	return 1;
}

int knGetShaderUniformLocation(lua_State *script) {
	/**
	 * (nil|number) location = knGetShaderUniformLocation(shader, name)
	 *
	 * Returns a shader program's uniform location by the uniform's name
	 */

	if (!lua_isnumber(script, 1) || !lua_isstring(script, 2)) {
		goto returnnil;
	}

	GLuint program = lua_tointeger(script, 1);
	const char *uniform = lua_tostring(script, 2);

	GLint loc = glGetUniformLocation(program, uniform);

	if (loc < 0) {
		goto returnnil;
	}

	lua_pushinteger(script, loc);
	return 1;

returnnil:
	lua_pushnil(script);
	return 1;
}

int knUseShader(lua_State *script) {
	/**
	 * knUseShader(shader)
	 *
	 * Sets the shader program as current
	 */

	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLuint program = lua_tointeger(script, 1);

	if (program == 0) {
		return 0;
	}

	glUseProgram(program);

	return 0;
}

/**
 * knShaderUniform1f((number) location, (number) v0)
 * knShaderUniform2f((number) location, (number) v0, (number) v1)
 * knShaderUniform3f((number) location, (number) v0, (number) v1, (number) v2)
 * knShaderUniform4f((number) location, (number) v0, (number) v1, (number) v2, (number) v3)
 * knShaderUniform1i((number) location, (number) v0)
 * knShaderUniform2i((number) location, (number) v0, (number) v1)
 * knShaderUniform3i((number) location, (number) v0, (number) v1, (number) v2)
 * knShaderUniform4i((number) location, (number) v0, (number) v1, (number) v2, (number) v3)
 *
 * Sets a float, vec2, vec3, vec4, int, vec2i, vec3i, or vec4i uniform in the current shader program
 */

int knShaderUniform1f(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLfloat v0 = lua_tonumber(script, 2);

	if (loc < 0) {
		return 0;
	}

	glUniform1f(loc, v0);

	return 0;
}

int knShaderUniform2f(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLfloat v0 = lua_tonumber(script, 2);
	GLfloat v1 = lua_tonumber(script, 3);

	if (loc < 0) {
		return 0;
	}

	glUniform2f(loc, v0, v1);

	return 0;
}

int knShaderUniform3f(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLfloat v0 = lua_tonumber(script, 2);
	GLfloat v1 = lua_tonumber(script, 3);
	GLfloat v2 = lua_tonumber(script, 4);

	if (loc < 0) {
		return 0;
	}

	glUniform3f(loc, v0, v1, v2);

	return 0;
}

int knShaderUniform4f(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLfloat v0 = lua_tonumber(script, 2);
	GLfloat v1 = lua_tonumber(script, 3);
	GLfloat v2 = lua_tonumber(script, 4);
	GLfloat v3 = lua_tonumber(script, 5);

	if (loc < 0) {
		return 0;
	}

	glUniform4f(loc, v0, v1, v2, v3);

	return 0;
}

int knShaderUniform1i(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLint v0 = lua_tointeger(script, 2);

	if (loc < 0) {
		return 0;
	}

	glUniform1i(loc, v0);

	return 0;
}

int knShaderUniform2i(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLint v0 = lua_tointeger(script, 2);
	GLint v1 = lua_tointeger(script, 3);

	if (loc < 0) {
		return 0;
	}

	glUniform2i(loc, v0, v1);

	return 0;
}

int knShaderUniform3i(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLint v0 = lua_tointeger(script, 2);
	GLint v1 = lua_tointeger(script, 3);
	GLint v2 = lua_tointeger(script, 4);

	if (loc < 0) {
		return 0;
	}

	glUniform3i(loc, v0, v1, v2);

	return 0;
}

int knShaderUniform4i(lua_State *script) {
	if (!lua_isnumber(script, 1)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	GLint v0 = lua_tointeger(script, 2);
	GLint v1 = lua_tointeger(script, 3);
	GLint v2 = lua_tointeger(script, 4);
	GLint v3 = lua_tointeger(script, 5);

	if (loc < 0) {
		return 0;
	}

	glUniform4i(loc, v0, v1, v2, v3);

	return 0;
}

/**
 * knShaderUniform1fv((number) location, (table) t)
 * knShaderUniform2fv((number) location, (table) t)
 * knShaderUniform3fv((number) location, (table) t)
 * knShaderUniform4fv((number) location, (table) t)
 * knShaderUniform1iv((number) location, (table) t)
 * knShaderUniform2iv((number) location, (table) t)
 * knShaderUniform3iv((number) location, (table) t)
 * knShaderUniform4iv((number) location, (table) t)
 *
 * Sets a float, vec2, vec3, vec4, int, vec2i, vec3i, or vec4i uniform array in the current shader program.
 *
 * The elements are placed linearly inside t, like this:
 *     { x0, y0, z0, x1, y1, z1, x2, y2, z2, ..., xn, yn, zn }
 *
 * If the last element is unfinished, it is ignored.
 */

int knShaderUniform1fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 2, 1);

	glUniform1fv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform2fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 2, 2);

	glUniform2fv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform3fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 2, 3);

	glUniform3fv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform4fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 2, 4);

	glUniform4fv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform1iv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readIntsToVBuffer(script, 2, 1);

	glUniform1iv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform2iv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readIntsToVBuffer(script, 2, 2);

	glUniform2iv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform3iv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readIntsToVBuffer(script, 2, 3);

	glUniform3iv(loc, el_count, gVBuffer);

	return 0;
}

int knShaderUniform4iv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 2)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readIntsToVBuffer(script, 2, 4);

	glUniform4iv(loc, el_count, gVBuffer);

	return 0;
}

/**
 * knShaderUniformMatrix2fv((number) location, (boolean) transpose, (table) t)
 * knShaderUniformMatrix3fv((number) location, (boolean) transpose, (table) t)
 * knShaderUniformMatrix4fv((number) location, (boolean) transpose, (table) t)
 *
 * Sets a mat2, mat3, or mat4 uniform array in the current shader program.
 *
 * The elements are placed linearly inside t, like this:
 *     { x0, y0, z0, x1, y1, z1, x2, y2, z2, ..., xn, yn, zn }
 *
 * If the last element is unfinished, it is ignored.
 *
 * If transpose is true, the matrices will be transposed before being assigned to uniforms.
 */

int knShaderUniformMatrix2fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 3)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	int transpose = lua_toboolean(script, 2);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 3, 4);

	glUniformMatrix2fv(loc, el_count, transpose, gVBuffer);

	return 0;
}

int knShaderUniformMatrix3fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 3)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	int transpose = lua_toboolean(script, 2);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 3, 9);

	glUniformMatrix3fv(loc, el_count, transpose, gVBuffer);

	return 0;
}

int knShaderUniformMatrix4fv(lua_State *script) {
	if (!lua_isnumber(script, 1) || !lua_istable(script, 3)) {
		return 0;
	}

	GLint loc = lua_tointeger(script, 1);
	int transpose = lua_toboolean(script, 2);

	if (loc < 0) {
		return 0;
	}

	size_t el_count = readFloatsToVBuffer(script, 3, 16);

	glUniformMatrix4fv(loc, el_count, transpose, gVBuffer);

	return 0;
}

int knEnableShaders(lua_State *script) {
	knRegisterFunc(script, knBeginShaders);
	knRegisterFunc(script, knEndShaders);

	knRegisterFunc(script, knGetGameShader);

	// shader usage and state
	knRegisterFunc(script, knUseShader);
	knRegisterFunc(script, knShaderUniform1f);
	knRegisterFunc(script, knShaderUniform2f);
	knRegisterFunc(script, knShaderUniform3f);
	knRegisterFunc(script, knShaderUniform4f);
	knRegisterFunc(script, knShaderUniform1i);
	knRegisterFunc(script, knShaderUniform2i);
	knRegisterFunc(script, knShaderUniform3i);
	knRegisterFunc(script, knShaderUniform4i);
	knRegisterFunc(script, knShaderUniform1fv);
	knRegisterFunc(script, knShaderUniform2fv);
	knRegisterFunc(script, knShaderUniform3fv);
	knRegisterFunc(script, knShaderUniform4fv);
	knRegisterFunc(script, knShaderUniform1iv);
	knRegisterFunc(script, knShaderUniform2iv);
	knRegisterFunc(script, knShaderUniform3iv);
	knRegisterFunc(script, knShaderUniform4iv);
	knRegisterFunc(script, knShaderUniformMatrix2fv);
	knRegisterFunc(script, knShaderUniformMatrix3fv);
	knRegisterFunc(script, knShaderUniformMatrix4fv);

	// shader query
	knRegisterFunc(script, knGetShaderUniformLocation);

	return 0;
}
