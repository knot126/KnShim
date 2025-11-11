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

#pragma once

/**
 * This function will be called for each file in the ZIP archive. `context` is
 * a user-defined context, `name` is a pointer to a filename which is owned by
 * the iterator, and the function returns `1` if the iteration should be
 * continued or `0` if it should be stopped after the current file.
 */
typedef int (*APKIterationCallback)(void *context, const char *name);

/**
 * Possible result codes returned by KnShim_ForEachZIPFileEntry().
 */
enum {
	KN_ZIP_ITERATOR_PARTLY_FINISHED_WITH_IO_ERROR = -2,
	KN_ZIP_ITERATOR_IO_ERROR = -1,
	KN_ZIP_ITERATOR_FULLY_FINISHED = 0,
	KN_ZIP_ITERATOR_PARTLY_FINISHED = 1,
};

/**
 * Iterate over each local file header in a ZIP file, calling callback each time
 * a new filename is found.
 */
int KnShim_ForEachZIPFileEntry(const char *zip_path, void *user_context, APKIterationCallback callback);
