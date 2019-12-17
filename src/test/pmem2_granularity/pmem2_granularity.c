/*
 * Copyright 2019, Intel Corporation
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
 * pmem2_granularity.c -- test for graunlarity functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "map.h"
#include "pmem2_granularity.h"
#include "unittest.h"
#include "ut_pmem2_config.h"
#include "ut_pmem2_utils.h"

size_t Is_nfit = 1;
size_t Pc_type = 7;
size_t Pc_capabilities;

/*
 * set_eadr -- set variable required for mocked functions
 */
static void
set_eadr()
{
	int is_eadr = atoi(os_getenv("IS_EADR"));
	if (is_eadr)
		Pc_capabilities = 3;
	else
		Pc_capabilities = 2;
}

/*
 * test_ctx -- essential parameters used by test
 */
struct test_ctx {
	int fd;
	enum pmem2_granularity requested_granularity;
	enum pmem2_granularity expected_granularity;
};

/*
 * init_test -- initialize basic parameters for test
 */
static void
init_test(int argc, char *argv[], struct test_ctx *ctx,
		enum pmem2_granularity granularity)
{
	if (argc != 1)
		UT_FATAL("usage: test_granularity_inval <file>");

	set_eadr();

	char *file = argv[0];
	ctx->fd = OPEN(file, O_RDWR);

	ctx->requested_granularity = granularity;

	int is_eadr = atoi(os_getenv("IS_EADR"));
	int is_pmem = atoi(os_getenv("IS_PMEM"));
	if (is_eadr) {
		if (is_pmem)
			ctx->expected_granularity = PMEM2_GRANULARITY_BYTE;
		else
			UT_FATAL("invalid configuration IS_EADR && !IS_PMEM");
	} else if (is_pmem) {
		ctx->expected_granularity = PMEM2_GRANULARITY_CACHE_LINE;
	} else {
		ctx->expected_granularity = PMEM2_GRANULARITY_PAGE;
	}
}

/*
 * init_cfg -- initialize basic pmem2 config
 */
static void
init_cfg(struct pmem2_config *cfg, struct test_ctx *ctx)
{
	pmem2_config_init(cfg);
#ifdef _WIN32
	cfg->handle = (HANDLE)_get_osfhandle(ctx->fd);
#else
	cfg->fd = ctx->fd;
#endif
}

/*
 * cleanup -- cleanup the environment after test
 */
static void
cleanup(struct pmem2_config *cfg, struct test_ctx *ctx)
{
#ifdef _WIN32
	CloseHandle(cfg->handle);
#else
	CLOSE(ctx->fd);
#endif
}

/*
 * map_with_available_granularity -- map the range with valid granularity,
 * includes cleanup
 */
static void
map_with_available_granularity(struct pmem2_config *cfg, struct test_ctx *ctx)
{
	cfg->requested_max_granularity = ctx->requested_granularity;

	struct pmem2_map *map;
	int ret = pmem2_map(cfg, &map);
	UT_PMEM2_EXPECT_RETURN(ret, 0);
	UT_ASSERTne(map, NULL);
	UT_ASSERTeq(ctx->expected_granularity,
				pmem2_map_get_store_granularity(map));

	/* cleanup after the test */
	pmem2_unmap(&map);
}

/*
 * map_with_unavailable_granularity -- map the range with invalid
 * granularity (unsuccessful)
 */
static void
map_with_unavailable_granularity(struct pmem2_config *cfg,
	struct test_ctx *ctx)
{
	cfg->requested_max_granularity = ctx->requested_granularity;

	struct pmem2_map *map;
	int ret = pmem2_map(cfg, &map);
	UT_PMEM2_EXPECT_RETURN(ret, PMEM2_E_GRANULARITY_NOT_SUPPORTED);
	UT_ERR("%s", pmem2_errormsg());
	UT_ASSERTeq(map, NULL);
}

typedef void(*map_func)(struct pmem2_config *cfg, struct test_ctx *ctx);

/*
 * granularity_template -- template for testing granularity in pmem2
 */
static void
granularity_template(int argc, char *argv[], map_func map_do,
	enum pmem2_granularity granularity)
{
	struct test_ctx ctx = { 0 };
	init_test(argc, argv, &ctx, granularity);

	struct pmem2_config cfg;
	init_cfg(&cfg, &ctx);

	map_do(&cfg, &ctx);

	cleanup(&cfg, &ctx);
}

/*
 * test_granularity_req_byte_avail_byte -- require byte granularity,
 * when byte granularity is available
 */
static int
test_granularity_req_byte_avail_byte(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_available_granularity,
					PMEM2_GRANULARITY_BYTE);
	return 2;
}

/*
 * test_granularity_req_byte_avail_cl -- require byte granularity,
 * when cache line granularity is available
 */
static int
test_granularity_req_byte_avail_cl(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_unavailable_granularity,
					PMEM2_GRANULARITY_BYTE);
	return 2;
}

/*
 * test_granularity_req_byte_avail_page -- require byte granularity,
 * when page granularity is available
 */
static int
test_granularity_req_byte_avail_page(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_unavailable_granularity,
					PMEM2_GRANULARITY_BYTE);
	return 2;
}

/*
 * test_granularity_req_cl_avail_byte -- require cache line granularity,
 * when byte granularity is available
 */
static int
test_granularity_req_cl_avail_byte(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_available_granularity,
					PMEM2_GRANULARITY_CACHE_LINE);
	return 2;
}

/*
 * test_granularity_req_cl_avail_cl -- require cache line granularity,
 * when cache line granularity is available
 */
static int
test_granularity_req_cl_avail_cl(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_available_granularity,
					PMEM2_GRANULARITY_CACHE_LINE);
	return 2;
}

/*
 * test_granularity_req_cl_avail_page -- require cache line granularity,
 * when page granularity is available
 */
static int
test_granularity_req_cl_avail_page(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_unavailable_granularity,
					PMEM2_GRANULARITY_CACHE_LINE);
	return 2;
}

/*
 * test_granularity_req_page_avail_byte -- require page granularity,
 * when byte granularity is available
 */
static int
test_granularity_req_page_avail_byte(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_available_granularity,
					PMEM2_GRANULARITY_PAGE);
	return 2;
}

/*
 * test_granularity_req_byte_avail_cl -- require page granularity,
 * when byte cache line is available
 */
static int
test_granularity_req_page_avail_cl(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_available_granularity,
					PMEM2_GRANULARITY_PAGE);
	return 2;
}

/*
 * test_granularity_req_page_avail_page -- require page granularity,
 * when page granularity is available
 */
static int
test_granularity_req_page_avail_page(const struct test_case *tc, int argc,
					char *argv[])
{
	granularity_template(argc, argv, map_with_available_granularity,
					PMEM2_GRANULARITY_PAGE);
	return 2;
}

/*
 * test_cases -- available test cases
 */
static struct test_case test_cases[] = {
	TEST_CASE(test_granularity_req_byte_avail_byte),
	TEST_CASE(test_granularity_req_byte_avail_cl),
	TEST_CASE(test_granularity_req_byte_avail_page),
	TEST_CASE(test_granularity_req_cl_avail_byte),
	TEST_CASE(test_granularity_req_cl_avail_cl),
	TEST_CASE(test_granularity_req_cl_avail_page),
	TEST_CASE(test_granularity_req_page_avail_byte),
	TEST_CASE(test_granularity_req_page_avail_cl),
	TEST_CASE(test_granularity_req_page_avail_page),
};

#define NTESTS ARRAY_SIZE(test_cases)

int
main(int argc, char *argv[])
{
	pmem2_map_init();
	START(argc, argv, "pmem2_granularity");
	TEST_CASE_PROCESS(argc, argv, test_cases, NTESTS);
	pmem2_map_fini();
	DONE(NULL);
}
