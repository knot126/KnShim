/**
 * Load from Smash Hit's native asset manager. The declaration for KNLoadAsset()
 * is in util.h.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <android/log.h>

#include "smashhit.h"
#include "util.h"

typedef struct KnMemoryOutputStream {
	QiOutputStream base;
	uint8_t *data;
	size_t size;
	size_t capacity;
} KnMemoryOutputStream;

QiOutputStreamVtable KnMemoryOutputStream_vtable;

void KnMemoryOutputStream_init(KnMemoryOutputStream *this) {
	memset(this, 0, sizeof *this);
	this->base.vtable = &KnMemoryOutputStream_vtable;
	this->base.byteOrder = 1;
}

bool KnMemoryOutputStream_flush(KnMemoryOutputStream *this) {
	return true;
}

bool KnMemoryOutputStream_write(KnMemoryOutputStream *this, uint8_t *data, size_t count) {
	// Increase capacity if we've run out of space
	if (this->capacity < this->size + count) {
		size_t new_cap = count + (2 * this->capacity);
		uint8_t *new_data = realloc(this->data, new_cap);
		
		if (!new_data) {
			return false;
		}
		else {
			this->capacity = new_cap;
			this->data = new_data;
		}
	}
	
	memcpy(this->data + this->size, data, count);
	this->size += count;
	
	return true;
}

bool KnMemoryOutputStream_accquireData(KnMemoryOutputStream *this, void **data, size_t *size) {
	uint8_t *trimmed = realloc(this->data, this->size + 1);
	
	if (trimmed) {
		trimmed[this->size] = '\0';
		if (data) { *data = trimmed; }
		if (size) { *size = this->size; }
		return true;
	}
	else {
		free(this->data);
		return false;
	}
}

void KnMemoryOutputStream_release(KnMemoryOutputStream *this) {
	free(this->data);
}

QiOutputStreamVtable KnMemoryOutputStream_vtable = {
	NULL,
	NULL,
	KnMemoryOutputStream_flush,
	KnMemoryOutputStream_write,
};

bool (*ResMan_load)(ResMan *this, QiString *path, QiOutputStream *output);

bool KNLoadAsset(const char *path, void **data, size_t *size) {
	/**
	 * Load an asset, with a extra NUL byte at the end (not counted as part of
	 * length). Return true on success or false on failure. Size pointer is
	 * optional.
	 * 
	 * You will need to free() the pointer returned in data if successful.
	 */
	
	if (!ResMan_load) {
		ResMan_load = KNGetSymbolAddr("_ZN6ResMan4loadERK8QiStringR14QiOutputStream");
	}
	
	ResMan *resMan = gGame->resman;
	
	QiString qPath = {
		.data = (char *)path,
		.length = strlen(path),
	};
	
	KnMemoryOutputStream os;
	KnMemoryOutputStream_init(&os);
	
	bool status = ResMan_load(resMan, &qPath, (QiOutputStream *) &os);
	
	if (status) {
		status = KnMemoryOutputStream_accquireData(&os, data, size);
	}
	else {
		KnMemoryOutputStream_release(&os);
	}
	
	return status;
}
