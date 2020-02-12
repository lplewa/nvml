/*
 * Copyright 2019-2020, Intel Corporation
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
 * config_linux.c -- linux specific pmem2_config implementation
 */
#include <errno.h>
#include <fcntl.h>
#include "config.h"
#include "os.h"
#include "out.h"
#include "pmem2_utils.h"

/*
 * pmem2_config_set_fd -- sets fd in config struct
 */
int
pmem2_config_set_fd(struct pmem2_config *cfg, int fd)
{
	if (fd < 0) {
		cfg->fd = INVALID_FD;
		return 0;
	}

	int flags = fcntl(fd, F_GETFL);

	if (flags == -1) {
		ERR("!fcntl");
		if (errno == EBADF)
			return PMEM2_E_INVALID_FILE_HANDLE;
		return PMEM2_E_ERRNO;
	}

	if ((flags & O_ACCMODE) == O_WRONLY) {
		ERR("fd must be open with O_RDONLY or O_RDWR");
		return PMEM2_E_INVALID_FILE_HANDLE;
	}

	/*
	 * XXX Files with FS_APPEND_FL attribute should also generate an error.
	 * If it is possible to filter them out pmem2_map would not generate
	 * -EACCESS trying to map them. Please update pmem2_map.3 when it will
	 * be fixed. For details please see the ioctl_iflags(2) manual page.
	 */

	os_stat_t st;

	if (os_fstat(fd, &st) < 0) {
		ERR("!fstat");
		if (errno == EBADF)
			return PMEM2_E_INVALID_FILE_HANDLE;
		return PMEM2_E_ERRNO;
	}

	enum pmem2_file_type type;
	int ret = pmem2_get_type_from_stat(&st, &type);
	if (ret)
		return ret;

	if (type == PMEM2_FTYPE_DIR) {
		ERR("cannot set fd to directory in pmem2_config");
		return PMEM2_E_INVALID_FILE_TYPE;
	}

	cfg->fd = fd;
	return 0;
}

/*
 * pmem2_config_get_file_size -- get a file size of the file handle stored in
 * the provided config
 */
int
pmem2_config_get_file_size(const struct pmem2_config *cfg, size_t *size)
{
	LOG(3, "fd %d", cfg->fd);

	if (cfg->fd == INVALID_FD) {
		ERR("cannot check size for invalid file descriptor");
		return PMEM2_E_FILE_HANDLE_NOT_SET;
	}

	os_stat_t st;

	if (os_fstat(cfg->fd, &st) < 0) {
		ERR("!fstat");
		if (errno == EBADF)
			return PMEM2_E_INVALID_FILE_HANDLE;
		return PMEM2_E_ERRNO;
	}

	enum pmem2_file_type type;
	int ret = pmem2_get_type_from_stat(&st, &type);
	if (ret)
		return ret;

	switch (type) {
		case PMEM2_FTYPE_DIR:
			ERR(
				"asking for size of a directory doesn't make any sense in context of pmem");
			return PMEM2_E_INVALID_FILE_TYPE;
		case PMEM2_FTYPE_DEVDAX: {
			int ret = pmem2_device_dax_size_from_stat(&st, size);
			if (ret)
				return ret;
			break;
		}
		case PMEM2_FTYPE_REG:
			if (st.st_size < 0) {
				ERR(
					"kernel says size of regular file is negative (%ld)",
					st.st_size);
				return PMEM2_E_INVALID_FILE_HANDLE;
			}
			*size = (size_t)st.st_size;
			break;
		default:
			FATAL(
				"BUG: unhandled file type in pmem2_config_get_file_size");
	}

	LOG(4, "file length %zu", *size);
	return 0;
}

/*
 * pmem2_config_get_alignment -- get alignment from the file handle stored in
 * the provided config
 */
int
pmem2_config_get_alignment(const struct pmem2_config *cfg, size_t *alignment)
{
	LOG(3, "fd %d", cfg->fd);

	if (cfg->fd == INVALID_FD) {
		ERR(
			"cannot determine the alignment if a file descriptor is not set");
		return PMEM2_E_FILE_HANDLE_NOT_SET;
	}

	os_stat_t st;

	if (os_fstat(cfg->fd, &st) < 0) {
		ERR("!fstat");
		if (errno == EBADF)
			return PMEM2_E_INVALID_FILE_HANDLE;
		return PMEM2_E_ERRNO;
	}

	enum pmem2_file_type type;
	int ret = pmem2_get_type_from_stat(&st, &type);
	if (ret)
		return ret;

	switch (type) {
			case PMEM2_FTYPE_DIR:
				ERR(
					"asking for alignment of a directory doesn't make any sense");
				return PMEM2_E_INVALID_FILE_TYPE;
			case PMEM2_FTYPE_DEVDAX: {
				int ret = pmem2_device_dax_alignment_from_stat(
						&st, alignment);
				if (ret)
					return ret;
				break;
			}
			case PMEM2_FTYPE_REG:
				*alignment = Pagesize;
				break;
			default:
				FATAL(
					"BUG: unhandled file type in pmem2_config_get_alignment");
		}

	if (!util_is_pow2(*alignment)) {
		ERR("alignment (%zu) has to be a power of two", *alignment);
		return PMEM2_E_INVALID_ALIGNMENT_VALUE;
	}

	LOG(4, "alignment %zu", *alignment);

	return 0;
}

/*
 * pmem2_config_set_address -- sets address and mmap flag in the config struct
 */
int
pmem2_config_set_address(struct pmem2_config *cfg, void *addr, int type)
{
	if (type & ~PMEM2_E_MAP_VALID_FLAGS) {
		ERR("invalid flags 0x%x", type);
		return PMEM2_E_INVALID_MMAP_FLAG;
	}

	/*
	 * after support to PMEM2_ADDRESS_FIXED_REPLACE, this flag should be
	 * added to this if statement
	 */
	if (type == PMEM2_ADDRESS_FIXED_NOREPLACE && !addr) {
		ERR("cannot use flag PMEM2_ADDRESS_FIXED_NOREPLACE with"
				" addr being NULL");
		return PMEM2_E_INVALID_MMAP_FLAG;
	}

	cfg->flags = type;
	cfg->addr = addr;

	return 0;
}
