/**
 * Definitely FUNction related things
 */

#include "util.h"

void KNFunInit(struct android_app *app, Leaf *leaf) {
	gIsOwnLibraryAvailable = KNIsOwnLibraryAvailable();
}
