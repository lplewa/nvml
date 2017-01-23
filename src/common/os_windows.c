/*
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * os_windows.c -- windows abstraction layer
 */
#include "util.h"
#include "os.h"

#define UTF8_BOM "﻿"
/* os_open -- XXX */
int
os_open(const utf8_t *pathname, int flags, ...)
{
	utf16_t *path = util_toUTF16(pathname);
	int ret = 0;
	if (path == NULL) {
		return -1;
	}
	/* there is no O_TMPFILE on windows */
	if (flags & O_CREAT) {
		va_list arg;
		va_start(arg, flags);
		mode_t mode = va_arg(arg, mode_t);
		va_end(arg);
		ret = _wopen(path, flags, mode);
	} else {
		ret = _wopen(path, flags);
	}
	free(path);

	if (ret != -1) {
		char bom[3];
		if (_read(ret, bom, 3) != 3 || memcmp(bom, UTF8_BOM, 3) != 0 )  {
			/* UTF-8 bom not found - reset file to the beginning */
			lseek(ret, 0, SEEK_SET);
		}
	}

	return ret;
  }

/* os_stat -- XXX */
int
os_stat(const char *pathname, os_stat_t *buf)
{
	utf16_t *path = util_toUTF16(pathname);
	if (path == NULL) {
		return -1;
	}

	int ret = _wstat64(path, buf);

	free(path);
	return ret;
}

/* os_unlink -- XXX */
int
os_unlink(const char *pathname)
{
	utf16_t *path = util_toUTF16(pathname);
	if (path == NULL) {
		return -1;
	}

	int ret = _wunlink(path);
	free(path);
	return ret;
}

int
os_access(const char *pathname, mode_t mode)
{
	utf16_t *path = util_toUTF16(pathname);
	if (path == NULL) {
		return -1;
	}

	int ret = _waccess(path, mode);
	free(path);
	return ret;
}

void
os_skipBOM(FILE *file)
{
	if (file == NULL)
		return;
	/* UTF-8 BOM + \0 */
	char bom[4];
	fgets(bom, 4, file);

	if (strcmp(bom, UTF8_BOM) != 0) {
		/* UTF-8 bom not found - reset file to the beginning */
		fseek(file, 0, SEEK_SET);
	}
}

FILE *
os_fopen(const char *pathname, const char *mode)
{
	utf16_t *path = util_toUTF16(pathname);
	if (path == NULL) {
		return NULL;
	}

	utf16_t *wmode = util_toUTF16(mode);
	if (path == NULL) {
		free(path);
		return NULL;
	}

	FILE *ret = _wfopen(path, wmode);

	free(path);
	free(wmode);

	os_skipBOM(ret);
	return ret;
}

FILE *
os_fdopen(int fd, const char *mode)
{
	FILE *ret = fdopen(fd, mode);
	os_skipBOM(ret);
	return ret;
}

int os_chmod(const char *pathname, mode_t mode)
{
	utf16_t *path = util_toUTF16(pathname);
	if (path == NULL) {
		return -1;
	}

	int ret = _wchmod(path, mode);
	free(path);
	return ret;
}