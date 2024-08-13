#include <android/log.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *base;
    size_t size;
    size_t pos;
} NXArchiveExtractor;

#define log __android_log_print
// HACK too lazy to include the real header
#define PATH_MAX 2048
typedef uint32_t errno_t;

// Partially correct wrappers for C11 _s functions
static errno_t strcpy_s(char *restrict dest, size_t destsz, const char *restrict src) {
    if (!dest || !src || destsz == 0) {
        return 1;
    }
    
    if (destsz < (strlen(src) + 1)) {
        return 1;
    }
    
    strcpy(dest, src);
    
    return 0;
}

static errno_t strcat_s(char *restrict dest, size_t destsz, const char *restrict src) {
    if (!dest || !src || destsz == 0) {
        return 1;
    }
    
    int foundnul = 0;
    for (size_t i = 0; i < destsz; i++) {
        if (dest[i] == '\0') {
            foundnul = 1;
            break;
        }
    }
    
    if (!foundnul) {
        return 1;
    }
    
    if ((strlen(dest) + strlen(src) + 1) > destsz) {
        return 1;
    }
    
    strcat(dest, src);
    
    return 0;
}

static void NXWriteBufToFile(const char *filename, size_t size, const void *buf) {
    FILE *f = fopen(filename, "wb");
    
    if (!f) {
        log(ANDROID_LOG_ERROR, "NXArchive", "Could not write file: %s", filename);
        return;
    }
    
    int size_written = fwrite(buf, 1, size, f);
    
    if (size_written != size) {
        log(ANDROID_LOG_WARN, "NXArchive", "Did not fully write file");
    }
    
    fclose(f);
}

static void NXFromBuffer(NXArchiveExtractor *this, size_t size, const void *buf) {
    /**
     * Create an NXArchiveExtractor from an existing buffer.
     */
    
    this->base = (const char *) buf;
    this->size = size;
    this->pos = 0;
}

static int NXReadIntoBuf(NXArchiveExtractor *this, size_t size, void *buf) {
    /**
     * Read size bytes into the buffer pointed to by buf. Returns 1 on failure
     * and 0 on success.
     * 
     * Buf may also be null, in which case those bytes are simpily skipped.
     */
    
    // Protect us from anything horrid
    if ((this->pos + size) > this->size) {
        return 1;
    }
    
    // Copy memory if we've got enough buffer left
    if (buf) {
        memcpy(buf, this->base + this->pos, size);
    }
    
    // Increment position in stream
    this->pos += size;
    
    return 0;
}

static const void *NXGetPointer(NXArchiveExtractor *this, size_t size) {
    /**
     * Get a pointer to the current position in the buffer, but only if the
     * remaining buffer is at least `size` bytes long. Increments the stream
     * position by size.
     */
    
    if ((this->size - this->pos) < size) {
        return NULL;
    }
    
    const void *result = (const void *)(this->base + this->pos);
    
    this->pos += size;
    
    return result;
}

static const char *NXGetStringPointer(NXArchiveExtractor *this, size_t size) {
    /**
     * Get a pointer to the current position in the buffer, but only if the
     * remaining buffer is at least `size` bytes long and buf[size - 1] is NUL.
     * Increments the stream position by size.
     * 
     * This is a pretty efficent way of getting a (const) NUL-terminated string.
     */
    
    if (((this->size - this->pos) < size) || this->base[this->pos + size - 1] != '\0') {
        return NULL;
    }
    
    const char *result = (const char *)(this->base + this->pos);
    
    this->pos += size;
    
    return result;
}

static uint32_t NXReadInt(NXArchiveExtractor *this) {
    /**
     * Read the next integer from this stream.
     */
    
    uint32_t data;
    
    if (NXReadIntoBuf(this, sizeof data, &data)) {
        log(ANDROID_LOG_ERROR, "NXArchive", "*** Failed to read an Int32, return 0. ***");
        return 0;
    }
    
    return data;
}

static const char *NXExtractArchive(NXArchiveExtractor *this, const char *location) {
    /**
     * Extract an NXArchive. Returns NULL on success, pointer to a string
     * explaining the error on failure.
     */
    
    size_t location_size = strlen(location);
    char tmp_path[PATH_MAX];
    
    if (strcpy_s(tmp_path, PATH_MAX, location)) { return "strcpy_s failed for location"; }
    if (strcat_s(tmp_path, PATH_MAX, "/")) { return "strcpy_s failed for slash"; }
    
    while (1) {
        uint32_t cmd = NXReadInt(this);
        
        switch (cmd) {
            case 100: {
                if (NXReadInt(this) != 0x584e0001) {
                    log(ANDROID_LOG_ERROR, "NXArchive", "*** Invalid archive format ***");
                    return "invalid archive format";
                }
                log(ANDROID_LOG_INFO, "NXArchive", "Starting to extract archive to %s", location);
                break;
            }
            case 200: {
                // Read name and strcat it to path
                uint32_t nameSize = NXReadInt(this);
                const char *name = NXGetStringPointer(this, nameSize);
                if (!name || !nameSize) { return "can't read file name string"; }
                if (strcat_s(tmp_path, PATH_MAX, name)) { return "can't strcat_s file name string"; }
                
                log(ANDROID_LOG_INFO, "NXArchive", "Extracting %s", tmp_path);
                
                // Get file size and buffer pointer then write it
                uint32_t fileSize = NXReadInt(this);
                const void *fileBuf = NXGetPointer(this, fileSize);
                if (!fileBuf) { return "can't read file buffer"; }
                NXWriteBufToFile(tmp_path, fileSize, fileBuf);
                
                // Restore old tmp_path
                // this is really location size + size of "/"
                tmp_path[location_size + 1] = '\0';
                
                break;
            }
            case 201: {
                // Basically the same as with writing out a new file
                uint32_t nameSize = NXReadInt(this);
                const char *name = NXGetStringPointer(this, nameSize);
                if (!name || !nameSize) { return "can't read dir name string"; }
                if (strcat_s(tmp_path, PATH_MAX, name)) { return "can't strcat_s dir name string"; }
                
                log(ANDROID_LOG_INFO, "NXArchive", "Creating directory %s", tmp_path);
                
                // Make the directory (ignore errors whilst doing so)
                mkdir(tmp_path, 0777);
                
                // Restore old tmp_path
                // this is really location size + size of "/"
                tmp_path[location_size + 1] = '\0';
               
                break;
            }
            case 300: {
                log(ANDROID_LOG_INFO, "NXArchive", "Finished extracting archive");
                return NULL;
            }
            default: {
                log(ANDROID_LOG_ERROR, "NXArchive", "*** Invalid command: %u ***", cmd);
                return "invalid nxarchive command";
            }
        }
    }
}

const char *NXExtractArchiveFromBuffer(const char *location, size_t size, const void *buf) {
    /**
     * Extract an archive to the given location from a source buffer of size.
     */
    
    NXArchiveExtractor nx;
    NXFromBuffer(&nx, size, buf);
    return NXExtractArchive(&nx, location);
}
