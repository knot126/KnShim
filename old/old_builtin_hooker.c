
// ARM Architecture Reference Manual for A-profile architecture F5.1.74
// Using P==0 and W==0
#define AARCH32_MAKE_LDR_LITERAL(U, Rt, imm12) ( (0b11100100 << 24) | (((U) & 1) << 23) | (0b0011111 << 16) | (((Rt) & 0xf) << 12) | ((imm12) & 0xfff))
// ARM Architecture Reference Manual for A-profile architecture F5.1.27
#define AARCH32_MAKE_BX(Rm) ( (0b1110000100101111111111110001 << 4) | (Rm & 0xf) )

// ARM Architecture Reference Manual for A-profile architecture C6.2.185
// Uses 64-bit variant
#define AARCH64_MAKE_LDRX_LITERAL(Rt, imm19) ( (0b01011000 << 24) | ((((imm19) >> 2) & 0b1111111111111111111) << 5) | (Rt & 0b11111) )
// ARM Architecture Reference Manual for A-profile architecture C6.2.38
#define AARCH64_MAKE_BR(Rn) ( (0b1101011000011111000000 << 10) | ((Rn & 0b11111) << 5) )

int replace_function(void *from, void *to) {
	/**
	 * Make it so that calls to `from` will be redirected to `to` in the future.
	 * Returns number of bytes written on success, zero on failure. This is
	 * similar to a hook but doesn't allow you to call the original function.
	 * It can be used as a primitive for a hook.
	 */
	
	#if defined(__ARM_ARCH_7A__)
	uint32_t *instructions = (uint32_t *) from;
	
	// The situtation is pretty similar on AArch32 as it is to AArch64. The
	// biggest difference is that ip/r12 is the register that can be corrupted
	// according to the AArch32 PCS. The only thing to note is that the PC
	// points to the current instruction address plus 8.
	instructions[0] = AARCH32_MAKE_LDR_LITERAL(1, 12, 0);
	instructions[1] = AARCH32_MAKE_BX(12);
	instructions[2] = (uint32_t) to;
	
	return 12;
	#elif defined(__aarch64__)
	uint32_t *instructions = (uint32_t *) from;
	uint32_t *to_parts = (uint32_t *) &to;
	
	// Unfortunately, we can't just load our value directly into the PC since
	// the PC isn't a general purpose register in AArch64. Instead, we can load
	// the address into x16 or x17 (which according to the AArch64 Procedure
	// Call Standard can be corrupted between a branch to a function and its
	// first instruction for things like veneers which is basically what we're
	// creating) then branch to it.
	// 
	// On AArch64 the PC points to current instruction so we need to load from
	// 8 bytes after the current PC. I *think* immidate ldr should allow somewhat
	// unaligned access as long as they are a multipule of four so it shouldn't
	// matter if this is somehow placed at an unaligned address.
	instructions[0] = AARCH64_MAKE_LDRX_LITERAL(16, 8);
	instructions[1] = AARCH64_MAKE_BR(16);
	instructions[2] = to_parts[0];
	instructions[3] = to_parts[1];
	
	return 16;
	#else
	return 0;
	#endif
}

void KNCreateLongJump(void *block, void *addr) {
	/**
	 * Assemble a long jump at `block` that jumps to `addr`. This can also be
	 * used to replace the function starting at `block` with the one at `addr`.
	 */
	
	#if defined(__ARM_ARCH_7A__)
	uint32_t *instructions = (uint32_t *) block;
	
	// The situtation is pretty similar on AArch32 as it is to AArch64. The
	// biggest difference is that ip/r12 is the register that can be corrupted
	// according to the AArch32 PCS. The only thing to note is that the PC
	// points to the current instruction address plus 8.
	instructions[0] = AARCH32_MAKE_LDR_LITERAL(1, 12, 0);
	instructions[1] = AARCH32_MAKE_BX(12);
	instructions[2] = (uint32_t) addr;
	#elif defined(__aarch64__)
	uint32_t *instructions = (uint32_t *) block;
	uint32_t *to_parts = (uint32_t *) &addr;
	
	// Unfortunately, we can't just load our value directly into the PC since
	// the PC isn't a general purpose register in AArch64. Instead, we can load
	// the address into x16 or x17 (which according to the AArch64 Procedure
	// Call Standard can be corrupted between a branch to a function and its
	// first instruction for things like veneers which is basically what we're
	// creating) then branch to it.
	// 
	// On AArch64 the PC points to current instruction so we need to load from
	// 8 bytes after the current PC. I *think* immidate ldr should allow somewhat
	// unaligned access as long as they are a multipule of four so it shouldn't
	// matter if this is somehow placed at an unaligned address.
	instructions[0] = AARCH64_MAKE_LDRX_LITERAL(16, 8);
	instructions[1] = AARCH64_MAKE_BR(16);
	instructions[2] = to_parts[0];
	instructions[3] = to_parts[1];
	#else
	#error Arch not supported
	#endif
}

void KNRewriteBlock(uint32_t *code, size_t code_size, size_t *fixup_block, void *orig_code_addr) {
	/**
	 * Rewrite the block so that some types of PC-relative operations are
	 * accurate, presuming that the start of the code was loaded at the given
	 * original address.
	 * 
	 * @warning `addr_block` must be a location nearby `code` and must be at
	 * least `(code_size/4) * sizeof(size_t)` bytes long
	 * @warning Only supports:
	 *   ARM64: adr, adrp
	 *   ARM32: ldr (literal)
	 */
	
	// Do the rewriting
	for (size_t i = 0; i < (code_size >> 2); i++) {
		uint32_t ins = code[i];
		
		// The original PC value (or PC-8 for arm32 because Reasons)
		const size_t orig_ip = (size_t)orig_code_addr + ((size_t)&code[0] - (size_t)&code[i]);
		
		// adr and adrp, C6.2.11 and C6.2.12
		#if defined(__aarch64__)
		if (((ins >> 24) & 0b11111) == 0b10000) {
			bool adrp = (ins & 0x80000000) >> 31;
			
			// Read Rd
			uint32_t Rd = ins & 0b11111;
			
			// Read imm
			uint64_t imm21 = (((ins >> 5) & 0x7ffff) << 2) | ((ins >> 29) & 0b11);
			
			// sign extend
			if (imm21 & 0x100000) {
				imm21 ^= 0xffffffffffe00000;
			}
			
			// it should be fine to do this after sign extending even if you're
			// meant to do it before -- the results should be the same
			if (adrp) {
				imm21 <<= 12;
			}
			
			// finally we get a signed int - might not be needed because of how
			// twos compliment works?
			// int64_t imm = *((int64_t *) &imm21);
			
			// Calculate pc value
			size_t pc = (size_t) orig_code_addr + ((size_t)&code[0] - (size_t)&code[i]);
			pc = (adrp ? pc & 0xfffffffffffff000 : pc);
			
			// Find the value we need to actually load and save it in the fixup
			// block
			fixup_block[0] = pc + imm21;
			
			// Assemble the ldr instruction
			code[i] = AARCH64_MAKE_LDRX_LITERAL(Rd, (size_t)fixup_block - (size_t)&code[i]);
			
			// Inc fixups block, since we used it
			fixup_block++;
		}
		#elif defined(__ARM_ARCH_7A__)
		// ldr (literal)
		if (((ins >> 16) & 0b111001011111) == 0b010000011111 && (ins >> 28) != 0b1111) {
			uint32_t imm12 = ins & 0xfff;
			uint32_t Rt = (ins >> 12) & 0xf;
			bool add = (ins >> 23) & 1;
			
			// Calculate load address
			size_t addr = orig_ip + 8;
			(add) ? (addr += imm12) : (addr -= imm12);
			
			// Write the value that is wanted to load
			fixup_block[0] = *(uint32_t *)addr;
			
			// Write the instruction to load it
			// Note: the constant U=1 is a bit jank but should work alright
			code[i] = AARCH32_MAKE_LDR_LITERAL(1, Rt, (size_t)fixup_block - (size_t)&code[i]);
			
			// inc fixup block
			fixup_block++;
		}
		#endif
	}
}

// Function hook manager
int KNHookManagerInit(KNHookManager *self) {
	memset(self, 0, sizeof *self);
	
	// Set bytes per hook
	#if defined(__ARM_ARCH_7A__)
	self->bytes_per_longjump = 12;
	#elif defined(__aarch64__)
	self->bytes_per_longjump = 16;
	#else
	#error Arch not supported
	#endif
	
	// Try to mmap() some memory for the hooks.
	// TODO: Make this more flexibly sized.
	self->code_alloced = 4 * getpagesize();
	self->code = mmap(NULL, self->code_alloced, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	return (self->code) ? 0 : -1; // behaves like a syscall
}

int KNHookManagerHookFunction(KNHookManager *self, void *func_to_hook, void *new_func, void **hooked_func_start) {
	/**
	 * Hook a function, assuming there is no PC-relative code within the first
	 * few bytes of the code as this will not be compensated for. That's TODO!
	 */
	
	// NOTE:
	// The layout of hook is:
	//  - 3/4 original instructions
	//  - jump back to original functions (3/4 instr)
	//  - 
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Hook %p -> %p", func_to_hook, new_func);
	
	// This is: (2 * (bytes per longjump) + needed rewrite bytes)
	// since there is the replaced instructions, the longjump back to the
	// original code, and then the rewrite scratch block which should have
	// enough space to cotain addresses for all three instructions at the start
	const size_t size_per_hook = (2 * self->bytes_per_longjump + sizeof(size_t) * (self->bytes_per_longjump / 4));
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Size per hook = 0x%zx", size_per_hook);
	
	// Check that there is enough space
	if (self->hook_count * size_per_hook > self->code_alloced) {
		return -1;
	}
	
	// Addr of next available stub for start of func/jump back
	void *stub = self->code + (self->hook_count * size_per_hook);
	
	__android_log_print(ANDROID_LOG_INFO, TAG, "Will make stub at %p", stub);
	
	// Copy start of func to stub
	memcpy(stub, func_to_hook, self->bytes_per_longjump);
	
	// Do any needed fixups for PC-relative instructions
	__android_log_print(ANDROID_LOG_INFO, TAG, "rewrite with scratch block at (stub @ %p) + 0x%zx = %p", stub, 2 * self->bytes_per_longjump, stub + (2 * self->bytes_per_longjump));
	
	KNRewriteBlock(stub, self->bytes_per_longjump, stub + (2 * self->bytes_per_longjump), func_to_hook);
	
	// Make our jump back to the original function, let's just hope it didnt
	// need ip or x16...
	KNCreateLongJump(stub + self->bytes_per_longjump, func_to_hook + self->bytes_per_longjump);
	
	// Finally let's install the hook
	KNCreateLongJump(func_to_hook, new_func);
	
	// Set hooked_func_start to the new start of the function (i.e. the stub)
	hooked_func_start[0] = stub;
	
	// Increment hook count
	self->hook_count++;
	
	// Return 0 for success (we hope)
	return 0;
}
