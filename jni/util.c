#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "util.h"

int unprotect_memory(void *addr, size_t length) {
	size_t page_size = getpagesize();
	size_t align_diff = (size_t)addr % page_size; 
	addr -= align_diff;
	length += align_diff;
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "unprotecting 0x%zx bytes at <%p> (aligned to %zu)", length, addr, page_size);
	
	int result = mprotect(addr, length, PROT_READ | PROT_WRITE | PROT_EXEC);
	
	if (result) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "mprotect() failed: status %d: %d %s", result, errno, strerror(errno));
	}
	
	return result;
}

int set_memory_protection(void *addr, size_t length, int protection) {
	/**
	 * Similar to unprotect_memory, but allows any protection status to be set.
	 * This is mainly so we can respect the W^X limits that Android 11+ seem to
	 * have.
	 */
	
	size_t page_size = getpagesize();
	size_t align_diff = (size_t)addr % page_size; 
	addr -= align_diff;
	length += align_diff;
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "unprotecting 0x%zx bytes at <%p> (aligned to %zu)", length, addr, page_size);
	
	int result = mprotect(addr, length, protection);
	
	if (result) {
		__android_log_print(ANDROID_LOG_FATAL, TAG, "mprotect() failed: status %d: %d %s", result, errno, strerror(errno));
	}
	
	return result;
}

int invert_branch(void *addr) {
	/**
	 * Invert the branch at the given address.
	 * 
	 * On armv7 this also allows to invert any conditional isntruction but is
	 * not supported when cond=AL (hex E, bin 0b1110) since that would require
	 * NOP'ing out the instruction, making it impossible to invert again.
	 * 
	 * On armv8 this may only work for instructions of the form b.COND and
	 * bc.COND with immidate values.
	 * 
	 * Return 0 on success, nonzero on error.
	 */
	
	#if defined(__ARM_ARCH_7A__)
	uint32_t instr = *(uint32_t *)addr;
	
	// Nicely, we're allowed to just flip bit 28 and get the inverse
	// branch for pretty much any type of condition :D
	// See ARMv7 manual A5.1 and A8.3
	if ((instr >> 29) != 0b111) {
		instr ^= 0x10000000;
		*(uint32_t *)addr = instr;
		return 0;
	}
	// The exception is for cond=AL or cond=1111, for which the first
	// is unconditional (and we'd have to nop out which would technically
	// work but means destroying what was there) and the second is also
	// unconditional but reserved for another set of unconditional
	// instructions.
	else {
		return 1;
	}
	#elif defined(__aarch64__)
	uint32_t instr = *(uint32_t *)addr;
	
	// Check that this is either B.cond or BC.cond - that's all we support here!
	// See C6.2.27 and C6.2.28 of ARMv8 reference manual. Note that bit 4 doesnt
	// matter for our purposes! Also see C4.2.1 for what cond bits are, same as
	// ARMv7 basically, so we can again just flip the bits though we don't need
	// to worry about if cond=1110 or 1111 since they're both the same anyways.
	if ((instr >> 24) == 0b01010100) {
		instr ^= 1;
		*(uint32_t *)addr = instr;
		return 0;
	}
	else {
		return 1;
	}
	#else
	return 1;
	#endif
}

// ARM Architecture Reference Manual for A-profile architecture C6.2.185
#define AARCH64_MAKE_LDRX_LITERAL(Rt, imm19) ( (0b01011000 << 24) | (((imm19 >> 2) & 0b1111111111111111111) << 5) | (Rt & 0b11111) )
// ARM Architecture Reference Manual for A-profile architecture C6.2.38
#define AARCH64_MAKE_BR(Rn) ( (0b1101011000011111000000 << 10) | ((Rn & 0b11111) << 5) )

int replace_function(void *from, void *to) {
	/**
	 * Make it so that calls to `from` will be redirected to `to` in the future.
	 * Returns 0 on success, or nonzero on failure.
	 */
	
	#if defined(__ARM_ARCH_7A__)
	return 1;
	#elif defined(__aarch64__)
	// Unfortunately, we can't just load our value directly into the PC since
	// the PC isn't a general purpose register in AArch64. Instead, we can load
	// the address into x16 or x17 (which according to the AArch64 Procedure
	// Call Standard can be corrupted between a branch to a function and its
	// first instruction for things like veneers which is basically what we're
	// creating) then branch to it.
	uint32_t *instructions = (uint32_t *) from;
	uint32_t *to_parts = (uint32_t *) &to;
	
	// On AArch64 the PC points to current instruction so we need to load from
	// 8 bytes after the current PC. I *think* immidate ldr should allow somewhat
	// unaligned access as long as they are a multipule of four so it shouldn't
	// matter if this is somehow placed at an unaligned address.
	instructions[0] = AARCH64_MAKE_LDRX_LITERAL(16, 8);
	instructions[1] = AARCH64_MAKE_BR(16);
	instructions[2] = to_parts[0];
	instructions[3] = to_parts[1];
	
	return 0;
	#else
	return 1;
	#endif
}
