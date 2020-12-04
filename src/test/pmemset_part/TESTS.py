#!../env.py
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation
#

import testframework as t
from testframework import granularity as g


@g.require_granularity(g.ANY)
class PMEMSET_PART(t.Test):
    test_type = t.Short
    create_file = True

    def run(self, ctx):
        filepath = "not/existing/file"
        if self.create_file:
            filepath = ctx.create_holey_file(16 * t.MiB, 'testfile1')
        ctx.exec('pmemset_part', self.test_case, filepath)


class TEST0(PMEMSET_PART):
    """test pmemset_part allocation with error injection"""
    test_case = "test_part_new_enomem"
    create_file = True


class TEST1(PMEMSET_PART):
    """create a new part from a source with invalid path assigned"""
    test_case = "test_part_new_invalid_source_file"
    create_file = False


class TEST2(PMEMSET_PART):
    """create a new part from a source with valid path assigned"""
    test_case = "test_part_new_valid_source_file"
    create_file = True


class TEST3(PMEMSET_PART):
    """create a new part from a source with valid pmem2_source assigned"""
    test_case = "test_part_new_valid_source_pmem2"
    create_file = True


class TEST4(PMEMSET_PART):
    """create a new part  from a source with valid pmem2_source and map it"""
    test_case = "test_part_map_valid_source_pmem2"
    create_file = True


class TEST5(PMEMSET_PART):
    """create a new part from a source with valid file path and map it"""
    test_case = "test_part_map_valid_source_file"
    create_file = True


class TEST6(PMEMSET_PART):
    """test pmemset_part_map with invalid offset"""
    test_case = "test_part_map_invalid_offset"
    create_file = True


@t.windows_exclude
@t.require_valgrind_enabled('memcheck')
class TEST7(PMEMSET_PART):
    """create a new part from a source with valid file path and map it"""
    test_case = "test_part_map_valid_source_file"
    create_file = True


class TEST8(PMEMSET_PART):
    """test if data is unavailable after pmemset_delete"""
    test_case = "test_unmap_part"
    create_file = True
