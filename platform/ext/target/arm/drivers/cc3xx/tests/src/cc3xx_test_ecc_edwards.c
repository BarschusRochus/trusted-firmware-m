/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "cc3xx_test_ecc_edwards.h"
#include "cc3xx_ec.h"
#include "cc3xx_ec_edwards.h"
#include "cc3xx_pka.h"
#include "cc3xx_test_assert.h"
#include "cc3xx_init.h"

#include "cc3xx_test_utils.h"
#include <stdint.h>
#include <string_utils.h>

void print__debug(uint32_t *debug, size_t len){
    for(size_t i=0; i < len; i++){
        printf("%08xl ", debug[i]);
    }
    printf("\n");
}

typedef struct {
    char *label;
    uint32_t y[8]; //compressed point as y coordinate with first bit set to lsb of x
    uint8_t expected_odd_x;
    uint32_t expected_x[8];
    uint32_t expected_y[8];
}cc3xx_ec_edw_point_decompress_test_data_t;

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_test_data =  {
    //generator point as simple test
    //when interpreted as little endian the hex number startis with 0x6 -> 0b0110 -> most significant bit is 0 -> matches lsb of expected x (0xA -> 0b10)
    .label = "Generator Point\n",
    .y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .expected_odd_x = 0b00000000,
    .expected_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .expected_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666}
    //0x6666666666666666666666666666666666666666666666666666666666666658
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_one =  {
    //0 * G
    //x = 0 = 0x0
    //y = 1 = 0x1
    .label = "Generator * 0\n",
    .y = {0x1},
    .expected_odd_x = 0b00000000,
    .expected_x = {0x0},
    .expected_y = {0x1}
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_even_x =  {
    //4 * G
    .label = "4 * G (even x)\n",
    .y = {0xca32112f, 0xdf38ab61, 0xea2f0ff0, 0x4cf22832, 0x80d5716c, 0x470eb885, 0xcb1595e1, 0x47d0e827},
    .expected_odd_x = 0b00000000,
    .expected_x = {0xc4c9f870, 0x493aa657, 0x93ce1547, 0x1a739ec1, 0x7a3520f9, 0x8325d4b8, 0x56cff146, 0x203da8db}, //0x203da8db 56cff146 8325d4b8 7a3520f9 1a739ec1 93ce1547 493aa657 c4c9f870,
    .expected_y = {0xca32112f, 0xdf38ab61, 0xea2f0ff0, 0x4cf22832, 0x80d5716c, 0x470eb885, 0xcb1595e1, 0x47d0e827}  //0x47d0e827 cb1595e1 470eb885 80d5716c 4cf22832 ea2f0ff0 df38ab61 ca32112f
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_odd_x =  {
    //5 * G
    .label = "5 * G (odd x)\n",
    .y = {0xd676c8ed, 0x10d21f83, 0x89430b5d, 0x31282eca, 0x89924666, 0xe02c6e14, 0x98feae6f, 0xdf4825b2}, //first bit to 1 -> 0xdf4825b298feae6fe02c6e148992466631282eca89430b5d10d21f83d676c8ed
    .expected_odd_x = 0b00000001,
    .expected_x = {0x322ef233, 0x91409cc0, 0x3e1be1a5, 0x5c2819f9, 0xd12da5de, 0xfcef7cf7, 0xade3587b, 0x49fda73e}, //x = 0x49fda73e ade3587b fcef7cf7 d12da5de 5c2819f9 3e1be1a5 91409cc0 322ef233
    .expected_y = {0xd676c8ed, 0x10d21f83, 0x89430b5d, 0x31282eca, 0x89924666, 0xe02c6e14, 0x98feae6f, 0x5f4825b2} //y = 0x5f4825b2 98feae6f e02c6e14 89924666 31282eca 89430b5d 10d21f83 d676c8ed
};

int cc3xx_test_ecc_edw_decompress_point(cc3xx_ec_edw_point_decompress_test_data_t *data){
    uint8_t odd_x = 0x00;
    int rc = 0;
    puts(data->label);
    //all representations are little endian -> first bit of last uint32 should be msb and thus lsb of x
    odd_x |= data->y[7] >> 31;
    cc3xx_test_assert(odd_x == data->expected_odd_x);

    //unset first bit in y coordinate as this can never be set for calculation
    //TODO the implementation needs to do that somewhere as this will lead to problems if not unset
    data->y[7] &= 0b01111111111111111111111111111111;

    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine decompressed = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_pka_reg_id_t reg_y = cc3xx_lowlevel_pka_allocate_reg();
    
    cc3xx_lowlevel_pka_write_reg(reg_y, data->y, 32);

    cc3xx_lowlevel_ec_edw_decompress_point(reg_y, odd_x, &decompressed, &curve);
    printf("returned\n");

    uint32_t decompressed_y[8];
    uint32_t decompressed_x[8];
    cc3xx_lowlevel_pka_read_reg(decompressed.x, decompressed_x, 32);
    cc3xx_lowlevel_pka_read_reg(decompressed.y, decompressed_y, 32);

    printf("\n");
    printf("Decompressed X:\n");
    for(size_t i=0; i < 8; i++){
        printf("%08x ", decompressed_x[i]);
    }
    
    printf("\n");

    printf("Expected X:\n");
    for(size_t i=0; i < 8; i++){
        printf("%08x ", data->expected_x[i]);
    }
    printf("\n");

    assert(memcmp(decompressed_x, data->expected_x, 32) == 0);

cleanup:
    cc3xx_lowlevel_pka_free_reg(reg_y);
    cc3xx_lowlevel_ec_free_point(&decompressed);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}


int cc3xx_test_ecc_edw_point_on_curve(cc3xx_ec_edw_point_decompress_test_data_t *data){

    int rc = 0;
    bool on_curve = false;
    
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine point = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(point.x, data->expected_x, 32);
    cc3xx_lowlevel_pka_write_reg(point.y, data->expected_y, 32);


    on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&point, &curve);
    assert(on_curve);


cleanup:
    cc3xx_lowlevel_ec_free_point(&point);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

static void ecc_edwards_tests_run(struct test_result_t *ret)
{

    printf("POINT DECOMPRESSION TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_test_data) == 0, "Point decompression did not succeed");
    printf("POINT DECOMPRESSION TESTS PASSED\n");

    printf("POINT ON CURVE TESTS STARTING\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_test_data) == 0, "Point decompression did not succeed");
    printf("POINT ON CURVE TESTS PASSED\n");
    //TEST_ASSERT(cc3xx_test_one_times_one() == 0, "Point decompression did not succeed");

    ret->val = TEST_PASSED;
    return;
}
static struct test_t ecc_edw_tests = {
    &ecc_edwards_tests_run,
    "CC3XX_ECC_EDW_TEST",
    "CC3XX ECC Edwards tests",
};

void add_cc3xx_ecc_edwards_tests_to_testsuite(struct test_suite_t *p_ts, uint32_t ts_size)
{
    cc3xx_add_tests_to_testsuite(&ecc_edw_tests, 1, p_ts, ts_size);
}
