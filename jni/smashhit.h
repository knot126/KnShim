#include <inttypes.h>

#if defined(__ARM_ARCH_7A__)

typedef struct Player {
	char _unknown0[0x7f4];
	int balls;
	int streak;
	char _unknown1[0xb0];
	int mode;
} Player;

typedef struct Game {
	char _unknown0[0x30];
	Player *player;
	// incomplete
} Game;

#elif defined(__aarch64__)

typedef struct Player {
	char _unknown0[0x8bc];
	int balls;
	int streak;
	char _unknown1[0xb8];
	int mode;
} Player;

typedef struct Game {
	char _unknown0[0x60];
	Player *player;
	// incomplete
} Game;

#else
#warning smashhit.h not defined for this platform
#endif
