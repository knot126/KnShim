#include <inttypes.h>
#include <stdio.h>

typedef struct QiVec3 {
	float x, y, z;
} QiVec3;

typedef struct QiString {
	char *data;
	int allocated_size;
	int length;
	char cached[32];
} QiString;

#if defined(__ARM_ARCH_7A__)

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
