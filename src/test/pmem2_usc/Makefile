# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# src/test/pmem2_usc/Makefile -- build pmem2_usc unit test
#
TOP = ../../..

#vpath %.c $(TOP)/src/test/unittest
vpath %.c $(TOP)/src/libpmem2

INCS += -I$(TOP)/src/libpmem2
TARGET = pmem2_usc
OBJS += errormsg.o\
	fake_ndctl.o\
	ndctl_region_namespace.o\
	pmem2_usc.o\
	pmem2_utils.o\
	pmem2_utils_linux.o\
	source.o\
	source_posix.o\
	usc_ndctl.o\
	ut_pmem2_source.o\
	ut_pmem2_utils.o\

LIBPMEMCORE=y
include ../Makefile.inc
