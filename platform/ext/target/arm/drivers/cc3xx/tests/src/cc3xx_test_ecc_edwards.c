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

void print__debug(uint32_t *debug, size_t len){
    for(size_t i=0; i < len; i++){
        printf("%08xl ", debug[i]);
    }
    printf("\n");
}

typedef struct {
    uint32_t y[8];
    uint8_t expected_odd_x;
    uint32_t expected_x[8];
    uint32_t expected_y[8];
}cc3xx_ec_edw_point_decompress_test_data_t;

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_test_data =  {
    //generator point as simple test
    //when interpreted as little endian the hex number startis with 0x6 -> 0b0110 -> most significant bit is 0 -> matches lsb of expected x (0xA -> 0b10)
    .y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .expected_odd_x = 0b00000000,
    .expected_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .expected_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666}
    //0x6666666666666666666666666666666666666666666666666666666666666658
};

int cc3xx_test_ecc_edw_decompress_point(cc3xx_ec_edw_point_decompress_test_data_t *data){
    uint8_t odd_x = 0x00;
    int rc = 0;
    //all representations are little endian -> first bit of last uint32 should be msb
    odd_x |= data->y[7] >> 31;
    cc3xx_test_assert(odd_x == data->expected_odd_x);

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
    printf("\n");
    printf("\n");
    for(size_t i=0; i < 8; i++){
        printf("%08x ", decompressed_x[i]);
    }
    printf("\n");
    for(size_t i=0; i < 8; i++){
        printf("%08x ", decompressed_y[i]);
    }

cleanup:
    cc3xx_lowlevel_pka_free_reg(reg_y);
    cc3xx_lowlevel_ec_free_point(&decompressed);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

/*
int cc3xx_test_mult_N(void){

    int rc = 0;
    uint32_t y[8] = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666};
    uint32_t y2[16] = {0xd70a3e40,0x0a3d70a3,0x3d70a3d7,0x70a3d70a,0xa3d70a3d,0xd70a3d70,0x0a3d70a3,0x3d70a3d7, 0x8f5c28ea, 0x5c28f5c2, 0x28f5c28f, 0xf5c28f5c, 0xc28f5c28, 0x8f5c28f5, 0x5c28f5c2, 0x28f5c28f};
    uint32_t res[16] = {0x0};
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();
    

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_pka_reg_id_t t = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t reg_y = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(reg_y, y, 32);

    cc3xx_lowlevel_pka_set_modulus(curve.field_modulus, false, CC3XX_PKA_REG_NP);
    
    cc3xx_lowlevel_pka_mod_mul(reg_y, reg_y, t);
    cc3xx_lowlevel_pka_read_reg(t, res, 64);
    print__debug(res, 64);
    print__debug(y2, 64);


cleanup:
    cc3xx_lowlevel_pka_free_reg(reg_y);    
    cc3xx_lowlevel_pka_free_reg(t);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_one_times_one(void){

    int rc = 0;
    uint32_t one[8] = {0x1};
    uint32_t res[8] = {0x0};
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();
    

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_pka_reg_id_t t = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t reg_one = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(reg_one, one, 32);

    cc3xx_lowlevel_pka_set_modulus(curve.order, true, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_read_reg(CC3XX_PKA_REG_NP, res, 32);
    print__debug(res, 8);
    
    cc3xx_lowlevel_pka_set_modulus(curve.field_modulus, true, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_read_reg(CC3XX_PKA_REG_NP, res, 32);
    print__debug(res, 8);
    
    cc3xx_lowlevel_pka_mod_mul(reg_one, reg_one, t);
    cc3xx_lowlevel_pka_read_reg(t, res, 32);
    print__debug(res, 8);


cleanup:
    cc3xx_lowlevel_pka_free_reg(reg_one);    
    cc3xx_lowlevel_pka_free_reg(t);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_modmul_five_times_y(void){
    return 1;
}

*/
static void ecc_edwards_tests_run(struct test_result_t *ret)
{

    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_test_data) == 0, "Point decompression did not succeed");
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
