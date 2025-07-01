/**
 * Leaf, a single header ELF loader. Supports 32-bit and 64-bit ARM, as well as
 * 32-bit x86. Always loads sections as RWX. Primarily targets Android, should
 * also work on GNU/Linux.
 * 
 * ****************************************************************************
 * 
 * Copyright (C) 2024 - 2025 Knot126
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LEAF_HEADER
#define LEAF_HEADER

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <elf.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>

#if defined(__arm__) || defined(__i386__)
#define LEAF_32BIT
#endif

#ifdef LEAF_32BIT
#define LEAF_CURRENT_CLASS 1
#define LeafEhdr Elf32_Ehdr
#define LeafPhdr Elf32_Phdr
#define LeafDyn  Elf32_Dyn
#define LeafRel  Elf32_Rel
#define LeafRela Elf32_Rela
#define LeafSym  Elf32_Sym
#define LeafAddr Elf32_Addr
#define LeafRelocSym(i) (i >> 8)
#define LeafRelocType(i) (i & 0xff)
#else
#define LEAF_CURRENT_CLASS 2
#define LeafEhdr Elf64_Ehdr
#define LeafPhdr Elf64_Phdr
#define LeafDyn  Elf64_Dyn
#define LeafRel  Elf64_Rel
#define LeafRela Elf64_Rela
#define LeafSym  Elf64_Sym
#define LeafAddr Elf64_Addr
#define LeafRelocSym(i) (i >> 32)
#define LeafRelocType(i) (i & 0xffffffff)
#endif

// Same for 32/64 bit
#define LeafSymBind(i) (i >> 4)
#define LeafSymType(i) (i & 0xf)

typedef struct LeafLoadedSegment {
	void *addr;
	size_t orig_addr;
	size_t size;
} LeafLoadedSegment;

typedef struct Leaf {
	// Headers
	LeafEhdr *ehdr;
	LeafPhdr **phdrs;
	
	// Segments of the program
	// 
	// Originally to implement loading things aligned I was going to load
	// segments sparsely, but it seems like that doesn't work with SH. I'm not
	// sure if that's actually conformant to the ELF spec since both sections
	// really.
	LeafLoadedSegment *segments;
	size_t segment_count;
	
#ifndef LEAF_LOAD_SPARSE
	void *block;
	size_t block_size;
#endif
	
	// dlopen() handles for libs required by this ELF
	void **dl_handles;
	size_t dl_handle_count;
	
	// Pointer to string and symbol table in the loaded binary
	const char *strtab;
	LeafSym *symtab;
	size_t sym_count;
	
	// Relocations and PLT relocations
	size_t reloc_types;
	
	void *relocs;
	size_t reloc_count;
	size_t reloc_ent_size;
	
	void *plt_relocs;
	size_t plt_reloc_count;
	
	// Init function array (not needed by the loader after loading, but kept for
	// reference)
	void **init_array;
	size_t init_count;
	
	// Finialisation function array
	void **fini_array;
	size_t fini_count;
} Leaf;

typedef struct LeafStream {
	uint8_t *data;
	size_t size;
	size_t pos;
} LeafStream;

Leaf *LeafInit(void);
const char *LeafLoadFromBuffer(Leaf *self, void *contents, size_t length);
const char *LeafLoadFromFile(Leaf *self, const char *path);
void *LeafSymbolAddr(Leaf *self, const char *symbol_name);
LeafSym *LeafSymbolInfo(Leaf *self, const char *symbol_name);
void *LeafGetRealAddr(Leaf *self, size_t virt_addr);
void LeafFree(Leaf *self);

#ifdef LEAF_IMPLEMENTATION

// Logging stuff dependent on platform
#ifndef LEAF_NO_LOGGING
	#ifdef __ANDROID__
		#include <android/log.h>
		#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "Leaf", __VA_ARGS__)
	#else
		#include <stdio.h>
		#define LOG(...) fprintf(stderr, __VA_ARGS__);
	#endif
#else
	#define LOG(...)
#endif

static LeafStream *LeafStreamInit(uint8_t *buffer, size_t size) {
	/**
	 * Makes a read stream around the given buffer
	 */
	
	LeafStream *self = malloc(sizeof *self);
	
	if (!self) {
		return NULL;
	}
	
	memset(self, 0, sizeof *self);
	
	self->data = buffer;
	self->size = size;
	
	return self;
}

static size_t LeafStreamReadInto(LeafStream *self, size_t count, void *buffer) {
	/**
	 * Read data from the stream into the given buffer, returns number of bytes
	 * read.
	 */
	
	if (self->pos + count > self->size) {
		return 0;
	}
	
	memcpy(buffer, self->data + self->pos, count);
	
	self->pos += count;
	
	return count;
}

static void *LeafStreamRead(LeafStream *self, size_t count) {
	if (self->pos + count > self->size) {
		return NULL;
	}
	
	void *data = malloc(count);
	
	if (!data) {
		return NULL;
	}
	
	LeafStreamReadInto(self, count, data);
	
	return data;
}

static size_t LeafStreamGetpos(LeafStream *self) {
	return self->pos;
}

static void *LeafStreamGetptr(LeafStream *self) {
	return self->data + self->pos;
}

static void LeafStreamSetpos(LeafStream *self, size_t pos) {
	self->pos = pos;
}

static void LeafStreamFree(LeafStream *self) {
	free(self);
}

////////////////////////////////////////////////////////////////////////////////
// Stub functions
/////////////////
static int Leaf__cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
	LOG("__cxa_atexit(<%p>, <%p>, <%p>)", func, arg, dso_handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Leaf itself
//////////////

Leaf *LeafInit(void) {
	/**
	 * Initialise a new instance of Leaf with the given parameters.
	 */
	
	Leaf *self = malloc(sizeof *self);
	
	if (!self) {
		return NULL;
	}
	
	memset(self, 0, sizeof *self);
	
	return self;
}

#define LEAF_ALIGN_UP(ADDR, ALIGN) ((ADDR) + ((ALIGN) - ((ADDR) % (ALIGN))))
#define LEAF_ALIGN_DOWN(ADDR, ALIGN) ((ADDR) - ((ADDR) % (ALIGN)))

static void *LeafMakeMap(size_t size, size_t alignment) {
	/**
	 * Makes a RWX, ANON memory map starting at an address aligned for the given
	 * alignment and that is at least size bytes.
	 * 
	 * Sadly there is no way (that I know of) to directly request aligned pages,
	 * so the next best solution is used: allocated more than is needed and trim
	 * off the unneeded parts using munmap(). Even the Android ELF loader uses
	 * this trick, so I think it's not all that jank at the end of things.
	 */
	
	// Size required if we need to always get at least size bytes of aligned
	// memory.
	const size_t req_size = size + alignment;
	
	// Map memory
	void * const addr = mmap(NULL, req_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if (addr == MAP_FAILED) {
		return MAP_FAILED;
	}
	
	const size_t start_addr = (size_t) addr;
	const size_t end_addr = LEAF_ALIGN_UP(start_addr + req_size, getpagesize());
	
	// Find aligned address of memory within the mapping
	const size_t aligned_addr = LEAF_ALIGN_UP(start_addr, alignment);
	
	// Calculate amount to trim from start and end of the mapping
	// Note that we need to take less from the after portition so we don't
	// accidentially trim off the last page
	const size_t trim_before = aligned_addr - start_addr;
	const size_t trim_after = LEAF_ALIGN_DOWN(alignment - trim_before, getpagesize());
	
	// Unmap parts
	if (trim_before) {
		munmap(addr, trim_before);
	}
	
	if (trim_after) {
		munmap((void *) (end_addr - trim_after), trim_after);
	}
	
	LOG("Map: Start=0x%zx End=0x%zx Aligned=0x%zx TrimBefore=0x%zx TrimAfter=0x%zx\n", start_addr, end_addr, aligned_addr, trim_before, trim_after);
	
	*((int *) aligned_addr) = 0;
	
	// Finally return aligned address
	return (void *) aligned_addr;
}

#define LEAF_IN_RANGE(A, X, B) ((X >= A) && (X < B))

void *LeafGetRealAddr(Leaf *self, size_t virt_addr) {
	/**
	 * Get the loaded address for a given virtual address in the binary.
	 */
	
	for (size_t i = 0; i < self->segment_count; i++) {
		const size_t start = self->segments[i].orig_addr;
		const size_t end = self->segments[i].orig_addr + self->segments[i].size;
		
		if (LEAF_IN_RANGE(start, virt_addr, end)) {
			return self->segments[i].addr + (virt_addr - self->segments[i].orig_addr);
		}
	}
	
	return NULL;
}

const uint8_t ELF_SIGNATURE[] = {0x7f, 'E', 'L', 'F'};

void LeafDoRela(Leaf *self, LeafRela *relocs, size_t reloc_count);
void LeafDoRel(Leaf *self, LeafRel *relocs, size_t reloc_count);
static void LeafDoInit(Leaf *self);

const char *LeafLoadFromBuffer(Leaf *self, void *contents, size_t length) {
	/**
	 * Returns a string containing details of the error that occured, or NULL
	 * on success
	 */
	
	// Init a read stream
	LeafStream *stream = LeafStreamInit(contents, length);
	
	// Read header
	self->ehdr = LeafStreamRead(stream, sizeof *self->ehdr);
	
	if (!self->ehdr) {
		return "Failed to read header";
	}
	
	if (memcmp(self->ehdr, ELF_SIGNATURE, 4)) {
		return "Invalid ELF file";
	}
	
	if (self->ehdr->e_ident[EI_CLASS] != LEAF_CURRENT_CLASS) {
		return "Incorrect binary class for this platform";
	}
	
	if (self->ehdr->e_ident[EI_DATA] != 1) {
		return "Big endian is not supported";
	}
	
	if (self->ehdr->e_ident[EI_VERSION] != 1) {
		return "Too new or invalid ELF version";
	}
	
	if (self->ehdr->e_type != ET_DYN) {
		return "Only loading shared objects is supported";
	}
	
	// TODO: e_machine
	
	// Program and section headers
	size_t phoff = self->ehdr->e_phoff;
	size_t phentsize = self->ehdr->e_phentsize;
	size_t phnum = self->ehdr->e_phnum;
	
	// Read program headers
	// https://www.sco.com/developers/gabi/2003-12-17/ch5.pheader.html
	self->phdrs = malloc((phnum + 1) * sizeof *self->phdrs);
	
	if (!self->phdrs) {
		return "Failed to alloc phdrs array";
	}
	
	self->phdrs[phnum] = NULL;
	
	LeafStreamSetpos(stream, phoff);
	
	for (size_t i = 0; i < phnum; i++) {
		LeafPhdr *phdr = LeafStreamRead(stream, phentsize);
		
		if (!phdr) {
			return "Failed to read a program header";
		}
		
		self->phdrs[i] = phdr;
	}
	
	// Count the number of LOAD headers (actual segments) so we can malloc()
	// the array of segment info structures.
	size_t segment_count = 0;
	
	for (size_t i = 0; i < phnum; i++) {
		if (self->phdrs[i]->p_type == PT_LOAD) {
			segment_count++;
		}
	}
	
	self->segment_count = segment_count;
	
	// Allocate memory for storing segment structures
	self->segments = malloc(segment_count * sizeof *self->segments);
	
	if (!self->segments) {
		return "Failed to allocate memory for segment information";
	}
	
	// Initialise segment structures, map memory and load segments
	LeafDyn *dyns = NULL;
#ifndef LEAF_LOAD_SPARSE
	size_t max_vaddr = 0;
	size_t max_align = 0;
#endif
	
	for (size_t i = 0, j = 0; i < phnum; i++) {
		LeafPhdr *phdr = self->phdrs[i];
		
		if (self->phdrs[i]->p_type == PT_LOAD) {
#ifdef LEAF_LOAD_SPARSE
			LeafLoadedSegment *seg = &self->segments[j];
			
			// Mind that for Leaf we ignore the flags (permissions) and always
			// use RWX. Maybe in the future we could only mark RW for pages
			// without execute but I don't think that's a problem right now.
			seg->addr = LeafMakeMap(phdr->p_memsz, phdr->p_align);
			seg->size = phdr->p_memsz;
			seg->orig_addr = phdr->p_vaddr;
			
			if (seg->addr == MAP_FAILED) {
				return strerror(errno);
			}
			
			// Load segment contents, or at least the ones we're supposed to
			LeafStreamSetpos(stream, phdr->p_offset);
			LeafStreamReadInto(stream, phdr->p_filesz, seg->addr);
			
			LOG("Section %zu  Addr=%p Size=0x%zx OrigAddr=0x%zx End=%p\n", j, seg->addr, seg->size, seg->orig_addr, seg->addr + seg->size);
			
			j++;
#else
			max_vaddr = ((phdr->p_vaddr + phdr->p_memsz) > max_vaddr) ? (phdr->p_vaddr + phdr->p_memsz) : max_vaddr;
			max_align = (phdr->p_align > max_align) ? phdr->p_align : max_align;
#endif
		}
		else if (phdr->p_type == PT_DYNAMIC) {
			// TODO: I'd like to actually load the dynamic segment into
			// permanent memory.
			LeafStreamSetpos(stream, phdr->p_offset);
			dyns = LeafStreamGetptr(stream);
		}
	}
	
#ifndef LEAF_LOAD_SPARSE
	LOG("Non sparse : LeafMakeMap(size=0x%zx, align=0x%zx)\n", max_vaddr, max_align);
	
	self->block_size = max_vaddr;
	self->block = LeafMakeMap(max_vaddr, max_align);
	
	if (self->block == MAP_FAILED) {
		return "Non sparse block allocation failed";
	}
	
	// *sigh* I don't really want to loop over everything a third goddamn time,
	// but I've been forced here. I was originally going to implement "spare"
	// section loading, so that both sections were loaded at 64K boundaries, but
	// it seems like Smash Hit does not contain information to relocate most
	// references properly...
	for (size_t i = 0, j = 0; i < phnum; i++) {
		LeafPhdr *phdr = self->phdrs[i];
		
		if (self->phdrs[i]->p_type == PT_LOAD) {
			LeafLoadedSegment *seg = &self->segments[j];
			
			seg->addr = self->block + phdr->p_vaddr;
			seg->size = phdr->p_memsz;
			seg->orig_addr = phdr->p_vaddr;
			
			LOG("Segment %zu  Addr=%p Size=0x%zx OrigAddr=0x%zx\n", j, seg->addr, seg->size, seg->orig_addr);
			
			// Load segment contents, or at least the ones we're supposed to
			LeafStreamSetpos(stream, phdr->p_offset);
			LeafStreamReadInto(stream, phdr->p_filesz, seg->addr);
			
			j++;
		}
	}
#endif
	
	if (!dyns) {
		return "Failed to find dynamic info";
	}
	
	// Get information from dynamic segment
	// WARNING: Lots of unimplemented stuff here, only implemented what's from
	// libsmashhit.so
	const char *strtab = NULL;
	size_t strtab_size;
	
	size_t reloc_types; // HACK the entire handling of reloc types is hacky
	
	void *relocs = NULL;
	size_t reloc_size;
	size_t reloc_ent_size;
	
	void *plt_relocs = NULL;
	size_t plt_relocs_size;
	
	LeafSym *symtab = NULL;
	size_t sym_count = 0;
	size_t sym_ent_size;
	
	void **init_array = NULL;
	size_t init_array_size;
	
	void **fini_array = NULL;
	size_t fini_array_size;
	
	for (size_t i = 0; dyns[i].d_tag != DT_NULL; i++) {
		switch (dyns[i].d_tag) {
			case DT_NEEDED: {
				LOG("Leaf: DT_NEEDED 0x%zx\n", (size_t)dyns[i].d_un.d_val);
				// TODO: check if it fails
				self->dl_handles = realloc(self->dl_handles, (self->dl_handle_count + 1) * sizeof *self->dl_handles);
				self->dl_handles[self->dl_handle_count] = (void *) dyns[i].d_un.d_ptr; // we will fix the pointers later
				self->dl_handle_count += 1;
				break;
			}
			case DT_PLTRELSZ: {
				plt_relocs_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_HASH: {
				Elf32_Word *p = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				sym_count = p[1];
				break;
			}
			case DT_STRTAB: {
				strtab = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_SYMTAB: {
				symtab = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_RELA: {
				relocs = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_RELASZ: {
				reloc_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_RELAENT: {
				reloc_ent_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_STRSZ: {
				strtab_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_SYMENT: {
				sym_ent_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_SYMBOLIC: {
				LOG("Leaf: DT_SYMBOLIC\n");
				break;
			}
			case DT_REL: {
				relocs = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_RELSZ: {
				reloc_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_RELENT: {
				reloc_ent_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_BIND_NOW: {
				LOG("Leaf: DT_BIND_NOW\n");
				break;
			}
			case DT_PLTREL: {
				reloc_types = dyns[i].d_un.d_val;
				break;
			}
			case DT_JMPREL: {
				plt_relocs = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_INIT_ARRAY: {
				init_array = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_FINI_ARRAY: {
				fini_array = LeafGetRealAddr(self, dyns[i].d_un.d_val);
				break;
			}
			case DT_INIT_ARRAYSZ: {
				init_array_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_FINI_ARRAYSZ: {
				fini_array_size = dyns[i].d_un.d_val;
				break;
			}
			case DT_SONAME: {
				break;
			}
			default: {
				LOG("Unknown dynamic section entry: 0x%zx\n", (size_t)dyns[i].d_tag);
				break;
			}
		}
	}
	
	if (!strtab) { return "Could not find string table address"; }
	if (!relocs) { return "Could not find relocs"; }
	if (!symtab) { return "Could not find symbol table address"; }
	if (!plt_relocs) { return "Could not find PLT relocs address"; }
	if (!init_array) { return "Could not find init array address"; }
	if (!fini_array) { return "Could not find fini array address"; }
	if (!sym_count) { return "Could not find number of symbols"; }
	
	// save stuff we might want later
	self->strtab = strtab;
	self->symtab = symtab;
	self->sym_count = sym_count;
	
	self->reloc_types = reloc_types;
	
	self->relocs = plt_relocs;
	self->reloc_count = reloc_size / reloc_ent_size;
	self->reloc_ent_size = reloc_ent_size;
	
	self->plt_relocs = plt_relocs;
	self->plt_reloc_count = plt_relocs_size / reloc_ent_size;
	
	self->init_array = init_array;
	self->init_count = init_array_size / sizeof(void *);
	
	self->fini_array = fini_array;
	self->fini_count = fini_array_size / sizeof(void *);
	
	// Correct needed library string names
	for (size_t i = 0; i < self->dl_handle_count; i++) {
		self->dl_handles[i] += (size_t)strtab;
	}
	
	// Load dependent libraries
	for (size_t i = 0; i < self->dl_handle_count; i++) {
		LOG("Dep lib soname: %s\n", (char *)self->dl_handles[i]);
		
		self->dl_handles[i] = dlopen(self->dl_handles[i], RTLD_NOW | RTLD_GLOBAL);
		
		if (!self->dl_handles[i]) {
			LOG("Loading lib failed! Continuing anyways...\n");
		}
	}
	
	// Reloc everything in symbol table, load external symbols
	// TODO
	LOG("Have %zd symbols, fixing up symbol table...\n", sym_count);
	
	for (size_t i = 1; i < sym_count; i++) {
		LeafSym *sym = &symtab[i];
		
		switch (sym->st_shndx) {
			case SHN_ABS: {
				// "The symbol has an absolute value that will not change
				// because of relocation."
				break;
			}
			case SHN_COMMON: {
				LOG("Symbol with SHN_COMMON, is this the 90s!?\n");
				break;
			}
			case SHN_UNDEF: {
				// resolve the symbol in the dumest way possible, also probably
				// not technically correct since ELF has stricter ordering
				// requirements than this but whateverthefuck.
				const char *symbol_name = strtab + sym->st_name;
				
				// dlsym(NULL, symbol_name) would be smarter but not sure if
				// that works in this case...
				for (size_t j = 0; j < self->dl_handle_count; j++) {
					if (self->dl_handles[j] != NULL) {
						void *symbol_value = dlsym(self->dl_handles[j], symbol_name);
						
						if (symbol_value) {
							sym->st_value = (LeafAddr) symbol_value;
							break;
						}
					}
				}
				
				if (sym->st_value) {
					// LOG("Found symbol '%s' at <0x%zx>\n", symbol_name, sym->st_value);
				}
				else {
					LOG("Warning: External symbol named '%s' not found.\n", symbol_name);
				}
				
				break;
			}
			default: {
				// not a special case, just relocate to it's loaded address
				sym->st_value = (LeafAddr) LeafGetRealAddr(self, sym->st_value);
				break;
			}
		}
	}
	
	// Replace __cxa_atexit with our own dummy
	// TODO: Is this realllly needed?
	LeafSym *p_cxa_atexit_sym = LeafSymbolInfo(self, "__cxa_atexit");
	LeafSym *p_aeabi_atexit_sym = LeafSymbolInfo(self, "__aeabi_atexit");
	
	if (p_cxa_atexit_sym) {
		p_cxa_atexit_sym->st_value = (size_t) &Leaf__cxa_atexit;
	}
	else {
		LOG("Symbol __cxa_atexit not found for replacement\n");
	}
	
	if (p_aeabi_atexit_sym) {
		p_aeabi_atexit_sym->st_value = (size_t) &Leaf__cxa_atexit;
	}
	else {
		LOG("Symbol __aeabi_atexit not found for replacement\n");
	}
	
	// debug: basic dump of symbol table
	// LOG("symbol table after relocs:\n");
	// for (size_t i = 0; i < sym_count; i++) {
	// 	LOG("[%04zu] 0x%016zx %s\n", i, symtab[i].st_value, strtab + symtab[i].st_name);
	// }
	
	// Preform relocations
	size_t reloc_count = reloc_size / reloc_ent_size;
	size_t plt_reloc_count = plt_relocs_size / reloc_ent_size;
	
	if (reloc_types == DT_RELA) {
		LOG("Will preform %zu relocations (DT_RELA)...\n", reloc_count);
		LeafDoRela(self, relocs, reloc_count);
		LOG("Will preform %zu relocations (DT_JMPREL)...\n", plt_reloc_count);
		LeafDoRela(self, plt_relocs, plt_reloc_count);
	}
	else {
		LOG("Will preform %zu relocations (DT_REL)...\n", reloc_count);
		LeafDoRel(self, relocs, reloc_count);
		LOG("Will preform %zu relocations (DT_JMPREL)...\n", plt_reloc_count);
		LeafDoRel(self, plt_relocs, plt_reloc_count);
	}
	
	// Call init functions
	LeafDoInit(self);
	
	LeafStreamFree(stream); // TODO free if it fails
	
	return NULL;
}

void LeafDoRela(Leaf *self, LeafRela *relocs, size_t reloc_count) {
	for (size_t i = 0; i < reloc_count; i++) {
		LeafRela *rela = &relocs[i];
		
		void *where = LeafGetRealAddr(self, rela->r_offset);
		
		switch (LeafRelocType(rela->r_info)) {
			// TODO other arches
#ifdef __aarch64__
			case R_AARCH64_RELATIVE: {
				// LOG("R_AARCH64_RELATIVE 0x%zx 0x%zx\n", (size_t)rela->r_offset, (size_t)rela->r_addend);
				// I think this works (?) since all symbols are zero in my case.
				void *result = LeafGetRealAddr(self, rela->r_addend);
				*((void **)where) = result;
				break;
			}
			case R_AARCH64_GLOB_DAT:
			case R_AARCH64_JUMP_SLOT: {
				// LOG("R_AARCH64_GLOB_DAT/R_AARCH64_JUMP_SLOT 0x%zx 0x%zx\n", (size_t)rela->r_offset, (size_t)rela->r_addend);
				LeafSym *sym = &self->symtab[LeafRelocSym(rela->r_info)];
				*((size_t *)where) = sym->st_value + rela->r_addend;
				break;
			}
#endif
			default: {
				LOG("Unknown reloc type: offset=0x%zx sym=0x%zx type=0x%zx addend=0x%zx\n", (size_t)rela->r_offset, (size_t)LeafRelocSym(rela->r_info), (size_t)LeafRelocType(rela->r_info), (size_t)rela->r_addend);
				break;
			}
		}
	}
}

void LeafDoRel(Leaf *self, LeafRel *relocs, size_t reloc_count) {
	for (size_t i = 0; i < reloc_count; i++) {
		LeafRel *rel = &relocs[i];
		
		void *where = LeafGetRealAddr(self, rel->r_offset);
		
		switch (LeafRelocType(rel->r_info)) {
			// TODO other arches
#ifdef __arm__
			case R_ARM_RELATIVE: {
				void *result = LeafGetRealAddr(self, *((size_t *) where));
				*((void **)where) = result;
				break;
			}
			case R_ARM_GLOB_DAT: {
				// <place> = (S + A) | T
				LeafSym *sym = &self->symtab[LeafRelocSym(rel->r_info)];
				*((size_t *)where) += (sym->st_value & 1) ? (sym->st_value ^ 1) : sym->st_value;
				*((size_t *)where) |= (LeafSymType(sym->st_info) == STT_FUNC && (sym->st_value & 1)) ? 1 : 0;
				break;
			}
			case R_ARM_JUMP_SLOT: {
				// From the manual for jump slots in REL form:
				// "In a REL form of this relocation the addend, A, is always 0."
				LeafSym *sym = &self->symtab[LeafRelocSym(rel->r_info)];
				*((size_t *) where) = sym->st_value;
				break;
			}
#endif
#ifdef __i386__
			case R_386_COPY: {
				break;
			}
			case R_386_RELATIVE: {
				// B + A
				void *result = LeafGetRealAddr(self, *((size_t *) where));
				*((void **)where) = result;
				break;
			}
			case R_386_GLOB_DAT: {
				// S
				LeafSym *sym = &self->symtab[LeafRelocSym(rel->r_info)];
				*((size_t *)where) = sym->st_value;
				break;
			}
			case R_386_JMP_SLOT: {
				// S
				LeafSym *sym = &self->symtab[LeafRelocSym(rel->r_info)];
				*((size_t *)where) = sym->st_value;
				break;
			}
#endif
			default: {
				LOG("Unknown reloc type: offset=0x%zx sym=0x%zx type=0x%zx\n", (size_t)rel->r_offset, (size_t)LeafRelocSym(rel->r_info), (size_t)LeafRelocType(rel->r_info));
				break;
			}
		}
	}
}

void LeafDoInit(Leaf *self) {
	LOG("Calling %zu init functions...\n", self->init_count);
	
	for (size_t i = 0; i < self->init_count; i++) {
		void (*func)(void) = ((void(**)(void)) self->init_array)[i];
		
		LOG("Func addr: <%p>\n", func);
		
		if (func) {
			func();
		}
	}
}

const char *LeafLoadFromFile(Leaf *self, const char *path) {
	FILE *file = fopen(path, "rb");
	
	if (!file) {
		return "Could not open file";
	}
	
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	uint8_t *data = malloc(length);
	
	if (!data) {
		fclose(file);
		return "Failed to allocate data";
	}
	
	if (fread(data, 1, length, file) != length) {
		fclose(file); free(data);
		return "Failed to read data";
	}
	
	fclose(file);
	
	const char *error = LeafLoadFromBuffer(self, data, length);
	
	free(data);
	
	return error;
}

void *LeafSymbolAddr(Leaf *self, const char *symbol_name) {
	/**
	 * Find the address of the given symbol.
	 * 
	 * TODO: hash table
	 */
	
	for (size_t i = 0; i < self->sym_count; i++) {
		if (strcmp(self->strtab + self->symtab[i].st_name, symbol_name) == 0) {
			return (void *) self->symtab[i].st_value;
		}
	}
	
	return NULL;
}

LeafSym *LeafSymbolInfo(Leaf *self, const char *symbol_name) {
	/**
	 * Find the info for the given symbol.
	 * 
	 * TODO: hash table
	 */
	
	for (size_t i = 0; i < self->sym_count; i++) {
		if (strcmp(self->strtab + self->symtab[i].st_name, symbol_name) == 0) {
			return &self->symtab[i];
		}
	}
	
	return NULL;
}

void LeafFinish(Leaf *self) {
	/**
	 * Use LeafFree() unless you are probably just going to rely on exiting the
	 * app to free the ELF data. This is fine for use in an atexit() handler
	 * with a global variable.
	 */
	
	LOG("Calling %zu fini functions...", self->fini_count);
	
	// remember: run them backwards
	for (size_t i = 1; i <= self->fini_count; i++) {
		void(*func)(void) = self->fini_array[self->fini_count - i];
		
		LOG("Func addr: <%p>\n", func);
		
		if (func) {
			func();
		}
	}
}

void LeafFree(Leaf *self) {
	/**
	 * Free a loaded binary and any associated resources
	 */
	
	// Call fini funcs
	LeafFinish(self);
	
	// Close and free dl_handles
	for (size_t i = 0; i < self->dl_handle_count; i++) {
		if (self->dl_handles[i]) {
			dlclose(self->dl_handles[i]);
		}
	}
	
	free(self->dl_handles);
	
	// Free program headers
	for (size_t i = 0; self->phdrs[i] != NULL; i++) {
		free(self->phdrs[i]);
	}
	
	free(self->phdrs);
	
	// Unmap segments and info structures
#ifdef LEAF_LOAD_SPARSE
	for (size_t i = 0; i < self->segment_count; i++) {
		LeafLoadedSegment *s = &self->segments[i];
		munmap(s->addr, s->size);
	}
#else
	munmap(self->block, self->block_size);
#endif
	
	free(self->segments);
	
	// Everything else is just a pointer to something in the loaded program
	// memory...
	
	// Free own memory
	free(self);
	
	return;
}

#endif // LEAF_IMPLEMENTATION
#endif // LEAF_HEADER
