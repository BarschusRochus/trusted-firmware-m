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

void debug_print_virt_reg_map(void){
    uint32_t BASE_ADDRESS = 0x5002B000;
    uint32_t offset = 0x0;
    uint32_t *reg = (uint32_t*) BASE_ADDRESS + offset;
    while(offset < 0x80){
        reg = (uint32_t*) ((BASE_ADDRESS + offset));
        printf("%08lx ", *reg);
        offset += 0x4;
        if(offset % 8 == 0){
            printf("\n");
        }
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

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_generator =  {
    //generator point as simple test
    //when interpreted as little endian the hex number startis with 0x6 -> 0b0110 -> most significant bit is 0 -> matches lsb of expected x (0xA -> 0b10)
    .label = "Generator Point",
    .y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .expected_odd_x = 0b00000000,
    .expected_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .expected_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666}
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_one =  {
    //0 * G
    //x = 0 = 0x0
    //y = 1 = 0x1
    .label = "Generator * 0",
    .y = {0x1},
    .expected_odd_x = 0b00000000,
    .expected_x = {0x0},
    .expected_y = {0x1}
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_even_x =  {
    //4 * G
    .label = "4 * G (even x)",
    .y = {0xca32112f, 0xdf38ab61, 0xea2f0ff0, 0x4cf22832, 0x80d5716c, 0x470eb885, 0xcb1595e1, 0x47d0e827},
    .expected_odd_x = 0b00000000,
    .expected_x = {0xc4c9f870, 0x493aa657, 0x93ce1547, 0x1a739ec1, 0x7a3520f9, 0x8325d4b8, 0x56cff146, 0x203da8db}, //0x203da8db 56cff146 8325d4b8 7a3520f9 1a739ec1 93ce1547 493aa657 c4c9f870,
    .expected_y = {0xca32112f, 0xdf38ab61, 0xea2f0ff0, 0x4cf22832, 0x80d5716c, 0x470eb885, 0xcb1595e1, 0x47d0e827}  //0x47d0e827 cb1595e1 470eb885 80d5716c 4cf22832 ea2f0ff0 df38ab61 ca32112f
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_odd_x =  {
    //5 * G
    .label = "5 * G (odd x)",
    .y = {0xd676c8ed, 0x10d21f83, 0x89430b5d, 0x31282eca, 0x89924666, 0xe02c6e14, 0x98feae6f, 0xdf4825b2}, //first bit to 1 -> 0xdf4825b298feae6fe02c6e148992466631282eca89430b5d10d21f83d676c8ed
    .expected_odd_x = 0b00000001,
    .expected_x = {0x322ef233, 0x91409cc0, 0x3e1be1a5, 0x5c2819f9, 0xd12da5de, 0xfcef7cf7, 0xade3587b, 0x49fda73e}, //x = 0x49fda73e ade3587b fcef7cf7 d12da5de 5c2819f9 3e1be1a5 91409cc0 322ef233
    .expected_y = {0xd676c8ed, 0x10d21f83, 0x89430b5d, 0x31282eca, 0x89924666, 0xe02c6e14, 0x98feae6f, 0x5f4825b2} //y = 0x5f4825b2 98feae6f e02c6e14 89924666 31282eca 89430b5d 10d21f83 d676c8ed
};

cc3xx_ec_edw_point_decompress_test_data_t point_decompress_r_pub_from_taler =  {
    //uint8_t r_pub_0[32] = { 0x91, 0x7a, 0xf5, 0xcd, 0xf6, 0x4e, 0x8c, 0xba, 0x00, 0x51, 0x37, 0xf6, 0x6f, 0x14, 0xc8, 0xb5, 0xc5, 0x31, 0xa5, 0x14, 0x68, 0xc6, 0x90, 0x0c, 0xcf, 0x11, 0x7b, 0xac, 0xd8, 0x2a, 0x3e, 0x87};
    // lil endian - 0x917af5cdf64e8cba005137f66f14c8b5c531a51468c6900ccf117bacd82a3e87  <-- input I get from rest of the application
    // big endian - 0x873e2ad8ac7b11cf0c90c66814a531c5b5c8146ff6375100ba8c4ef6cdf57a91  <-- I reverse this one wordwise as input, 0x87 turns into 0x07 when 1 is set to 0
    //x lil endian c757a49e 7adf3c65 178d9b45 98aadfd9 dc7a2443 133082a9 91b59d94 b5982863 - big endian: 0x632898b5, 0x949db591, 0xa9823013, 0x43247adc, 0xd9dfaa98, 0x459b8d17, 0x653cdf7a, 0x9ea457c7
    //y lil endian 917af5cd f64e8cba 005137f6 6f14c8b5 c531a514 68c6900c cf117bac d82a3e07 - big endian: 0x073e2ad8, 0xac7b11cf, 0x0c90c668, 0x14a531c5, 0xb5c8146f, 0xf6375100, 0xba8c4ef6, 0xcdf57a91
    .label = "taler R pub from test cases",
    .y = {0xcdf57a91, 0xba8c4ef6, 0xf6375100, 0xb5c8146f, 0x14a531c5, 0x0c90c668, 0xac7b11cf, 0x873e2ad8}, //first bit to 1 -> 0xdf4825b298feae6fe02c6e148992466631282eca89430b5d10d21f83d676c8ed
    .expected_odd_x = 0b00000001,
    .expected_x = { 0x9ea457c7, 0x653cdf7a, 0x459b8d17, 0xd9dfaa98, 0x43247adc, 0xa9823013, 0x949db591, 0x632898b5},
    .expected_y = { 0xcdf57a91, 0xba8c4ef6, 0xf6375100, 0xb5c8146f, 0x14a531c5, 0x0c90c668, 0xac7b11cf, 0x073e2ad8}
};


/*further decompress tests:
- point that can't be decoded
- point with wrong X coordinate set */

int cc3xx_test_ecc_edw_decompress_point(cc3xx_ec_edw_point_decompress_test_data_t *data){
    int rc = 0;
    printf("%s \n",data->label);

    //enable cryptocell and curve
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();
    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    //TODO: Could be moved inside function and only uint32_t values could be returned
    //allocate registers for decompressed point
    cc3xx_ec_point_affine decompressed = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_ec_edw_decompress_point(data->y,&curve, &decompressed);

    uint32_t decompressed_y[8];
    uint32_t decompressed_x[8];
    cc3xx_lowlevel_pka_read_reg(decompressed.x, decompressed_x, 32);
    cc3xx_lowlevel_pka_read_reg(decompressed.y, decompressed_y, 32);
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

/*
int cc3xx_test_reg_bit_manipulation(void){

    int rc = 0;
    uint32_t x[8] = {0x0};
    uint32_t y[8] = {0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x11111111};
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    printf("right after ec init:\n");
    debug_print_virt_reg_map();

    cc3xx_ec_point_affine point = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(point.x, x, 32);
    cc3xx_lowlevel_pka_write_reg(point.y, y, 32);
    debug_read_and_print_reg(point.x, "x: ");
    debug_read_and_print_reg(point.y, "y: ");

    printf("before bit analysis:\n");
    debug_print_virt_reg_map();

    uint32_t bits = cc3xx_lowlevel_pka_test_bits_ui(point.x, 0, 4);
    printf("read bits from reg x: %08lx\n", bits);
    bits = cc3xx_lowlevel_pka_test_bits_ui(point.y, 0, 4);
    printf("read bits from reg y: %08lx\n", bits);
    
    //flip both bits
    cc3xx_lowlevel_pka_flip_bit(point.x, 0, point.x);
    cc3xx_lowlevel_pka_flip_bit(point.x, 0, point.y);
    bits = cc3xx_lowlevel_pka_test_bits_ui(point.x, 0, 4);
    printf("read bits from reg x: %08lx\n", bits);
    bits = cc3xx_lowlevel_pka_test_bits_ui(point.y, 0, 4);
    printf("read bits from reg y: %08lx\n", bits);

    printf("after:\n");
    debug_print_virt_reg_map();

cleanup:
    cc3xx_lowlevel_ec_free_point(&point);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}
*/
static void ecc_edwards_tests_run(struct test_result_t *ret)
{

    printf("POINT DECOMPRESSION TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_generator) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_r_pub_from_taler) == 0, "Point decompression did not succeed");
    printf("POINT DECOMPRESSION TESTS PASSED\n\n");

    printf("POINT ON CURVE TESTS STARTING\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_generator) == 0, "Point decompression did not succeed");
    printf("POINT ON CURVE TESTS PASSED\n\n");

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
