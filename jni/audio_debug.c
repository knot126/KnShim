/**
 * This is not a real shim module; instead it's related to investigating why
 * QiAudioChannel::fillBuffer() segfaults after a while and contains some
 * special debug logging.
 */

#if 0
#include <android/log.h>
#include "util.h"
#include "smashhit.h"

typedef void (*QiAudioChannelFillBufferMethod)(QiAudioChannel *this, float *p1, float *p2, int p3);

QiAudioChannelFillBufferMethod QiAudioChannel_fillBuffer;

#define ACCESS(THIS, OFFSET, TYPE) (*(TYPE *)(((char *) THIS) + OFFSET))

void QiAudioChannel_fillBuffer_hook(QiAudioChannel *this, float *left, float *right, int samples) {
	float position = ACCESS(this, 0x5c, float);
	void *audioBuffer = ACCESS(this, 0x50, void *);
	int frequnecy = ACCESS(audioBuffer, 0xc, int);
	char stereo = ACCESS(audioBuffer, 0x10, char);
	
	short *audioData = ACCESS(audioBuffer, 0x20, short *);
	int audioDataSize = ACCESS(audioBuffer, 0x20, int);
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "QiAudioChannel::fillBuffer(%p, %p, %p, %d) position=%f freq=%d isStereo=%d audioData=%p audioDataSize=%d", this, left, right, samples, position, frequnecy, stereo, audioData, audioDataSize);
	
	QiAudioChannel_fillBuffer(this, left, right, samples);
}

void KNAudioDebugInit(struct android_app *app, Leaf *leaf) {
	QiAudioChannel_fillBuffer = KNHookFunctionByName("_ZN14QiAudioChannel10fillBufferEPfS0_i", QiAudioChannel_fillBuffer_hook, false);
	
	// Attempt to patch out faulty loop
#ifdef __aarch64__
	uint32_t *ins = KNGetSymbolAddr("_ZN14QiAudioChannel10fillBufferEPfS0_i") + 0x3d0;
	*ins = 0x14000021;
#endif
}
#endif
