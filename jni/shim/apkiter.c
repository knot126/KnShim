/**
 * Iterate over the filenames of files in a ZIP or APK file.
 * 
 * -----------------------------------------------------------------------------
 * 
 * This file is part of KnShim. Copyright (c) 2025 Knot126.
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

#include <stdio.h>

#include "apkiter.h"

#define READ_TYPE(F, V) fread(&V, sizeof V, 1, F)
#define READ(F, S, B) fread(B, S, 1, F)

int KnShim_ForEachZIPFileEntry(const char *zip_path, void *user_context, APKIterationCallback callback) {
	FILE *file = fopen(zip_path, "rb");
	
	if (!file) {
		return KN_ZIP_ITERATOR_IO_ERROR;
	}
	
	unsigned char magic[4];
	unsigned int compressed_size;
	unsigned short file_name_length;
	unsigned short extra_field_length;
	
	while (1) {
		if (feof(file)) {
			break;
		}
		
		if (READ(file, 4, magic) < 4) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		if (magic[0] != 0x50 || magic[1] != 0x4B || magic[2] != 0x03 || magic[3] != 0x04) {
			break;
		}
		
		// skip a ton of feilds to get to compressed size
		if (fseek(file, 14, SEEK_CUR)) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		if (READ_TYPE(file, compressed_size) != 1) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		// skip uncompressed size
		if (fseek(file, 4, SEEK_CUR)) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		if (READ_TYPE(file, file_name_length) != 1) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		if (READ_TYPE(file, extra_field_length) != 1) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		char filename_buf[file_name_length+1];
		
		if (READ(file, file_name_length, filename_buf) < file_name_length) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		filename_buf[file_name_length] = '\0';
		
		int status = callback(user_context, filename_buf);
		
		if (!status) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED;
		}
		
		// Skip extra data
		if (fseek(file, extra_field_length, SEEK_CUR)) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
		
		// Skip file data
		if (fseek(file, compressed_size, SEEK_CUR)) {
			return KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR;
		}
	}
	
	return KN_ZIP_ITERATOR_FULLY_FINISHED;
}
