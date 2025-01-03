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

typedef struct QiOutputStream {
	// unknown contents
} QiOutputStream;

typedef struct QiInput {
	// unknown contents
} QiInput;

typedef struct QiInput_Event {
	int type;
	int data;
	int x;
	int y;
} QiInput_Event;

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

typedef struct Game {
	void *device;
	void *input;
	void *display;
	void *renderer;
	void *resman;
	void *audio;
	void *debug;
	void *gfx;
	void *scene1;
	void *scene2;
	void *scene3;
	Level *level;
	Player *player;
	void *http_thread;
	// incomplete
} Game;

typedef struct QiFileInputStream {
	char _unknown0[0xc];
	FILE *file;
	QiString path;
	int length;
	int headpos;
	size_t _unknown1;
} QiFileInputStream;

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

typedef struct Game {
	void *device;
	void *input;
	void *display;
	void *renderer;
	void *resman;
	void *audio;
	void *debug;
	void *gfx;
	void *scene1;
	void *scene2;
	void *scene3;
	Level *level;
	Player *player;
	void *http_thread;
	// incomplete
} Game;

typedef struct QiFileInputStream {
	char _unknown0[0x10];
	FILE *file;
	QiString path;
	int length;
	int headpos;
	size_t _unknown1;
} QiFileInputStream;

typedef struct QiFileOutputStream {
	char _unknown0[0x10];
	FILE *file;
	QiString path;
} QiFileOutputStream;

#else
#warning smashhit.h not defined for this platform
#endif
