/**
 * Leaf - a single header ELF loader.
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

typedef struct Leaf {
	LeafEhdr *ehdr;
	LeafPhdr **phdrs;
	void *blob;
	size_t blob_length;
	void **dl_handles;
	size_t dl_handle_count;
	const char *strtab;
	LeafSym *symtab;
	size_t sym_count;
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
void LeafFree(Leaf *self);

#ifdef LEAF_IMPLEMENTATION

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
	__android_log_print(ANDROID_LOG_INFO, "leaflib", "__cxa_atexit(<%p>, <%p>, <%p>)", func, arg, dso_handle);
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

static void *LeafMakeMap(size_t size) {
	return mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

uint8_t ELF_SIGNATURE[] = {0x7f, 'E', 'L', 'F'};

void LeafDoRela(Leaf *self, LeafRela *relocs, size_t reloc_count);
void LeafDoRel(Leaf *self, LeafRel *relocs, size_t reloc_count);

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
	
	// Determine how much memory we need to map based on loadable segment sizes
	// since base address == 0 for ET_DYN we can just use the highest VirtAddr
	// + MemSiz value
	size_t highest = 0;
	
	for (size_t i = 0; i < phnum; i++) {
		if (self->phdrs[i]->p_type == PT_LOAD) {
			// don't need to check if its larger, PT_LOAD's should be sorted
			// by p_vaddr from low->high
			highest = self->phdrs[i]->p_vaddr + self->phdrs[i]->p_memsz;
		}
	}
	
	__android_log_print(ANDROID_LOG_INFO, "leaflib", "leaf: highest value = 0x%zx, mapping...\n", highest);
	
	// Map memory for loadable segments, copy their contents
	self->blob = LeafMakeMap(highest);
	self->blob_length = highest;
	
	if (self->blob == MAP_FAILED) {
		return strerror(errno);
	}
	
	// load code, data, etc and also find location of dynamic symbol table
	__android_log_print(ANDROID_LOG_INFO, "leaflib", "leaf: mapped at <%p>, copying...\n", self->blob);
	
	LeafDyn *dyns = NULL;
	
	for (size_t i = 0; i < phnum; i++) {
		LeafPhdr *phdr = self->phdrs[i];
		
		if (phdr->p_type == PT_LOAD) {
			LeafStreamSetpos(stream, phdr->p_offset);
			LeafStreamReadInto(stream, phdr->p_filesz, self->blob + phdr->p_vaddr);
		}
		else if (phdr->p_type == PT_DYNAMIC) {
			LeafStreamSetpos(stream, phdr->p_offset);
			dyns = LeafStreamGetptr(stream);
		}
		// I think we can ignore the PT_GNU_STACK and PT_GNU_RELRO, but maybe
		// not PT_GNU_EH_FRAME ?
	}
	
	if (!dyns) {
		return "Failed to find dynamic info";
	}
	
	// Get information from dynamic segment
	// WARNING: Lots of unimplemented stuff here, only implemented what's from
	// libsmashhit.so
	// NOTE: try only to use things from the loaded blob now
	const char *strtab = NULL;
	size_t strtab_size;
	
	size_t reloc_types; // HACK the entire handling of reloc types is hacky
	
	LeafRela *relocs = NULL;
	size_t reloc_size;
	size_t reloc_ent_size;
	
	LeafRela *plt_relocs = NULL;
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
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Leaf: DT_NEEDED 0x%zx\n", dyns[i].d_un.d_val);
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
				sym_count = ((Elf32_Word *)(self->blob + dyns[i].d_un.d_ptr))[1];
				break;
			}
			case DT_STRTAB: {
				strtab = self->blob + dyns[i].d_un.d_ptr;
				break;
			}
			case DT_SYMTAB: {
				symtab = self->blob + dyns[i].d_un.d_ptr;
				break;
			}
			case DT_RELA: {
				relocs = self->blob + dyns[i].d_un.d_ptr;
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
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Leaf: DT_SYMBOLIC\n");
				break;
			}
			case DT_REL: {
				relocs = self->blob + dyns[i].d_un.d_ptr;
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
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Leaf: DT_BIND_NOW\n");
				break;
			}
			case DT_PLTREL: {
				// if (dyns[i].d_un.d_val != DT_RELA) {
				// 	return "Using ElfXX_Rel instead of ElfX_Rela for PLT is not supported";
				// }
				reloc_types = dyns[i].d_un.d_val;
				break;
			}
			case DT_JMPREL: {
				plt_relocs = self->blob + dyns[i].d_un.d_ptr;
				break;
			}
			case DT_INIT_ARRAY: {
				init_array = self->blob + dyns[i].d_un.d_ptr;
				break;
			}
			case DT_FINI_ARRAY: {
				fini_array = self->blob + dyns[i].d_un.d_ptr;
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
			default: {
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Unknown dynamic section entry: 0x%zx\n", dyns[i].d_tag);
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
	self->fini_array = fini_array;
	self->fini_count = fini_array_size / sizeof(void *);
	
	// Correct needed library string names
	for (size_t i = 0; i < self->dl_handle_count; i++) {
		self->dl_handles[i] += (size_t)strtab;
	}
	
	// Load dependent libraries
	for (size_t i = 0; i < self->dl_handle_count; i++) {
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Dep lib soname: %s\n", (char *)self->dl_handles[i]);
		self->dl_handles[i] = dlopen(self->dl_handles[i], RTLD_NOW | RTLD_GLOBAL);
		if (!self->dl_handles[i]) {
			__android_log_print(ANDROID_LOG_INFO, "leaflib", "Loading lib failed! Continuing anyways...\n");
		}
	}
	
	// Reloc everything in symbol table, load external symbols
	// TODO
	__android_log_print(ANDROID_LOG_INFO, "leaflib", "Have %zd symbols, fixing up symbol table...\n", sym_count);
	
	for (size_t i = 1; i < sym_count; i++) {
		LeafSym *sym = &symtab[i];
		
		switch (sym->st_shndx) {
			case SHN_ABS: {
				// "The symbol has an absolute value that will not change
				// because of relocation."
				break;
			}
			case SHN_COMMON: {
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Symbol with SHN_COMMON, is this the 90s!?\n");
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
					// __android_log_print(ANDROID_LOG_INFO, "leaflib", "Found symbol '%s' at <0x%zx>\n", symbol_name, sym->st_value);
				}
				else {
					__android_log_print(ANDROID_LOG_INFO, "leaflib", "Warning: External symbol named '%s' not found.\n", symbol_name);
				}
				
				break;
			}
			default: {
				// not a special case, just relocate relative to blob
				sym->st_value += (size_t) self->blob;
				break;
			}
		}
	}
	
	// Replace __cxa_atexit with our own dummy
	LeafSym *p_cxa_atexit_sym = LeafSymbolInfo(self, "__cxa_atexit");
	LeafSym *p_aeabi_atexit_sym = LeafSymbolInfo(self, "__aeabi_atexit");
	
	if (p_cxa_atexit_sym) {
		p_cxa_atexit_sym->st_value = (size_t) &Leaf__cxa_atexit;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Symbol __cxa_atexit not found for replacement\n");
	}
	
	if (p_aeabi_atexit_sym) {
		p_aeabi_atexit_sym->st_value = (size_t) &Leaf__cxa_atexit;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Symbol __aeabi_atexit not found for replacement\n");
	}
	
	// debug: basic dump of symbol table
	// __android_log_print(ANDROID_LOG_INFO, "leaflib", "symbol table after relocs:\n");
	// for (size_t i = 0; i < sym_count; i++) {
	// 	__android_log_print(ANDROID_LOG_INFO, "leaflib", "[%04zu] 0x%016zx %s\n", i, symtab[i].st_value, strtab + symtab[i].st_name);
	// }
	
	// Preform relocations
	size_t reloc_count = reloc_size / reloc_ent_size;
	size_t plt_reloc_count = plt_relocs_size / reloc_ent_size;
	
	if (reloc_types == DT_RELA) {
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Will preform %zu relocations (DT_RELA)...\n", reloc_count);
		LeafDoRela(self, relocs, reloc_count);
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Will preform %zu relocations (DT_JMPREL)...\n", plt_reloc_count);
		LeafDoRela(self, plt_relocs, plt_reloc_count);
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Will preform %zu relocations (DT_REL)...\n", reloc_count);
		LeafDoRel(self, (LeafRel*) relocs, reloc_count);
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Will preform %zu relocations (DT_JMPREL)...\n", plt_reloc_count);
		LeafDoRel(self, (LeafRel*) plt_relocs, plt_reloc_count);
	}
	
	// Call init functions
	// TODO
	size_t init_count = init_array_size / sizeof(void *);
	
	__android_log_print(ANDROID_LOG_INFO, "leaflib", "Calling %zu init functions...\n", init_count);
	
	for (size_t i = 0; i < init_count; i++) {
		void (*func)(void) = ((void(**)(void)) init_array)[i];
		
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Func addr: <%p>\n", func);
		
		if (func) {
			func();
		}
	}
	
	LeafStreamFree(stream); // TODO free if it fails
	
	return NULL;
}

void LeafDoRela(Leaf *self, LeafRela *relocs, size_t reloc_count) {
	for (size_t i = 0; i < reloc_count; i++) {
		LeafRela *rela = &relocs[i];
		
		void *where = self->blob + rela->r_offset;
		
		switch (LeafRelocType(rela->r_info)) {
			// TODO other arches
			case R_AARCH64_RELATIVE: {
				// I think this works (?) since all symbols are zero in my case.
				void *result = self->blob + rela->r_addend;
				*((void **)where) = result;
				break;
			}
			case R_AARCH64_GLOB_DAT:
			case R_AARCH64_JUMP_SLOT: {
				LeafSym *sym = &self->symtab[LeafRelocSym(rela->r_info)];
				*((size_t *)where) = sym->st_value + rela->r_addend;
				break;
			}
			default: {
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Unknown reloc type: offset=0x%zx sym=0x%zx type=0x%zx addend=0x%zx\n", rela->r_offset, LeafRelocSym(rela->r_info), LeafRelocType(rela->r_info), rela->r_addend);
				break;
			}
		}
	}
}

void LeafDoRel(Leaf *self, LeafRel *relocs, size_t reloc_count) {
	for (size_t i = 0; i < reloc_count; i++) {
		LeafRel *rel = &relocs[i];
		
		void *where = self->blob + rel->r_offset;
		
		switch (LeafRelocType(rel->r_info)) {
			// TODO other arches
			case R_ARM_RELATIVE: {
				void *result = self->blob + *((size_t *) where);
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
			default: {
				__android_log_print(ANDROID_LOG_INFO, "leaflib", "Unknown reloc type: offset=0x%zx sym=0x%zx type=0x%zx\n", rel->r_offset, LeafRelocSym(rel->r_info), LeafRelocType(rel->r_info));
				break;
			}
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
	
	__android_log_print(ANDROID_LOG_INFO, "leaflib", "Calling %d fini functions...", self->fini_count);
	
	// remember: run them backwards
	for (size_t i = 1; i <= self->fini_count; i++) {
		void(*func)(void) = self->fini_array[self->fini_count - i];
		
		__android_log_print(ANDROID_LOG_INFO, "leaflib", "Func addr: <%p>\n", func);
		
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
	
	// Everything else is just a pointer to something in the loaded program
	// memory...
	
	// Unmap program memory
	munmap(self->blob, self->blob_length);
	
	// Free own memory
	free(self);
	
	return;
}

#endif // LEAF_IMPLEMENTATION
#endif // LEAF_HEADER
