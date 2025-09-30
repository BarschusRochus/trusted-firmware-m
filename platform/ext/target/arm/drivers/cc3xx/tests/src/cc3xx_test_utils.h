/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __CC3XX_TEST_UTILS_H__
#define __CC3XX_TEST_UTILS_H__

#include <stdint.h>
#include <stddef.h>

#include "test_framework.h"
#include "tfm_hal_device_header.h"

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        TEST_FAIL(msg); \
        return; \
    }

#define TEST_SETUP(x) TEST_ASSERT((x) == 0, "Test setup failed")
#define TEST_TEARDOWN(x) TEST_ASSERT((x) == 0, "Test teardown failed")

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

#ifdef __cplusplus
extern "C" {
#endif

void cc3xx_add_tests_to_testsuite(struct test_t *test_list, uint32_t test_am,
                                  struct test_suite_t *p_ts, uint32_t ts_size);

static inline uint32_t pmod(int32_t num, int32_t N)
{
    return (((num % N) + N) % N);
}

static inline void enable_cycle_counter(void)
{
    SysTick->VAL  = 0;
    SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

static inline uint32_t get_cycle_count(void)
{
    return SysTick_LOAD_RELOAD_Msk - SysTick->VAL;
}

static inline uint32_t reset_cycle_count(void)
{
    SysTick->CTRL = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __CC3XX_TEST_UTILS_H__ */
