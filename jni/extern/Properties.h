/**
 * *Very* simple key-value config parser.
 * 
 * To parse a config, call:
 * KPProperties *props = KPParse("my.cool=config\n file=/data");
 * 
 * To get a value for a key:
 * const char *value = KPGet(props, "my.cool"); // "config"
 * 
 * To get a value for a key, with a fallback value:
 * const char *value = KPGetWithFallback(props, "file", "/usr/test"); // "/data"
 * const char *value = KPGetWithFallback(props, "socket", "120"); // "120"
 * 
 * To clean everything up, just call free() on the properties object:
 * free(props);
 */

#ifndef _KNOT_PROPERTIES_H_
#define _KNOT_PROPERTIES_H_

typedef struct KPProperties KPProperties;

KPProperties *KPParse(const char *content);
const char *KPGetWithFallback(KPProperties *self, const char *target_key, const char *fallback);
#define KPGet(SELF, TARGET_KEY) (KPGetWithFallback(SELF, TARGET_KEY, NULL))

#endif // _KNOT_PROPERTIES_H_

#ifdef KNOT_PROPERTIES_IMPLEMENTATION

#define KP_NEWLINE "\r\n"
#define KP_WHITESPACE " \t\v\f"
#define KP_SEP ":="

#include <string.h>
#include <stdlib.h>

typedef struct KPProperties {
	int sb_last;
	short count;
	char bank[];
} KPProperties;

#define KP_FIND_IN_SET(STR, SET) (strpbrk(STR, SET) ? strpbrk(STR, SET) : ((STR) + strlen(STR)))

#define KP_WRITE_STRING(STR, LEN) \
	memcpy(props->bank + props->sb_last, STR, LEN);\
	props->bank[props->sb_last + (LEN)] = '\0';\
	props->sb_last += (LEN) + 1;

static inline size_t KPUpperSizeBound(const char *content) {
	size_t size = strlen(content);
	
	while (*(content++) != '\0') {
		if (*content == '\r' || *content == '\n') {
			size += 1;
		}
	}
	
	return size;
}

KPProperties *KPParse(const char * const content) {
	KPProperties *props = malloc(sizeof *props + KPUpperSizeBound(content));
	
	if (!props) {
		return NULL;
	}
	
	props->count = 0;
	props->sb_last = 0;
	
	const char *line = content;
	
	while (line) {
		const char * const line_end_at = KP_FIND_IN_SET(line, KP_NEWLINE);
		const char * const next_line = line_end_at + strspn(line_end_at, KP_NEWLINE);
		
		// Skip comments
		if (line[0] == '#') {
			line = next_line;
			continue;
		}
		
		// Skip any whitespace
		line += strspn(line, KP_WHITESPACE);
		// line = key now
		
		// Start of separator
		const char *sepstart = KP_FIND_IN_SET(line, KP_WHITESPACE KP_SEP KP_NEWLINE);
		
		// Skip that to start of value
		const char *valstart = sepstart + strspn(sepstart, KP_WHITESPACE KP_SEP);
		
		const int key_len = (sepstart - line);
		const int val_len = (line_end_at - valstart);
		
		// Add string to string bank
		KP_WRITE_STRING(line, key_len);
		KP_WRITE_STRING(valstart, val_len);
		
		// Inc property count
		props->count++;
		
		line = next_line;
		
		if (line[0] == '\0') {
			break;
		}
	}
	
	return props;
}

const char *KPGetWithFallback(KPProperties *self, const char *target_key, const char *fallback) {
	const char *str = self->bank;
	
	for (size_t i = 0; i < self->count; i++) {
		const char *key = str;
		const char *val = str + strlen(str) + 1;
		
		// printf("[SEARCH] %s %s\n", key, val);
		
		if (!strcmp(key, target_key)) {
			return val;
		}
		
		str = val + strlen(val) + 1;
	}
	
	return fallback;
}
#endif // KNOT_PROPERTIES_IMPLEMENTATION
