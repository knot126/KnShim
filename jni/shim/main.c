/**
 * Shit file
 * 
 * -----------------------------------------------------------------------------
 * 
 * This file is part of KnShim. Copyright (c) 2024 - 2025 Knot126.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <android_native_app_glue.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>

#ifdef GRANNY
#warning "Granny smith builds are not reliable yet!"
#endif

#include "util.h"
#include "loader.h"

typedef void (*AndroidMainFunc)(struct android_app *app);

#include "version.h"

void android_main(struct android_app *app) {
	const char *status;
	
	// Set gApp to android app structure
	gApp = app;
	
	// Early shim init
	status = KnShim_EarlyInit();
	
	if (status) {
		LogF("Early shim init failed: %s", status);
		abort();
	}
	
	LogI("KnShim r%s (%s for %s); App SDK %d, Device SDK %d", SHIM_VERSION, KN_GAME_STRING, KN_ARCH_STRING, KNGetAppSDK(), KNGetDeviceSDK());
	
	// Load game
	status = KnShim_LoadGame();
	
	if (status) {
		LogF("Loading game failed: %s", status);
		abort();
	}
	
	// Load mods + later shim init
	status = KnShim_LoadMods();
	
	if (status) {
		LogF("Failed to load mods: %s", status);
		abort();
	}
	
	// Get main func and call
	AndroidMainFunc func = LeafSymbolAddr(gLeaf, "android_main");
	
	if (!func) {
		LogF("Could not find android_main()!!!");
		abort();
	}
	
	func(app);
}
