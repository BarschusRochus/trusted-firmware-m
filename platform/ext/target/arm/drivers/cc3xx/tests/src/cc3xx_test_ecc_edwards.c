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
#include "cc3xx_ec_edw_extended_point.h"

#include "cc3xx_test_utils.h"
#include <stdint.h>
#include <string_utils.h>

void print__debug(uint32_t *debug, size_t len){
    for(size_t i=0; i < len; i++){
        printf("%08lx ", debug[i]);
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

typedef struct {
    char *label;
    uint32_t x_affine[8]; 
    uint32_t y_affine[8]; 
    uint32_t x_extended[8];
    uint32_t y_extended[8];
    uint32_t z_extended[8];
    uint32_t t_extended[8];
}cc3xx_ec_edw_ext_point_test_data_t;

typedef struct {
    char *label;
    uint32_t p_x[8]; 
    uint32_t p_y[8]; 
    uint32_t q_x[8]; 
    uint32_t q_y[8]; 
    uint32_t res_x[8]; 
    uint32_t res_y[8]; 
}cc3xx_ec_edw_addition_test_t;

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

cc3xx_ec_edw_ext_point_test_data_t generator_to_extendend = {
    .label = "Generator point",
    .x_affine = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .y_affine = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .x_extended = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .y_extended = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .z_extended = {0x1},
    //0x67875f0f d78b7665 66ea4e8e 64abe37d 20f09f80 775152f5 6dde8ab3 a5b7dda3
    .t_extended = {0xa5b7dda3, 0x6dde8ab3, 0x775152f5, 0x20f09f80, 0x64abe37d, 0x66ea4e8e, 0xd78b7665, 0x67875f0f}
};

cc3xx_ec_edw_addition_test_t generator_plus_generator = {
    .label = "Generator + Generator\n",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .q_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .q_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    //0x36ab384c 9f5a046c 3d043b7d 1833e7ac 080d8e45 15d7a45f 83c5a14e 2843ce0e
    .res_x = {0x2843ce0e, 0x83c5a14e, 0x15d7a45f, 0x080d8e45, 0x1833e7ac, 0x3d043b7d, 0x9f5a046c, 0x36ab384c},
    //0x2260cdf3 092329c2 1da25ee8 c9a21f56 97390f51 64385156 0e5f46ae 6af8a3c9
    .res_y = {0x6af8a3c9, 0x0e5f46ae, 0x64385156, 0x97390f51, 0xc9a21f56, 0x1da25ee8, 0x092329c2, 0x2260cdf3}
};

cc3xx_ec_edw_addition_test_t twenty_G_plus_fifty_G = {
    .label = "20*G + 50*G\n",
    //0x673c65ca edd698b9 4f5bbd75 7df73a9e 6985150e cd4a2135 a058e273 ab4cf9af - 20Gx
    //0x136cebac b6260a9d 5e6a3e31 71c535f0 be71cfbe 16a960b9 dd317bda 6f3c5a38 - 20Gy
    //0x1687cbf8 4fd6eff0 8833b22b 598bd634 e5e2b2e8 ac548450 6bb9a105 7f2f259e - 50Gx
    //0x14a6e98e 85a577bc 8299245a 489b96fe fb1197f1 edb5ab95 49d283a4 e209bfb1 - 50Gy
    //0x3baf6ffd 495a94ab debaaeb9 d4853792 be9c8a5b 622b3f77 d6e18801 391d723e - 70Gx
    //0x16a37175 2a302c2d b274caad 6e59afb7 b0675344 1663e48b 772606d0 c485b2e8 - 70Gy
    .p_x    = {0xab4cf9af, 0xa058e273, 0xcd4a2135, 0x6985150e, 0x7df73a9e, 0x4f5bbd75, 0xedd698b9, 0x673c65ca},
    .p_y    = {0x6f3c5a38, 0xdd317bda, 0x16a960b9, 0xbe71cfbe, 0x71c535f0, 0x5e6a3e31, 0xb6260a9d, 0x136cebac},
    .q_x    = {0x7f2f259e, 0x6bb9a105, 0xac548450, 0xe5e2b2e8, 0x598bd634, 0x8833b22b, 0x4fd6eff0, 0x1687cbf8},
    .q_y    = {0xe209bfb1, 0x49d283a4, 0xedb5ab95, 0xfb1197f1, 0x489b96fe, 0x8299245a, 0x85a577bc, 0x14a6e98e},
    .res_x  = {0x391d723e, 0xd6e18801, 0x622b3f77, 0xbe9c8a5b, 0xd4853792, 0xdebaaeb9, 0x495a94ab, 0x3baf6ffd},
    .res_y  = {0xc485b2e8, 0x772606d0, 0x1663e48b, 0xb0675344, 0x6e59afb7, 0xb274caad, 0x2a302c2d, 0x16a37175}
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

int cc3xx_test_ecc_edw_extended_point(cc3xx_ec_edw_ext_point_test_data_t *data){

    int rc = 0;
     
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine point = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(point.x, data->x_affine, 32);
    cc3xx_lowlevel_pka_write_reg(point.y, data->y_affine, 32);

    bool on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&point, &curve);
    assert(on_curve);

    cc3xx_ec_point_extended pt_extended = cc3xx_lowlevel_ec_allocate_extended_point();

    cc3xx_lowlevel_ec_affine_to_extended(&curve, &point, &pt_extended);
    
    uint32_t tmp[8];
    cc3xx_lowlevel_pka_read_reg(pt_extended.x,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(pt_extended.y,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(pt_extended.z,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(pt_extended.t,tmp, 32);
    print__debug(tmp, 8);
    print__debug(data->t_extended, 8);

    cc3xx_ec_point_affine back_to_affine = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_lowlevel_ec_extended_to_affine(&curve, &pt_extended, &back_to_affine);
    on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&back_to_affine, &curve);
    assert(on_curve);

    cc3xx_lowlevel_pka_read_reg(back_to_affine.x,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(back_to_affine.y,tmp, 32);
    print__debug(tmp, 8);


cleanup:
    cc3xx_lowlevel_ec_free_point(&back_to_affine);    
    cc3xx_lowlevel_ec_free_extended_point(&pt_extended);
    cc3xx_lowlevel_ec_free_point(&point);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_ecc_edw_addition(cc3xx_ec_edw_addition_test_t *data){

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
     
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine p = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(p.x, data->p_x, 32);
    cc3xx_lowlevel_pka_write_reg(p.y, data->p_y, 32);

    cc3xx_ec_point_affine q = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(q.x, data->q_x, 32);
    cc3xx_lowlevel_pka_write_reg(q.y, data->q_y, 32);

    bool on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&p, &curve);
    assert(on_curve);

    on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&q, &curve);
    assert(on_curve);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_lowlevel_ec_edwards_add_points(&curve, &p, &q, &res);

    on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&res, &curve);
    assert(on_curve);
    
    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    assert(memcmp(tmp, data->res_x, 32) == 0);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    assert(memcmp(tmp, data->res_y, 32) == 0);

    /*
    printf("calculated results:\n");
    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    print__debug(tmp, 8);
    
    printf("expected results:\n");
    print__debug(data->res_x, 8);
    print__debug(data->res_y, 8);
    */

cleanup:
    cc3xx_lowlevel_ec_free_point(&res);
    cc3xx_lowlevel_ec_free_point(&q);
    cc3xx_lowlevel_ec_free_point(&p);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}


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

    printf("EXTENDEND POINT TESTS\n\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_extended_point(&generator_to_extendend) == 0, "Point decompression did not succeed");
    printf("EXTENDEND POINT TESTS PASSED\n\n");

    printf("POINT ADDITION TESTS\n\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&generator_plus_generator) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&twenty_G_plus_fifty_G) == 0, "Point decompression did not succeed");
    printf("POINT ADDITION PASSED\n\n");

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
