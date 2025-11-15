/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "cc3xx_test_ecc_edwards.h"
#include "cc3xx_ec.h"
#include "cc3xx_ec_edwards.h"
#include "cc3xx_error.h"
#include "cc3xx_pka.h"
#include "cc3xx_test_assert.h"
#include "cc3xx_init.h"
#include "cc3xx_ec_edw_extended_point.h"

#include "cc3xx_test_utils.h"
#include <stdint.h>
#include <string.h>
#include <string_utils.h>

void print__debug(uint32_t *debug, size_t len){
    for(size_t i=0; i < len; i++){
        printf("%08lx ", debug[i]);
    }
    printf("\n");
}

void init_edwards_25519(cc3xx_ec_curve_t *curve){
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, curve);
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

typedef struct {
    char *label;
    uint32_t p_x[8]; 
    uint32_t p_y[8]; 
    uint32_t res_x[8]; 
    uint32_t res_y[8]; 
}cc3xx_ec_edw_doubling_test_t;

typedef struct {
    char *label;
    uint32_t p_x[8]; 
    uint32_t p_y[8]; 
    uint32_t p_z[8];
    uint32_t p_t[8];
    uint32_t res_x[8]; 
    uint32_t res_y[8]; 
}cc3xx_ec_edw_doubling_test_ext_point_t;

typedef struct {
    char *label;
    uint32_t p_x[8]; 
    uint32_t p_y[8]; 
    uint32_t scalar[8];
    uint32_t res_x[8]; 
    uint32_t res_y[8]; 
}cc3xx_ec_edw_scalar_mult_test_t;

typedef struct {
    char *label;
    uint32_t p1_x[8]; 
    uint32_t p1_y[8]; 
    uint32_t p2_x[8]; 
    uint32_t p2_y[8]; 
    uint32_t scalar_a[8];
    uint32_t scalar_b[8];
    uint32_t res_x[8]; 
    uint32_t res_y[8];
}cc3xx_ec_edw_mult_and_add_test_t;

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
    .label = "Generator + Generator",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .q_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .q_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    //0x36ab384c 9f5a046c 3d043b7d 1833e7ac 080d8e45 15d7a45f 83c5a14e 2843ce0e
    .res_x = {0x2843ce0e, 0x83c5a14e, 0x15d7a45f, 0x080d8e45, 0x1833e7ac, 0x3d043b7d, 0x9f5a046c, 0x36ab384c},
    //0x2260cdf3 092329c2 1da25ee8 c9a21f56 97390f51 64385156 0e5f46ae 6af8a3c9
    .res_y = {0x6af8a3c9, 0x0e5f46ae, 0x64385156, 0x97390f51, 0xc9a21f56, 0x1da25ee8, 0x092329c2, 0x2260cdf3}
};

cc3xx_ec_edw_addition_test_t G2_plus_G2 = {
    .label = "2G + 2G",
    .p_x = {0x2843ce0e, 0x83c5a14e, 0x15d7a45f, 0x080d8e45, 0x1833e7ac, 0x3d043b7d, 0x9f5a046c, 0x36ab384c},
    .p_y = {0x6af8a3c9, 0x0e5f46ae, 0x64385156, 0x97390f51, 0xc9a21f56, 0x1da25ee8, 0x092329c2, 0x2260cdf3},
    .q_x = {0x2843ce0e, 0x83c5a14e, 0x15d7a45f, 0x080d8e45, 0x1833e7ac, 0x3d043b7d, 0x9f5a046c, 0x36ab384c},
    .q_y = {0x6af8a3c9, 0x0e5f46ae, 0x64385156, 0x97390f51, 0xc9a21f56, 0x1da25ee8, 0x092329c2, 0x2260cdf3},
    //0x36ab384c 9f5a046c 3d043b7d 1833e7ac 080d8e45 15d7a45f 83c5a14e 2843ce0e
    .res_x = {0xc4c9f870,0x493aa657,0x93ce1547,0x1a739ec1,0x7a3520f9,0x8325d4b8,0x56cff146,0x203da8db},
    //0x2260cdf3 092329c2 1da25ee8 c9a21f56 97390f51 64385156 0e5f46ae 6af8a3c9
    .res_y = {0xca32112f,0xdf38ab61,0xea2f0ff0,0x4cf22832,0x80d5716c,0x470eb885,0xcb1595e1,0x47d0e827}
};

cc3xx_ec_edw_addition_test_t twenty_G_plus_fifty_G = {
    .label = "20*G + 50*G",
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

cc3xx_ec_edw_addition_test_t l_minus_one_times_G_plus_G = {
    .label = "(group order -1)*G + G",
    //l-1 * G
    //X: 0x5e96c92c3291ac013f5b1dce022923a396d3389f6ada584d36a9d29f70da2ad3
    //Y: 0x6666666666666666666666666666666666666666666666666666666666666658
    .p_x    = {0x70da2ad3,0x36a9d29f,0x6ada584d,0x96d3389f,0x022923a3,0x3f5b1dce,0x3291ac01,0x5e96c92c},
    .p_y    = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    //G
    .q_x    = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .q_y    = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    //should yield point at infinity/zero/neutral element 0,1
    .res_x  = {0x0},
    .res_y  = {0x1}
};

cc3xx_ec_edw_addition_test_t l_minus_one_times_G_plus_2G = {
    .label = "(group order -1)*G + 2*G",
    //(group order -1)*G
    //X: 0x5e96c92c3291ac013f5b1dce022923a396d3389f6ada584d36a9d29f70da2ad3
    //Y: 0x6666666666666666666666666666666666666666666666666666666666666658
    .p_x    = {0x70da2ad3,0x36a9d29f,0x6ada584d,0x96d3389f,0x022923a3,0x3f5b1dce,0x3291ac01,0x5e96c92c},
    .p_y    = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    //2G
    //X: 0x36ab384c9f5a046c3d043b7d1833e7ac080d8e4515d7a45f83c5a14e2843ce0e
    //Y: 0x2260cdf3092329c21da25ee8c9a21f5697390f51643851560e5f46ae6af8a3c9
    .q_x    = {0x2843ce0e,0x83c5a14e,0x15d7a45f,0x080d8e45,0x1833e7ac,0x3d043b7d,0x9f5a046c,0x36ab384c},
    .q_y    = {0x6af8a3c9,0x0e5f46ae,0x64385156,0x97390f51,0xc9a21f56,0x1da25ee8,0x092329c2,0x2260cdf3},
    //should yield G
    .res_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .res_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
};

cc3xx_ec_edw_doubling_test_t double_generator = {
    .label = "Double the generator",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .res_x = {0x2843ce0e, 0x83c5a14e, 0x15d7a45f, 0x080d8e45, 0x1833e7ac, 0x3d043b7d, 0x9f5a046c, 0x36ab384c},
    .res_y = {0x6af8a3c9, 0x0e5f46ae, 0x64385156, 0x97390f51, 0xc9a21f56, 0x1da25ee8, 0x092329c2, 0x2260cdf3}
};


cc3xx_ec_edw_doubling_test_ext_point_t double_2G = {
.label = "2 * extended(2G)",
.p_x = {0x9f3fd67e, 0x1515e6cf, 0x163a7692, 0xde44f888, 0x213c1bd9, 0x5776d1e1, 0x960f6ad4, 0x3b6f8891},
.p_y = {0x874a007e, 0xb6d34c9a, 0x5107378d, 0xd6e15667, 0x14dab827, 0x5921f40f, 0x4cdb3092, 0x336d9ece},
.p_z = {0xf61c8608, 0x722d5b0b, 0x31b598ba, 0x50b27bff, 0x12f675b4, 0xfd9cb817, 0x52a20ea2, 0x59e4ea1a},
.p_t = {0x92cad989, 0xc532c7e3, 0x483d139b, 0x72749500, 0xdd5e07c1, 0xfc6ea6fe, 0x2d298daa, 0x1f6e08da},
.res_x = {0xc4c9f870,0x493aa657,0x93ce1547,0x1a739ec1,0x7a3520f9,0x8325d4b8,0x56cff146,0x203da8db},
.res_y = {0xca32112f,0xdf38ab61,0xea2f0ff0,0x4cf22832,0x80d5716c,0x470eb885,0xcb1595e1,0x47d0e827}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_1 = {
    .label ="G * 1",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x1},
    .res_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .res_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_70 = {
    .label ="G * 70",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x46},
    .res_x  = {0x391d723e, 0xd6e18801, 0x622b3f77, 0xbe9c8a5b, 0xd4853792, 0xdebaaeb9, 0x495a94ab, 0x3baf6ffd},
    .res_y  = {0xc485b2e8, 0x772606d0, 0x1663e48b, 0xb0675344, 0x6e59afb7, 0xb274caad, 0x2a302c2d, 0x16a37175}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_l_minus_one = {
    .label ="G * l-1",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x5cf5d3ec,0x5812631a,0xa2f79cd6,0x14def9de, 0x00000000,0x00000000,0x00000000,0x10000000},
    .res_x  = {0x70da2ad3,0x36a9d29f,0x6ada584d,0x96d3389f,0x022923a3,0x3f5b1dce,0x3291ac01,0x5e96c92c},
    .res_y  = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_l = {
    .label ="G * l",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x5cf5d3ed,0x5812631a,0xa2f79cd6,0x14def9de, 0x00000000,0x00000000,0x00000000,0x10000000},
    .res_x  = {0x0},
    .res_y  = {0x1}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_l_plus_one = { //that should be reduced to one
    .label ="G * l+1",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x5cf5d3ee,0x5812631a,0xa2f79cd6,0x14def9de, 0x00000000,0x00000000,0x00000000,0x10000000},
    .res_x  = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .res_y  = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_0x10000000 = {
    .label ="G * 0x10000000",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x10000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    .res_x  = {0xa1afbe2b,0xb10b2f6d,0x38e0e38f,0x1b410bcd,0xf307154a,0xe9b8dc6f,0x40a3b2de,0x1e45a601},
    .res_y  = {0x8dda0a76,0x893f072c,0x43ad047d,0x47d26e50,0x85e68acb,0xd2f3241a,0x73dffd60,0x0e730da4}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_0x1000 = {
    .label ="G * 0x1000",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x00001000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    .res_x  = {0x29ded6ea,0x02b9003a,0xc426cb59,0x97b199ba,0x92008e2f,0xeb524f26,0x8b891b47,0x7d13c024},
    .res_y  = {0x805b20d5,0x952080a6,0x8e9fe9c3,0x9e1e9e87,0x75ccc77a,0x91f1a56c,0x2c01a81a,0x59a976ab}
};

cc3xx_ec_edw_scalar_mult_test_t gen_times_0x10000 = {
    .label ="G * 0x10000",
    .p_x = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p_y = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar = {0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
    .res_x  = {0x8411a565,0x9e9d678a,0x7a6844d1,0xa39fe134,0xca07cd54,0x2f541f79,0xee4e7013,0x5de7faa2},
    .res_y  = {0xf854ec36,0xdf85e4ce,0x901d6ff6,0xc0e8bc08,0x6b432d92,0xab8ea992,0x1e1c2e0a,0x2c9f2364}
};

cc3xx_ec_edw_mult_and_add_test_t twenty_G_plus_fifty_G_ma = {
    .label = "20*G + 50*G",
    .p1_x    = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p1_y    = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .p2_x    = {0x8F25D51A,0xC9562D60,0x9525A7B2,0x692CC760,0xFDD6DC5C,0xC0A4E231,0xCD6E53FE,0x216936D3},
    .p2_y    = {0x66666658,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666,0x66666666},
    .scalar_a = {0x14},
    .scalar_b = {0x32},
    .res_x  = {0x391d723e, 0xd6e18801, 0x622b3f77, 0xbe9c8a5b, 0xd4853792, 0xdebaaeb9, 0x495a94ab, 0x3baf6ffd},
    .res_y  = {0xc485b2e8, 0x772606d0, 0x1663e48b, 0xb0675344, 0x6e59afb7, 0xb274caad, 0x2a302c2d, 0x16a37175}
};

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
    rc = cc3xx_lowlevel_ec_edw_decompress_point(data->y,&curve, &decompressed);

    assert(rc == CC3XX_ERR_SUCCESS);
    
    uint32_t decompressed_y[8];
    uint32_t decompressed_x[8];
    cc3xx_lowlevel_pka_read_reg(decompressed.x, decompressed_x, 32);
    cc3xx_lowlevel_pka_read_reg(decompressed.y, decompressed_y, 32);

    assert(memcmp(decompressed_x, data->expected_x, 32) == 0);
    assert(memcmp(decompressed_y, data->expected_y, 32) == 0);

cleanup:
    cc3xx_lowlevel_ec_free_point(&decompressed);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_ecc_edw_compress_point(cc3xx_ec_edw_point_decompress_test_data_t *data){
    int rc = 0;
    printf("%s \n",data->label);

    //enable cryptocell and curve
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();
    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine pt = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(pt.x, data->expected_x, 32);
    cc3xx_lowlevel_pka_write_reg(pt.y, data->expected_y, 32);

    uint32_t compressed_y[8];
    
    cc3xx_lowlevel_ec_edw_compress_point(&curve, &pt, compressed_y);

    assert(memcmp(compressed_y, data->y, 32) == 0);

cleanup:
    cc3xx_lowlevel_ec_free_point(&pt);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}


int cc3xx_test_ecc_edw_point_on_curve(cc3xx_ec_edw_point_decompress_test_data_t *data){

    int rc = 0;
    bool on_curve = false;
    printf("%s \n",data->label);
    
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
    printf("%s \n",data->label);
     
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
    
    /*
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
    */

    cc3xx_ec_point_affine back_to_affine = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_lowlevel_ec_extended_to_affine(&curve, &pt_extended, &back_to_affine);
    on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&back_to_affine, &curve);
    assert(on_curve);

    /*
    cc3xx_lowlevel_pka_read_reg(back_to_affine.x,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(back_to_affine.y,tmp, 32);
    print__debug(tmp, 8);
    */


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
    printf("%s\n", data->label);
     
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

int cc3xx_test_ecc_edw_doubling(cc3xx_ec_edw_doubling_test_t *data){

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("%s\n", data->label);
     
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine p = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(p.x, data->p_x, 32);
    cc3xx_lowlevel_pka_write_reg(p.y, data->p_y, 32);

    bool on_curve = cc3xx_lowlevel_ec_edw_is_point_on_curve(&p, &curve);
    assert(on_curve);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_lowlevel_ec_edwards_double_point(&curve, &p, &res);

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
    cc3xx_lowlevel_ec_free_point(&p);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_ecc_edw_doubling_extended_coord(
                        cc3xx_ec_edw_doubling_test_ext_point_t *data){

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("%s\n", data->label);
     
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_extended p = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_pka_write_reg(p.x, data->p_x, 32);
    cc3xx_lowlevel_pka_write_reg(p.y, data->p_y, 32);
    cc3xx_lowlevel_pka_write_reg(p.z, data->p_z, 32);
    cc3xx_lowlevel_pka_write_reg(p.t, data->p_t, 32);

    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();

    cc3xx_lowlevel_ec_edwards_double_extended_points(&curve, &p, &res_ext);

    cc3xx_ec_point_affine res= cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_ec_extended_to_affine(&curve, &res_ext, &res);
    
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
    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}


int cc3xx_test_ecc_edw_scalar_mult(
                        cc3xx_ec_edw_scalar_mult_test_t *data){

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("%s\n", data->label);
     
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine p = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(p.x, data->p_x, 32);
    cc3xx_lowlevel_pka_write_reg(p.y, data->p_y, 32);

    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, data->scalar, 32);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_lowlevel_ec_edwards_scalar_mult(&curve, &p, data->scalar, &res);

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


    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    assert(memcmp(tmp, data->res_x, 32) == 0);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    assert(memcmp(tmp, data->res_y, 32) == 0);
    

cleanup:
    cc3xx_lowlevel_ec_free_point(&res);
    cc3xx_lowlevel_pka_free_reg(s);
    cc3xx_lowlevel_ec_free_point(&p);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_ecc_edw_scalar_mult_generator(
                        cc3xx_ec_edw_scalar_mult_test_t *data)
{
    cc3xx_ec_curve_t curve = {};
    init_edwards_25519(&curve);

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("Scalar Mult Generator: ");
    printf("%s\n", data->label);
    
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, data->scalar, 32);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();

    cc3xx_lowlevel_ec_edwards_scalar_mult_generator(&curve, data->scalar, &res);

    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    assert(memcmp(tmp, data->res_x, 32) == 0);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    assert(memcmp(tmp, data->res_y, 32) == 0);
    

cleanup:
    cc3xx_lowlevel_ec_free_point(&res);
    cc3xx_lowlevel_pka_free_reg(s);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}



int cc3xx_test_ecc_edw_mult_and_add(
                        cc3xx_ec_edw_mult_and_add_test_t *data)
{
    cc3xx_ec_curve_t curve = {};
    init_edwards_25519(&curve);

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("Mult and Add: ");
    printf("%s\n", data->label);
    
    cc3xx_ec_point_affine pt1 = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(pt1.x, data->p1_x, 32);
    cc3xx_lowlevel_pka_write_reg(pt1.y, data->p1_y, 32);
    cc3xx_ec_point_affine pt2 = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(pt2.x, data->p2_x, 32);
    cc3xx_lowlevel_pka_write_reg(pt2.y, data->p2_y, 32);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();
    printf("Entering mult and add\n");
    cc3xx_lowlevel_ec_edwards_mult_and_add(&curve, &pt1, &pt2, data->scalar_a, data->scalar_b, &res);
    //cc3xx_lowlevel_ec_edwards_mult_and_add_four_bit_window(&curve, &pt1, &pt2, data->scalar_a, data->scalar_b, &res);

    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    print__debug(tmp, 8);
    print__debug(data->res_x, 8);
    assert(memcmp(tmp, data->res_x, 32) == 0);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    assert(memcmp(tmp, data->res_y, 32) == 0);
    

cleanup:
    cc3xx_lowlevel_ec_free_point(&res);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}

int cc3xx_test_ecc_edw_mult_and_add_from_msb(
                        cc3xx_ec_edw_mult_and_add_test_t *data)
{
    cc3xx_ec_curve_t curve = {};
    init_edwards_25519(&curve);

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("Mult and Add from msb: ");
    printf("%s\n", data->label);
    
    cc3xx_ec_point_affine pt1 = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(pt1.x, data->p1_x, 32);
    cc3xx_lowlevel_pka_write_reg(pt1.y, data->p1_y, 32);
    cc3xx_ec_point_affine pt2 = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(pt2.x, data->p2_x, 32);
    cc3xx_lowlevel_pka_write_reg(pt2.y, data->p2_y, 32);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();
    printf("Entering mult and add\n");
    cc3xx_lowlevel_ec_edwards_mult_and_add_from_msb(&curve, &pt1, &pt2, data->scalar_a, data->scalar_b, &res);

    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    print__debug(tmp, 8);
    print__debug(data->res_x, 8);
    assert(memcmp(tmp, data->res_x, 32) == 0);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    assert(memcmp(tmp, data->res_y, 32) == 0);
    

cleanup:
    cc3xx_lowlevel_ec_free_point(&res);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}


int cc3xx_test_ecc_edw_scalar_mult_window(
                        cc3xx_ec_edw_scalar_mult_test_t *data){

    int rc = 0;
    uint32_t tmp[8]; //to read values from reg
    printf("Window multiplication - ");
    printf("%s\n", data->label);
     
    NRF_CRYPTOCELL->ENABLE = 1;
    cc3xx_lowlevel_init();

    cc3xx_ec_curve_t curve = {};
    cc3xx_lowlevel_ec_init(CC3XX_EC_CURVE_ED25519, &curve);

    cc3xx_ec_point_affine p = cc3xx_lowlevel_ec_allocate_point();
    cc3xx_lowlevel_pka_write_reg(p.x, data->p_x, 32);
    cc3xx_lowlevel_pka_write_reg(p.y, data->p_y, 32);

    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, data->scalar, 32);

    cc3xx_ec_point_affine res = cc3xx_lowlevel_ec_allocate_point();

    //cc3xx_lowlevel_ec_edwards_scalar_mult_4_bit_window(&curve, &p, data->scalar, &res);
    cc3xx_lowlevel_ec_edwards_scalar_mult(&curve, &p, data->scalar, &res);

    
    printf("calculated results:\n");
    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    print__debug(tmp, 8);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    print__debug(tmp, 8);
    
    printf("expected results:\n");
    print__debug(data->res_x, 8);
    print__debug(data->res_y, 8);
    


    cc3xx_lowlevel_pka_read_reg(res.x,tmp, 32);
    assert(memcmp(tmp, data->res_x, 32) == 0);
    cc3xx_lowlevel_pka_read_reg(res.y,tmp, 32);
    assert(memcmp(tmp, data->res_y, 32) == 0);
    

cleanup:
    cc3xx_lowlevel_ec_free_point(&res);
    cc3xx_lowlevel_pka_free_reg(s);
    cc3xx_lowlevel_ec_free_point(&p);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}


void bits_from_lsb(cc3xx_pka_reg_id_t s, uint32_t *scalar){
    uint8_t bit = 0x0;
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    for(int j=0; j<32;j++){
        uint8_t byte = j*8;
        //go trough all bits
        for(int i = 0; i < 8; i++){
            bit = cc3xx_lowlevel_pka_test_bits_ui(s, byte+i, 1);
            printf("%01lx",bit);
        }
    }
    printf("\n");
}

void bits_from_msb(cc3xx_pka_reg_id_t s, uint32_t *scalar){
    uint8_t bit = 0x0;
    cc3xx_lowlevel_pka_write_reg_swap_endian(s, scalar, 32);
    //trough 32 bytes
    for(int j=0; j<32;j++){
        uint8_t byte = j*8;
        //go trough all bits
        for(int i = 7; i >= 0; i--){
            bit = cc3xx_lowlevel_pka_test_bits_ui(s, byte+i, 1);
            printf("%01lx",bit);
        }
    }
    printf("\n");
}

void halfbytes_from_msB(cc3xx_pka_reg_id_t s, uint32_t *scalar){
    uint32_t halfbyte = 0x0;
    cc3xx_lowlevel_pka_write_reg_swap_endian(s, scalar, 32);
    //cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    for(int i = 0; i < (256/4); i++){
        halfbyte = cc3xx_lowlevel_pka_test_bits_ui(s, 4, 4);
        printf("%1x ", halfbyte);
        cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 4, s);
    }
    printf("\n");
}

void bytes_from_msB(cc3xx_pka_reg_id_t s, uint32_t *scalar){
    uint32_t byte = 0x0;
    cc3xx_lowlevel_pka_write_reg_swap_endian(s, scalar, 32);
    //cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    for(int i = 0; i < 32; i++){
        uint8_t bytestart = i*8;
        byte = cc3xx_lowlevel_pka_test_bits_ui(s, bytestart+4, 4);
        printf("%1lx", byte);
        byte = cc3xx_lowlevel_pka_test_bits_ui(s, bytestart, 4);
        printf("%1lx ", byte);
        //if(i%8 == 0) cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 8, s);
    }
    printf("\n");
}

void bytes_from_msB_anders(cc3xx_pka_reg_id_t s, uint32_t *scalar){
    uint32_t byte = 0x0;
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    //cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    for(int i = 255; i >= 0; i--){
        if(i%4 == 0){
            byte = cc3xx_lowlevel_pka_test_bits_ui(s, i, 4);
            printf("%1lx", byte);
        }
        //if(i%8 == 0) cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 8, s);
    }
    printf("\n");
}


int simple_things(void)
{
    cc3xx_ec_curve_t curve = {};
    init_edwards_25519(&curve);
    int rc = 0;

    uint32_t bits;
    //uint32_t scalar = 13;
    //0x1000000000000000000000000000000014def9dea2f79cd65812631a5cf5d3ec
    //0b0001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010100110111101111100111011110101000101111011110011100110101100101100000010010011000110001101001011100111101011101001111101100
    //0xecd3f55c1a631258d69cf7a2def9de1400000000000000000000000000000010
    //0b1110110011010011111101010101110000011010011000110001001001011000110101101001110011110111101000101101111011111001110111100001010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000
    //least significant to most significant bit
    //0011011111001011101011110011101001011000110001100100100000011010011010110011100111101111010001010111101110011111011110110010100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000
    uint32_t lm1[8] = {0x5cf5d3ec,0x5812631a,0xa2f79cd6,0x14def9de, 0x00000000,0x00000000,0x00000000,0x10000000};
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    
    cc3xx_lowlevel_pka_clear(s);
    cc3xx_lowlevel_pka_write_reg(s, lm1, 32);
    debug_read_and_print_reg(s, "l-1 : ");
    /*for(int i = 15; i >= 0; i--){
        //for(int j = 7; j >= 0; j--){
            bits = cc3xx_lowlevel_pka_test_bits_ui(s, i, 1);
            printf("%ith Bit: %08lx\n",16-i, bits);
        //}
        //cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 1, s);
    }*/
    /*
    for(int j=0; j<32;j++){
        uint8_t byte = j*8;
        //go trough all bits
        for(int i = 7; i >= 0; i--){
        //for(int j = 7; j >= 0; j--){
            bits = cc3xx_lowlevel_pka_test_bits_ui(s, byte+i, 1);
            //printf("byte %d: %dth Bit: %01lx\n",byte, 8-i, bits);
            printf("%01lx",bits);
        //}
        //cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 1, s);
        }
    }
    */
    printf("\n");
    bits_from_lsb(s, lm1);
    (void) bits;

    /*
    for(int i = 0; i <= 4; i++){
        bits = cc3xx_lowlevel_pka_test_bits_ui(s, 0, i);
        printf("First %d Bits: %08lx\n", i, bits);
    }
    cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 4, s);
    for(int i = 0; i <= 4; i++){
        bits = cc3xx_lowlevel_pka_test_bits_ui(s, 0, i);
        printf("First %d Bits: %08lx\n", i, bits);
    }*/

    cc3xx_lowlevel_pka_clear(s);
    cc3xx_lowlevel_pka_write_reg_swap_endian(s, lm1, 32);
    debug_read_and_print_reg(s, "l-1 swap endian: ");
    //bits_from_msb_to_lsb(s, lm1);
    
    /*
    for(int i = 0; i <= 4; i++){
        bits = cc3xx_lowlevel_pka_test_bits_ui(s, 0, i);
        printf("First %d Bits: %08lx\n", i, bits);
    }
    cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 4, s);
    for(int i = 0; i <= 4; i++){
        bits = cc3xx_lowlevel_pka_test_bits_ui(s, 0, i);
        printf("First %d Bits: %08lx\n", i, bits);
    }
    */
    //per byte
    /*
    for(int j=0; j<32;j++){
        uint8_t byte = j*8;
        //go trough all bits
        for(int i = 7; i >= 0; i--){
        //for(int j = 7; j >= 0; j--){
            bits = cc3xx_lowlevel_pka_test_bits_ui(s, byte+i, 1);
            //printf("byte %d: %dth Bit: %01lx\n",byte, 8-i, bits);
            printf("%01lx",bits);
        //}
        //cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 1, s);
        }
    }
    */
    printf("\n");
    cc3xx_lowlevel_pka_clear(s);
    bits_from_msb(s, lm1);
    //bits_from_lsb(s, lm1);

    cc3xx_lowlevel_pka_clear(s);
    halfbytes_from_msB(s,lm1);
    uint32_t scalar_70[8] = {0x46};
    for(int i=0; i<8; i++) printf("%08lx ", scalar_70[i]);
    printf("\n");
    cc3xx_lowlevel_pka_write_reg(s, scalar_70, 32);
    debug_read_and_print_reg(s, "70: ");
    cc3xx_lowlevel_pka_write_reg_swap_endian(s, scalar_70, 32);
    debug_read_and_print_reg(s, "70 se: ");
    printf("from lsb: ");
    bits_from_lsb(s, scalar_70);
    printf("\n");
    printf("from msb: ");
    bits_from_msb(s, scalar_70);
    cc3xx_lowlevel_pka_write_reg_swap_endian(s, scalar_70, 32);
    printf("0,4: ");
    bits = cc3xx_lowlevel_pka_test_bits_ui(s, 0, 4);
    printf("%08lx \n", bits);
    printf("4,4: ");
    bits = cc3xx_lowlevel_pka_test_bits_ui(s, 4, 4);
    printf("%08lx \n", bits);
    debug_read_and_print_reg(s, "before shift: ");
    cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 248, s);
    debug_read_and_print_reg(s, "after shift: ");
    printf("0,4: ");
    bits = cc3xx_lowlevel_pka_test_bits_ui(s, 0, 4);
    printf("%08lx \n", bits);
    printf("4,4: ");
    bits = cc3xx_lowlevel_pka_test_bits_ui(s, 4, 4);
    printf("%08lx \n", bits);
    bytes_from_msB(s, scalar_70);
    bytes_from_msB_anders(s, scalar_70);

cc3xx_ec_point_extended_data table_g[16] = {
{
 .x={0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 ,0x00000000}, 
 .y={0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 ,0x00000000}, 
 .z={0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 ,0x00000000}, 
 .t={0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 ,0x00000000}},
{
 .x={0x8f25d51a, 0xc9562d60, 0x9525a7b2, 0x692cc760, 0xfdd6dc5c, 0xc0a4e231, 0xcd6e53fe, 0x216936d3},
 .y={0x66666658, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666, 0x66666666},
 .z={0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000},
 .t={0xa5b7dda3, 0x6dde8ab3, 0x775152f5, 0x20f09f80, 0x64abe37d, 0x66ea4e8e, 0xd78b7665, 0x67875f0f}}, 
{
 .x={0x9f3fd67e, 0x1515e6cf, 0x163a7692, 0xde44f888, 0x213c1bd9, 0x5776d1e1, 0x960f6ad4, 0x3b6f8891},
 .y={0x874a007e, 0xb6d34c9a, 0x5107378d, 0xd6e15667, 0x14dab827, 0x5921f40f, 0x4cdb3092, 0x336d9ece},
 .z={0xf61c8608, 0x722d5b0b, 0x31b598ba, 0x50b27bff, 0x12f675b4, 0xfd9cb817, 0x52a20ea2, 0x59e4ea1a},
 .t={0x92cad989, 0xc532c7e3, 0x483d139b, 0x72749500, 0xdd5e07c1, 0xfc6ea6fe, 0x2d298daa, 0x1f6e08da}},
{
 .x={0xc90df8e0, 0x594156cf, 0xb6af56b3, 0xdb062ccc, 0x1d35e449, 0xf8f205b0, 0xbe5fea71, 0x7c79bd81},
 .y={0x50735ca7, 0xd374c07f, 0x1653a9c9, 0x586d1bf2, 0xe0361d1a, 0x60cdc34f, 0xeba89914, 0x1eebd8c6},
 .z={0xfba69efe, 0xf9bc59d9, 0x9d523275, 0x8cf3c044, 0xa2402c95, 0x94e0c159, 0x83ace42b, 0x0101f450},
 .t={0x8aa004bb, 0x270156e4, 0xc0865f3d, 0x1ffa6f28, 0x949fa15b, 0xbf0d879f, 0x665df81d, 0x1217d7af}},
{
 .x={0xde8526bf, 0x190363fc, 0xb45152fd, 0x026efc0e, 0x19a8bcb2, 0x992dcb58, 0x8ff7f1dd, 0x3349374b},
 .y={0x0a55f002, 0x08877c9e, 0x242c2966, 0x4f429b8d, 0xebae9743, 0x44727563, 0x155e7f79, 0x7b444d3f},
 .z={0xdc80e4cd, 0xc8f193b1, 0xc0ae6252, 0x2484a2d4, 0x5363bf11, 0x2a54b8b7, 0xe83c105d, 0x045be850},
 .t={0x28269de7, 0x1589fd8f, 0x3832aaed, 0x47cd629a, 0x5dd5e6a0, 0xeeba22a9, 0xd006395d, 0x59a06515}},
 {
 .x={0xe2986bd1, 0x091674db, 0x44e5cb41, 0xea1ef5b5, 0x94ab9b48, 0x98dee11e, 0xda2f30b0, 0x2ab2a5b6},
 .y={0x7c6e9b73, 0xd1a5c358, 0x0b33a957, 0x06f9d269, 0xf81dc498, 0xbe632753, 0x20ccd8f4, 0x69bae0f2},
 .z={0xada5c85d, 0x1d106417, 0x824e2b8b, 0xf9ca4e45, 0x882552b1, 0x73516c8e, 0x033fb147, 0x5699f014},
 .t={0xcd7b5be4, 0x72e1a7ce, 0xe898eaa0, 0x8576d38f, 0x2522f3f8, 0x70dff65e, 0x9aec9e66, 0x5102c2e9}},
{
 .x={0x3589c7fb, 0x4c7fad5c, 0x2e202b31, 0x9d12c35e, 0xedcc1707, 0x891f67e5, 0x94d860d6, 0x1bdf6170},
 .y={0x04cf5337, 0x6216255d, 0x00ae18f8, 0x5e31820c, 0xf0784ddb, 0x9104272a, 0x39c5189c, 0x29013ddf},
 .z={0x806a5d9d, 0x860d5220, 0xbc58c0eb, 0xd377c309, 0x1dddf6c3, 0x453e3b54, 0xfa7d2efb, 0x4e9738d2},
 .t={0xca97e1db, 0xabdfd6b4, 0xf1ac589a, 0x3e9631fb, 0xc6027537, 0xccc05767, 0x9379ab67, 0x2a53f297}},
{
 .x={0xc1ac2631, 0x0c8e15be, 0x9cebd618, 0x9661844a, 0xee93875b, 0xf1adee87, 0x2e2895c6, 0x1d378124},
 .y={0x6d655b61, 0x1fd1a762, 0xb7ca283d, 0xa76f353c, 0x05c21e8e, 0x9190c7d5, 0xf5ceb881, 0x536fd5af},
 .z={0x01d1a6a4, 0x11d4c368, 0x71f70d02, 0x317776f3, 0xd2e5a1df, 0x098aa999, 0x71acd7dc, 0x4ef4533a},
 .t={0x321a6f9c, 0xd7e9e56a, 0x3cc3ed09, 0xe00d0f61, 0xbc2d3ab9, 0xbe4e6555, 0x998b2376, 0x66b8aa0d}},
{
 .x={0x824cd1d9, 0x1add7970, 0xfa3233c8, 0x51104cfa, 0xc55b6cc5, 0xd93b0000, 0x7f38d967, 0x592a134f},
 .y={0x6d78fafc, 0xbc44ee68, 0x9c23ad74, 0xe5ac62f9, 0x2b3d8c4d, 0x5469c3ea, 0xafb4e5d4, 0x1d8fdfa3},
 .z={0xb78555f5, 0x2f4729e8, 0x0a47cb5f, 0x82ffd12d, 0x203de1e0, 0xb3101315, 0xcba0db22, 0x70e6708d},
 .t={0xf0a2a9ed, 0xbd79f7cb, 0x151169a2, 0x579f84de, 0x9801fe0f, 0x0f1d3f5b, 0xe8207fbf, 0x6850e0db}},
{
 .x={0x17869f29, 0x577615e8, 0xe7b8a0a0, 0x919b611c, 0x2a851888, 0xa34c7b7a, 0x57aaff06, 0x7f41cdc8},
 .y={0x78f88310, 0x57486e48, 0x13db90fa, 0x53ca5318, 0xc2303273, 0xf5319ae0, 0x8cca520a, 0x77458428},
 .z={0x4dd2ef61, 0x5322fb21, 0xa6902d57, 0x1ebca9c9, 0x7d45d0be, 0xa9efb6ee, 0x1052b3bd, 0x34fa437f},
 .t={0x1e02ebf7, 0x21deb1be, 0xb2bb07e8, 0xe1e09905, 0x7652cda8, 0x4c97747f, 0xf813d3ae, 0x3db50ba8}},
{
 .x={0x5eebea57, 0x5fe80e94, 0x30fa98aa, 0x57a8d311, 0x7f81775b, 0x6a2eb04d, 0xfc9da38b, 0x7ca8c99d},
 .y={0x31d260d3, 0x1c060fe8, 0xeb5f1ae8, 0x35b7c2f0, 0x84ba9d69, 0xfd0244ac, 0xe41e38a3, 0x015c96f0},
 .z={0x7c761dab, 0x72dde80c, 0x78bbed73, 0x237aaf70, 0xdbbd2e76, 0xef8d2ba6, 0xe0f41873, 0x6612ceea},
 .t={0x288b3d66, 0x74c60221, 0x06bfce47, 0x0a70c008, 0xca68af1e, 0xfd23768a, 0x937b9073, 0x1ab3f536}},
{
 .x={0x35fc6914, 0xfa7a9b36, 0x7b0464f6, 0xa7854693, 0x34f95b54, 0xa9d60294, 0xba964842, 0x3a08c55d},
 .y={0xab8b3bad, 0x0b444dff, 0x6d14e959, 0x9660ccd1, 0xf4fe2a38, 0x7a17bd0c, 0x45da2546, 0x54577a6f},
 .z={0xbcd30d0a, 0x835b5919, 0x2661388c, 0x8c014ca9, 0x846f44fb, 0x30135451, 0x6907002a, 0x24dc1434},
 .t={0x3b408934, 0x5bb2ca5a, 0x17182f4c, 0xa3b9cc87, 0x0c26c417, 0x91120e9d, 0xc246f261, 0x531ba123}},
{
 .x={0x894eefe8, 0x9a2bda9f, 0x41bbb397, 0xb5b319d8, 0x7c310fea, 0xb2117f9d, 0xdcfc9df7, 0x4cb58f88},
 .y={0x7df66147, 0x81627756, 0x753aa21b, 0x67413858, 0x690c067f, 0x50dcb3df, 0x46ad9bbe, 0x7843d5e4},
 .z={0x82c10e88, 0x5c459a75, 0xce492652, 0x1bcea0de, 0xf5922b55, 0xe9eff01a, 0x5d0bfc57, 0x0dfc907e},
 .t={0x3800b442, 0x7fd20d02, 0x20b77ebe, 0xdf84858a, 0x6f74a591, 0xa2be2dee, 0xa99ad196, 0x1ac0064b}},
{
 .x={0xc6469094, 0x8480149c, 0xf47d8c4b, 0x8b276ed0, 0x4652cbf1, 0x2bb14287, 0x7c947007, 0x0622e879},
 .y={0xa259c979, 0x87568ca0, 0x350e450b, 0xc09631ae, 0xaf4d5ca4, 0x3ff15a11, 0xc1494769, 0x11a559bb},
 .z={0x956d28d4, 0x4aa059a3, 0x576155d4, 0x93e53d44, 0xb23afb7c, 0x58dd9d96, 0x6049b5e2, 0x59a225da},
 .t={0xbc6dfff6, 0x9abc28e6, 0x287348a5, 0x0f29d66d, 0x40dfe908, 0x5f6d5d4e, 0x4974c81c, 0x2e013bdb}},
{
 .x={0xd4065f0f, 0x0b2eadb3, 0xb8f2abab, 0xbd1208c5, 0x42f55f3c, 0x419325f6, 0x6969277a, 0x627471a9},
 .y={0xa5c35e7d, 0x88977ac0, 0x51d79fcf, 0x655e8553, 0x397dea5e, 0xaf555f28, 0x79989c0c, 0x1c5b41b2},
 .z={0xc42fec91, 0xa70e58dc, 0x7c248f0f, 0x8082122f, 0xa81ad15a, 0x09770889, 0xd7ae22e6, 0x309488be},
 .t={0xe87d52f5, 0x494d0576, 0x21261257, 0xaa9aa708, 0x531285b5, 0x2c5e4ce5, 0x7d464347, 0x3e5a47e8}},
{
 .x={0x671ae865, 0x4f2e8748, 0x2b953524, 0xa9f9880f, 0x5b749832, 0x29b2a59d, 0x8b2d7a2c, 0x3aea74f0},
 .y={0x5da74113, 0xca116373, 0x989aa4a2, 0x733cda36, 0xf27b8f99, 0x9d6896fb, 0xdc49f496, 0x7f9d6c35},
 .z={0xc9d90a24, 0x076d826c, 0x5be8c947, 0x9a679046, 0xa7300bd4, 0xca3509a0, 0x7f39f912, 0x533552e0},
 .t={0xfd6ea971, 0xfbcf5b37, 0x151d8146, 0x58e13da2, 0xdc3e2021, 0x72f06465, 0xcddb7d4f, 0x32a3f803}}
};

    cc3xx_ec_point_extended_data table[16];
    allocate_addition_registers();
    cc3xx_ec_point_extended g = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_affine_to_extended(&curve, &curve.generator, &g);
    calculate_table(&curve, &g, table);
    cc3xx_lowlevel_ec_free_extended_point(&g);
    free_addition_registers();
    for(int i=0;i<16;i++){
        
        rc = memcmp(&table[i], &table_g[i], 32*4);
        if(rc!=0){
            printf("%d differs\n", i);
            //print__debug(table[i].x, 8);    
        }
        /*printf("Y: ");
        print__debug(table[i].y, 8);
        printf("Z: ");
        print__debug(table[i].z, 8);
        printf("T: ");
        print__debug(table[i].t, 8);*/
    }
    

cleanup:
    cc3xx_lowlevel_pka_free_reg(s);
    cc3xx_lowlevel_ec_uninit();
    NRF_CRYPTOCELL->ENABLE = 0;
    return rc;
}



/*further test ideas
decompression tests:
- point that can't be decoded
- point with wrong X coordinate set 
addition:
- stuff with identity elements
*/

static void ecc_edwards_tests_run(struct test_result_t *ret)
{

    printf("POINT DECOMPRESSION TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_generator) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_decompress_point(&point_decompress_r_pub_from_taler) == 0, "Point decompression did not succeed");
    printf("POINT DECOMPRESSION TESTS PASSED\n\n");

    printf("POINT COMPRESSION TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_compress_point(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_compress_point(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_compress_point(&point_decompress_generator) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_compress_point(&point_decompress_r_pub_from_taler) == 0, "Point decompression did not succeed");
    printf("POINT COMPRESSION TESTS PASSED\n\n");

    printf("POINT ON CURVE TESTS \n");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_odd_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_even_x) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_point_on_curve(&point_decompress_generator) == 0, "Point decompression did not succeed");
    printf("POINT ON CURVE TESTS PASSED\n\n");

    printf("EXTENDEND POINT TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_extended_point(&generator_to_extendend) == 0, "Point decompression did not succeed");
    printf("EXTENDEND POINT TESTS PASSED\n\n");

    printf("POINT ADDITION TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&generator_plus_generator) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&G2_plus_G2) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&twenty_G_plus_fifty_G) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&l_minus_one_times_G_plus_G) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_addition(&l_minus_one_times_G_plus_2G) == 0, "Point decompression did not succeed");
    printf("POINT ADDITION PASSED\n\n");

    printf("POINT DOUBLING TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_doubling(&double_generator) == 0, "Point decompression did not succeed");
    //TEST_ASSERT(cc3xx_test_ecc_edw_doubling_extended_coord(&double_2G) == 0, "Point decompression did not succeed"); //needs setting of registers now
    printf("POINT DOUBLING TESTS PASSED \n\n");

    printf("SCALAR MULT TESTS\n");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_1) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_70) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_0x1000) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_0x10000) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_l) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_l_minus_one) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult(&gen_times_l_plus_one) == 0, "Point decompression did not succeed");

    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_1) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_70) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_0x1000) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_0x10000) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_l) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_l_minus_one) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_generator(&gen_times_l_plus_one) == 0, "Point decompression did not succeed");

    TEST_ASSERT(cc3xx_test_ecc_edw_scalar_mult_window(&gen_times_70) == 0, "Point decompression did not succeed");

    //TEST_ASSERT(cc3xx_test_ecc_edw_mult_and_add_slow(&twenty_G_plus_fifty_G_ma) == 0, "Point decompression did not succeed");
    TEST_ASSERT(cc3xx_test_ecc_edw_mult_and_add(&twenty_G_plus_fifty_G_ma) == 0, "Point decompression did not succeed");

    
    printf("SCALAR MULT TESTS PASSED\n\n");

    simple_things();

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
