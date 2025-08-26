#include <inttypes.h>
#include <stdio.h>

struct lua_State;
typedef struct lua_State lua_State;

typedef struct QiVec3 {
	float x, y, z;
} QiVec3;

typedef struct QiString {
	char *data;
	int allocated_size;
	int length;
	char cached[32];
} QiString;

typedef struct QiScript {
	void *_unknown0;
	void *fixed_chunk_allocator;
	lua_State* *state; // there is more this points to but idrc atm
} QiScript;

typedef struct Script {
	QiScript *script;
	// incomplete
} Script;

typedef struct ResMan {
	// unknown contents
} ResMan;

typedef struct Resource {
	ResMan *resMan;
	QiString path;
	void *resource;
	int type;
	/* 64-bit: 4 bytes padding */
} Resource;

typedef struct Gfx {
	// unknown
} Gfx;

typedef uint32_t QiByteOrder;

typedef struct QiOutputStreamVtable {
	void *destruct;
	void *destructWithFree;
	void *flush;
	void *writeInternal;
} QiOutputStreamVtable;

typedef struct QiOutputStream {
	QiOutputStreamVtable *vtable;
	QiByteOrder byteOrder;
	int position;
} QiOutputStream;

typedef struct QiInput {
	// unknown contents
} QiInput;

typedef struct QiFileInputStream {
	void *vtable;
	QiByteOrder byteOrder;
	int _unk;
	FILE *file;
	QiString path;
	int size;
	int position;
	void *androidAsset;
} QiFileInputStream;

typedef struct QiInput_Event {
	int type;
	int data;
	int x;
	int y;
} QiInput_Event;

typedef struct QiAudioChannel {
	/* Contents */
} QiAudioChannel;

// From Aladdin Enterprise's MD5 implemenation which Dennis uses.
typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t; /* 32-bit word */

typedef struct md5_state_s {
    md5_word_t count[2];	/* message length in bits, lsw first */
    md5_word_t abcd[4];		/* digest buffer */
    md5_byte_t buf[64];		/* accumulate block */
} md5_state_t;

typedef struct QiMd5 {
	md5_state_t md5_state;
	md5_byte_t final_hash[16];
} QiMd5;

/// !!! PLATFORM SPECIFIC (mostly unfinished structs) ///

#if defined(__arm__) || defined(__i386__)

typedef struct Player {
	char _unknown0[0x7f4];
	int balls;
	int streak;
	char _unknown1[0xb0];
	int mode;
} Player;

typedef struct Level {
	char _unknown0[0xf4];
	float offsetZ;
} Level;

typedef struct QiFileOutputStream {
	char _unknown0[0xc];
	FILE *file;
	QiString path;
} QiFileOutputStream;

#elif defined(__aarch64__)

typedef struct Player {
	char _unknown0[0x8bc];
	int balls;
	int streak;
	char _unknown1[0xb8];
	int mode;
} Player;

typedef struct Level {
	char _unknown0[0x124];
	float offsetZ;
} Level;

typedef struct QiFileOutputStream {
	char _unknown0[0x10];
	FILE *file;
	QiString path;
} QiFileOutputStream;

#else
#warning smashhit.h not defined for this platform
#endif

typedef struct Game {
	void *device;
	void *input;
	void *display;
	void *renderer;
	ResMan *resman;
	void *audio;
	void *debug;
	Gfx *gfx;
	void *scene1;
	void *scene2;
	void *scene3;
	Level *level;
	Player *player;
	void *http_thread;
	// incomplete
} Game;
