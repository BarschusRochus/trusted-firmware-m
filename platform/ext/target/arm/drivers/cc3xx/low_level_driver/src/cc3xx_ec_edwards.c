#include "cc3xx_ec_edwards.h"
#include "cc3xx_ec.h"
#include "cc3xx_ec_edw_extended_point.h"
#include "cc3xx_error.h"
#include "cc3xx_pka.h"
#include <stddef.h>
#include <stdint.h>

//debug
#include <stdio.h>
#include <string.h>

void print_debug(char* msg, uint32_t *debug, size_t len){
    printf("%s",msg);
    for(size_t i=0; i < len; i++){
        printf("%08lx ", debug[i]);
    }
    printf("\n");
}

void debug_read_and_print_reg(cc3xx_pka_reg_id_t reg, char* label){
    uint32_t debug[8] = {0};
    size_t debug_len = 32;
    size_t debug_bytes = 8;
    printf("Register idx %ld - ", reg);
    cc3xx_lowlevel_pka_read_reg(reg, debug, debug_len);
    print_debug(label, debug, debug_bytes);
}

static cc3xx_ec_point_extended_data table_g[16] = {
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

//addition/doubling registers
static cc3xx_pka_reg_id_t A;
static cc3xx_pka_reg_id_t B;
static cc3xx_pka_reg_id_t C;
static cc3xx_pka_reg_id_t D;
static cc3xx_pka_reg_id_t E;
static cc3xx_pka_reg_id_t F;
static cc3xx_pka_reg_id_t G;
static cc3xx_pka_reg_id_t H;

void allocate_addition_registers(void){
    A = cc3xx_lowlevel_pka_allocate_reg();
    B = cc3xx_lowlevel_pka_allocate_reg();
    C = cc3xx_lowlevel_pka_allocate_reg();
    D = cc3xx_lowlevel_pka_allocate_reg();
    E = cc3xx_lowlevel_pka_allocate_reg();
    F = cc3xx_lowlevel_pka_allocate_reg();
    G = cc3xx_lowlevel_pka_allocate_reg();
    H = cc3xx_lowlevel_pka_allocate_reg();
}

void free_addition_registers(void){
    cc3xx_lowlevel_pka_free_reg(H);
    cc3xx_lowlevel_pka_free_reg(G);
    cc3xx_lowlevel_pka_free_reg(F);
    cc3xx_lowlevel_pka_free_reg(E);
    cc3xx_lowlevel_pka_free_reg(D);
    cc3xx_lowlevel_pka_free_reg(C);
    cc3xx_lowlevel_pka_free_reg(B);
    cc3xx_lowlevel_pka_free_reg(A);    
}

/**
 * @brief Decompress a compressed edwards25519 point to x,y affine coordinates.
 *        This code is adapted from the cc312_runtime point decoding code.
 *        It follows the steps of RFC 8032 5.1.3 and assumes that the coodrinate
 *        is encoded as describted in 5.1.2
 *        This whole procedure is very well explained in 
 *        https://words.filippo.io/edwards25519-formulas/
 * @param reg_y [in] register containing compressed y coordinate
 * @param isOddX [in] most significant bit of compressed coordinate, 
                    i.e. least significant bit of x coordinate
 * @param decompressed_pt [out] result of decompression
 * @param curve [in] curve object
 */
//void cc3xx_lowlevel_ec_edw_decompress_point_pka(cc3xx_pka_reg_id_t reg_y, uint32_t isOddX, 
//        cc3xx_ec_point_affine *decompressed_pt, cc3xx_ec_curve_t *curve){
cc3xx_err_t cc3xx_lowlevel_ec_edw_decompress_point_pka(cc3xx_ec_point_affine *decompressed_pt, 
            uint32_t isOddX, cc3xx_ec_curve_t *curve){
        // decompress: (YP) -> (XP,YP,ZP=1,TP) 
        // tw. edw curve= ax^2 + y^2 = 1 + dx^2y^2 ==> x = sqrt(1-y^2 / 1-dy^2)

        cc3xx_lowlevel_pka_unmap_physical_registers();
        uint32_t bit0;

        cc3xx_pka_reg_id_t reg_x = decompressed_pt->x;
        cc3xx_pka_reg_id_t reg_y = decompressed_pt->y;
        //allocate registers for calculations
        cc3xx_pka_reg_id_t t = cc3xx_lowlevel_pka_allocate_reg();
        cc3xx_pka_reg_id_t t3 = cc3xx_lowlevel_pka_allocate_reg();
        cc3xx_pka_reg_id_t t4 = cc3xx_lowlevel_pka_allocate_reg();
        cc3xx_pka_reg_id_t t5 = cc3xx_lowlevel_pka_allocate_reg();
        
        //setzt N auf 2^255-19 weil das cc3xx so vorsieht
        //TODO: nach draussen verlagern?
        //cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

        //start with creation of u and v
        cc3xx_lowlevel_pka_mod_mul(reg_y, reg_y, t3); //y^2 
        cc3xx_lowlevel_pka_mod_mul(t3, curve->param_d, t4); //y^2*d                                                                             
        cc3xx_lowlevel_pka_sub_si(t3, 1, t3); //u => t3 = y^2 - 1
        cc3xx_lowlevel_pka_add_si(t4, 1, t4); //v => t4 = d* y^2 + 1
        
        //calculate x as u * v^3 * (u v^7)^((p-5)/8)
        cc3xx_lowlevel_pka_mod_mul(t4, t4, t); //v^2
        cc3xx_lowlevel_pka_mod_mul(t4, t, t); //t = v^3
        cc3xx_lowlevel_pka_mod_mul(t, t, reg_x); //v^6
        cc3xx_lowlevel_pka_mod_mul(t4, reg_x, reg_x); //v^7
        cc3xx_lowlevel_pka_mod_mul(t3, reg_x, t5); //u*v^7

        //q58 = (p-5)/8 with p = 2^255 - 19
        cc3xx_lowlevel_pka_mod_exp(t5, curve->q58, reg_x); //(u v^7)^((p-5)/8)
        cc3xx_lowlevel_pka_mod_mul(t3, reg_x, reg_x); //(u*v^7)^((p-5)/8) * u
        cc3xx_lowlevel_pka_mod_mul(t, reg_x, reg_x); //(u*v^7)^((p-5)/8) * uv^3 = x
        
        //check cases for whether x is a square root
        cc3xx_lowlevel_pka_mod_mul(reg_x, reg_x, t); // ((u*v^7)^((p-5)/8) * uv^3)^2 = x^2
        cc3xx_lowlevel_pka_mod_mul(t4, t, t); //x^2 * v
        cc3xx_lowlevel_pka_add(t3, t, t); //x^2 * v + u (<=> x^2 * v = -u )
        cc3xx_lowlevel_pka_div(t, curve->field_modulus, t4, t); //(x^2 * v + u) / p = quotient (t4) + remainder (t)
        //(x^2 * v + u) % p =?= 0     this is case 2 (x^2 * v + u <=> x^2 * v = -u )
        bit0 = cc3xx_lowlevel_pka_are_equal_si(t, 0); 
        if(bit0) {// (x^2 * v + u) % p == 0
                //sqrt_m1 = sqrt(-1) % p
                cc3xx_lowlevel_pka_mod_mul(reg_x, curve->sqrt_m1, reg_x); // x <-- x * 2^((p-1)/4)
        }else{ //(x^2 * v + u) % p != 0
            //I think this means case 1 is assumed - but never really checked
            //(x^2 * v + u) % p != 0 <=!=> (x^2 * v = u) % p
            //TODO: remove empty else branch?
        }

        cc3xx_lowlevel_pka_div(reg_x, curve->field_modulus, t4, reg_x); //x / p = quotient (t4) + remainder (x) => x = x mod p
        

        //problem: x decoded to 0 but oddity is 1 -> cannot be
        if(cc3xx_lowlevel_pka_are_equal_si(reg_x, 0)){
            if(isOddX == 1){
                cc3xx_lowlevel_pka_free_reg(t5);
                cc3xx_lowlevel_pka_free_reg(t4);
                cc3xx_lowlevel_pka_free_reg(t3);
                cc3xx_lowlevel_pka_free_reg(t);
                //find a better return code
                return CC3XX_ERR_FAULT_DETECTED;
            }
        }

        //last part of RFC decode procedure
        bit0 = cc3xx_lowlevel_pka_test_bits_ui(reg_x, 0, 1); //read bit[0]

        //decide about proper square root by checking least significant bit of x (odd/even)
        if(bit0 != isOddX){ //if isOddx != x % 2 
            cc3xx_lowlevel_pka_sub(curve->field_modulus, reg_x, reg_x); //x <-- p - x
        }
        
        //TODO: write y to decompressed_pt
        //TODO: need to care about first bit, must always be set to 0
        //cc3xx_lowlevel_pka_copy(reg_y, decompressed_pt->y);
        
        //finished, free all registers
        cc3xx_lowlevel_pka_free_reg(t5);
        cc3xx_lowlevel_pka_free_reg(t4);
        cc3xx_lowlevel_pka_free_reg(t3);
        cc3xx_lowlevel_pka_free_reg(t);

        return CC3XX_ERR_SUCCESS;

}

cc3xx_err_t cc3xx_lowlevel_ec_edw_decompress_point(uint32_t *compressed, 
                    cc3xx_ec_curve_t *curve, cc3xx_ec_point_affine *decompressed)
{
    int rc = CC3XX_ERR_SUCCESS;
    uint32_t isOddX = 0x00;
    //this assumes that the compressed point is given as 8 uint32_t blocks 
    //this takes the most significant bit of the 8th uint32_t 
    //that is assumed to be the most significant bit of the represented number
    isOddX |= compressed[7] >> 31;

    //uncompress y value by setting most significant bit to 0
    uint32_t y[8] = {0x0};
    memcpy(y, compressed, 32);
    y[7] &= 0b01111111111111111111111111111111;

    //setzt N auf 2^255-19
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    //prepare registers for decompress operation
    cc3xx_lowlevel_pka_write_reg(decompressed->y, y, 32);
    rc = cc3xx_lowlevel_ec_edw_decompress_point_pka(decompressed, 
        isOddX, curve);


    if( !cc3xx_lowlevel_ec_edw_is_point_on_curve(decompressed, curve)){
        //TODO: find a better error code
        return CC3XX_ERR_EC_POINT_OUTSIDE_FIELD;
    }

    //setzt N auf curve oder
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    

    return rc;
} 

//set least significant bit of x coordinate as most significant bit of y coordinate
void cc3xx_lowlevel_ec_edw_compress_point(cc3xx_ec_curve_t *curve, cc3xx_ec_point_affine *point, 
                    uint32_t *compressed)
{
    //get bit 0
    uint32_t bit0 = cc3xx_lowlevel_pka_test_bits_ui(point->x, 0, 1);
    //y to compressed - msb is always 0
    cc3xx_lowlevel_pka_read_reg(point->y, compressed, 32);
    
    //if x is odd set msb in y to 1
    if (bit0 == 1){
        compressed[7] |= 0b10000000000000000000000000000000;
    }
    

}


//effectively evaluate the equation -x^2+y^2 mod p = 1 + d(x^2y^2)
bool cc3xx_lowlevel_ec_edw_is_point_on_curve(cc3xx_ec_point_affine *p, cc3xx_ec_curve_t *curve){

    bool rc = 0;
    //TODO: less regs possible? Guess so
    cc3xx_pka_reg_id_t xx = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t yy = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t xxyy = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t rhs = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t lhs = cc3xx_lowlevel_pka_allocate_reg();

    //setzt N auf 2^255-19
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    cc3xx_lowlevel_pka_mod_mul(p->x, p->x, xx); //x * x
    cc3xx_lowlevel_pka_mod_mul(p->y, p->y, yy); //y * y
    cc3xx_lowlevel_pka_mod_mul(xx, yy, xxyy); //xx * yy

    cc3xx_lowlevel_pka_sub_si(curve->field_modulus, 0x1, lhs); // lhs = a mod p = p -1 in case of a = -1


    //cc3xx_lowlevel_pka_div(curve->param_a, curve->field_modulus, rhs, lhs); //lhs = a % p, rhs = a / p - rhs will be overwritten, no need for it
    cc3xx_lowlevel_pka_mod_mul(lhs, xx, lhs); //lhs = a * xx //Todo: does that work? If so I may as well hardcode that
    cc3xx_lowlevel_pka_mod_add(lhs, yy, lhs); // lhs = lhs + yy
    cc3xx_lowlevel_pka_div(lhs, curve->field_modulus, rhs, lhs); //lhs = lhs % p, rhs = lhs / p - quotient again unimportant

    //rhs
    cc3xx_lowlevel_pka_mod_mul(curve->param_d, xxyy, rhs); //rhs = d * xxyy
    cc3xx_lowlevel_pka_mod_add_si(rhs, 0x1, rhs); // rhs = 1 + rhs
    cc3xx_lowlevel_pka_div(rhs, curve->field_modulus, xx, rhs); //rhs = rhs % p, xx = rhs / p - quotient again unimportant
    
    //debug_read_and_print_reg(rhs, "RHS: ");
    //debug_read_and_print_reg(lhs, "LHS: ");

    rc = cc3xx_lowlevel_pka_are_equal(lhs, rhs);


    cc3xx_lowlevel_pka_free_reg(lhs);
    cc3xx_lowlevel_pka_free_reg(rhs);
    cc3xx_lowlevel_pka_free_reg(xxyy);
    cc3xx_lowlevel_pka_free_reg(yy);
    cc3xx_lowlevel_pka_free_reg(xx);
    //return (rhs == lhs)
    
    //setzt N auf curve oder
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    
    
    return rc;

}


/*
 * About k
 * First option: See "Twisted Edwards Curves Revisited", 2008 by Hisil, Wong, Carter, Dawson chap. 3.1
 * Second option: See in library c25519; ed25519_k in ed25519.c - this is where this is adapted from
 * Third option: Calculate following orders form 1st option:
 * d, a from twisted edwards 25519 formula, p = 2^255-19
 * d_ = -d /a (as a =-1 this should be d)
 * k = 2 * d_ % p
 */

cc3xx_err_t cc3xx_lowlevel_ec_edwards_add_extended_points(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_extended *p, cc3xx_ec_point_extended *q, cc3xx_ec_point_extended *res)
{

    //this removes the currently mapped registers and saves the addresses under
    //which they are stored for later
    //as plenty of registers are needed here this simply makes room in the memory
    //map
    cc3xx_lowlevel_pka_unmap_physical_registers();

    //allocate_addition_registers();

    //parameter k = 2d'; d' = -d/a (mod field modulus); see above
    //k = 0x2406d9dc56dffce7198e80f2eef3d13000e0149a8283b156ebd69b9426b2f159
    uint32_t k[8] = {0x26b2f159, 0xebd69b94, 0x8283b156, 0x00e0149a,
                     0xeef3d130, 0x198e80f2, 0x56dffce7, 0x2406d9dc};

    // compute A = (Y1-X1)(Y2-X2)
    cc3xx_lowlevel_pka_mod_sub(p->y, p->x, A); //(Y1-X1)
    cc3xx_lowlevel_pka_mod_sub(q->y, q->x, B); //(Y2-X2)
    cc3xx_lowlevel_pka_mod_mul(A, B, A); //A = (Y1-X1)(Y2-X2)
    
    //compute B = (Y1+X1)(Y2+X2)
    cc3xx_lowlevel_pka_mod_add(p->y, p->x, B); //(Y1+X1)
    cc3xx_lowlevel_pka_mod_add(q->y, q->x, C); //(Y2+X2)
    cc3xx_lowlevel_pka_mod_mul(C, B, B); //B = (Y1+X1)(Y2+X2)
    
    //compute C = T1 k T2
    cc3xx_lowlevel_pka_mod_mul(p->t, q->t, C); //T1 * T2
    cc3xx_lowlevel_pka_write_reg(D, k, 32); // D=k
    cc3xx_lowlevel_pka_mod_mul(C, D, C); //C = T1 * T2 * k
    
    //compute D = Z1 2 Z2
    cc3xx_lowlevel_pka_mod_mul(p->z, q->z, D); //D = Z1 * Z2
    cc3xx_lowlevel_pka_mod_mul_si(D, 0x2, D); //D = Z1 * Z2 * 2
    
    //compute E = B - A 
    cc3xx_lowlevel_pka_mod_sub(B, A, E);
    
    //compute F = D - C
    cc3xx_lowlevel_pka_mod_sub(D, C, F);
    //compute G = D + C
    cc3xx_lowlevel_pka_mod_add(D, C, G);
    
    //compute H = B + A
    cc3xx_lowlevel_pka_mod_add(B,A,H);
    
    //compute X3 = E F
    cc3xx_lowlevel_pka_mod_mul(E, F, res->x);

    //compute Y3 = G H
    cc3xx_lowlevel_pka_mod_mul(G, H, res->y);

    //compute T3 = E H
    cc3xx_lowlevel_pka_mod_mul(E, H, res->t);

    //compute Z3 = F G
    cc3xx_lowlevel_pka_mod_mul(F, G, res->z);

    //free_addition_registers();

    /*
    debug_read_and_print_reg(res->x, "X: ");
    debug_read_and_print_reg(res->y, "Y: ");
    debug_read_and_print_reg(res->z, "Z: ");
    debug_read_and_print_reg(res->t, "T: ");
    */

    return CC3XX_ERR_SUCCESS;
}

cc3xx_err_t cc3xx_lowlevel_ec_edwards_add_points(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_affine *p, cc3xx_ec_point_affine *q, cc3xx_ec_point_affine *res){
    
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //I think i need to start with making points projective
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_ec_point_extended q_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();

    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);
    cc3xx_lowlevel_ec_affine_to_extended(curve, q, &q_ext);
    
    //affine to ext sets to order, resetting to field here
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    allocate_addition_registers();
    cc3xx_lowlevel_pka_unmap_physical_registers(); //seems odd but some operations individuall allocate registers, thus free all before starting
    //Then, I can add them using Explicit formulas database: add-2008-hwcd-3
    cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &p_ext, &q_ext, &res_ext);
    free_addition_registers();

    //Then, I can write them to q
    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&q_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);

    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    

    return 0;
}

void cc3xx_lowlevel_ec_edwards_double_extended_points(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_extended *p, cc3xx_ec_point_extended *res)
{
    
    //hyperelliptic.com -> EFD -> mdbl-2008-hwcd, assumes Z=1. 
    //if that is not the case 
    //use the dbl-2008-hwcd formula, same source, doesn't assume Z=1
    bool is_z_one = false;
    is_z_one = cc3xx_lowlevel_pka_are_equal_si(p->z, 0x1);

    //ensure that there is space
    //cc3xx_lowlevel_pka_unmap_physical_registers();

    //allocate_addition_registers();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    //A = X1^2
    cc3xx_lowlevel_pka_mod_exp_si(p->x, 0x2, A);
    
    //B = Y1^2
    cc3xx_lowlevel_pka_mod_exp_si(p->y, 0x2, B);
    
    if(!is_z_one){
        //C = 2*Z1^2
        cc3xx_lowlevel_pka_mod_exp_si(p->z, 0x2, C);
        cc3xx_lowlevel_pka_mod_mul_si(C, 0x2, C);
    }

    //D = a*A, a = -1 should be the same as field modulus - A
    //however, this hardcodes this to a curve with a=-1
    cc3xx_lowlevel_pka_mod_neg(A, D);

    //E = (X1+Y1)^2-A-B
    cc3xx_lowlevel_pka_mod_add(p->x, p->y, E);
    cc3xx_lowlevel_pka_mod_exp_si(E, 0x2, E);
    cc3xx_lowlevel_pka_mod_sub(E, A,E);
    cc3xx_lowlevel_pka_mod_sub(E, B,E);
    //G = D+B
    cc3xx_lowlevel_pka_mod_add(D, B, G);

    if(!is_z_one){
        //F = G-C
        cc3xx_lowlevel_pka_mod_sub(G, C, F);
    }
    
    //H = D-B
    cc3xx_lowlevel_pka_mod_sub(D, B, H);
    
    if(is_z_one){
        //X3 = E*(G-2)
        cc3xx_lowlevel_pka_sub_si(G, 0x2, res->x);
        cc3xx_lowlevel_pka_mod_mul(E, res->x, res->x);
    }else{
        //X3 = E*F
        cc3xx_lowlevel_pka_mod_mul(E, F, res->x);
    }
    
    //Y3 = G*H
    cc3xx_lowlevel_pka_mod_mul(G, H, res->y);

    //T3 = E*H
    cc3xx_lowlevel_pka_mod_mul(E, H, res->t);
    
    if(is_z_one){
        //Z3 = G^2-2*G
        cc3xx_lowlevel_pka_mod_exp_si(G, 0x2, res->z);
        cc3xx_lowlevel_pka_mod_mul_si(G, 0x2, G); //store 2G in G
        cc3xx_lowlevel_pka_mod_sub(res->z, G, res->z);
    }else{
        //Z3 = F*G
        cc3xx_lowlevel_pka_mod_mul(F, G, res->z);
    }

    //free_addition_registers();

}

cc3xx_err_t cc3xx_lowlevel_ec_edwards_double_point(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_affine *p, cc3xx_ec_point_affine *res)
{

    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();

    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);

    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    allocate_addition_registers();
    cc3xx_lowlevel_pka_unmap_physical_registers(); //seems odd but some operations individuall allocate registers, thus free all before starting
    cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &p_ext, &res_ext);
    free_addition_registers();

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);

    return CC3XX_ERR_SUCCESS;
    

}


cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult_slow(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{
    //erstmal die simple variante s mal
    /*
    sP = Q
    Q = P +P +P + P + ... + P + P +P
    res = 0
    also
    while s > 0:
      res = res + P
      s--

    */
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    
    //given point to extended
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);
    
    //mach mal ein allocate neutral point für diesen Fall
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();
    
    //scalar to register
    //must be done modulo l
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);

    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    //s times add P to 
    while( ! cc3xx_lowlevel_pka_are_equal_si(s, 0x0)){ //while s > 0
        cc3xx_lowlevel_pka_sub_si(s, 0x1, s);
        cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p_ext, &res_ext); //res = res + p
    }

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);
    cc3xx_lowlevel_pka_free_reg(s);



    return 0;
}


cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult_double_and_add(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{

    //double and add
    //unfortunately this one is not side-channel resistant
    //you get a different runtime for a 1 then for a 0
    //measuring that let's you measure the scalar which might to be a secret key
    /*
    res = 0
    adder = p
    bits = bits_in_scalar_beginning_with_least_significant_bit(scalar)
    for bit in bits:
      if bit == 1:
        res += adder
      double(adder)
    return res

    */

    //scalar to register
    //must be done modulo l
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    
    cc3xx_lowlevel_pka_reduce(s);

    if(cc3xx_lowlevel_pka_are_equal_si(s, 0x1)){ //1*P = P
        cc3xx_lowlevel_pka_copy(p->x, res->x);
        cc3xx_lowlevel_pka_copy(p->y, res->y);
        
        cc3xx_lowlevel_pka_free_reg(s);
        return CC3XX_ERR_SUCCESS;   
    }

    //TODO: if s == 0 % l return O ?


    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);    

    //given point to extended
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);

    //mach mal ein allocate neutral point für diesen Fall
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();

    //adder
    cc3xx_ec_point_extended adder_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(&p_ext, &adder_ext);
    
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    
    //s times add P to 
    //for bit in scalar
    uint32_t i = 0;
    uint32_t bit = 0x0;
    //this allows a measurement of how long the scalar is - will terminate earlier if scalar is short 
    while(!cc3xx_lowlevel_pka_are_equal_si(s, 0x0)){//while s > 0
        //printf("Start Runde %ld\n", i);
        bit = cc3xx_lowlevel_pka_test_bits_ui(s, 0, 1);
        
        cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 0x1, s); //s--
        
        if(bit == 0x1){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder_ext, &res_ext);
        }
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &adder_ext, &adder_ext);
        //printf("Ende Runde %ld\n", i);
        i++;
    }
    //printf("Ende Runde for loop\n");
    

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&adder_ext);
    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);
    cc3xx_lowlevel_pka_free_reg(s);

    return 0;
}
//daa-approach
cc3xx_err_t _cc3xx_lowlevel_ec_edwards_scalar_mult(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{

    //double and add with unconditional add as seen in c25519 library 
    //double and add always - daa
    //this is sidechannel resistant in so far as that it will take the same time for bit 0 and 1
    //it also constantly does 255 operations
    //TODO: seek some sources before claiming that - guess it is obvious given the while loop
    /*
    res = 0
    adder = p
    tmp;
    bits = bits_in_scalar_beginning_with_least_significant_bit(scalar)
    for bit in bits:
      if bit == 1:
        res += adder
      else:
        tmp = res + adder //addition to hide bit decision
      double(adder)
    return res

    */
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //scalar to register
    //must be done modulo l
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    
    cc3xx_lowlevel_pka_reduce(s);

    /*this would mean a way quicker calculation of s = 1 - as unlikely as that is*/
    /*if(cc3xx_lowlevel_pka_are_equal_si(s, 0x1)){ //1*P = P
        cc3xx_lowlevel_pka_copy(p->x, res->x);
        cc3xx_lowlevel_pka_copy(p->y, res->y);
        
        cc3xx_lowlevel_pka_free_reg(s);
        return CC3XX_ERR_SUCCESS;   
    }*/

    //TODO: if s == 0 % l return O ?


    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);    

    //given point to extended
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);

    //mach mal ein allocate neutral point für diesen Fall
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();
    
    //adder
    cc3xx_ec_point_extended adder_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(&p_ext, &adder_ext);
    
    //tmp point for uncondition addition
    cc3xx_ec_point_extended tmp = cc3xx_lowlevel_ec_allocate_extended_point();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_unmap_physical_registers();
    allocate_addition_registers();
    cc3xx_lowlevel_pka_unmap_physical_registers();
    uint32_t amount_bits = curve->modulus_size * 8;
    uint32_t i = 0;
    uint32_t bit = 0x0;
    while(i < amount_bits){ 
        bit = cc3xx_lowlevel_pka_test_bits_ui(s, 0, 1);
        
        cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 0x1, s); //s--
        
        //always do an addition
        //that shall avoid time based measurement of 0 or 1 
        //likely also energy based - assuming that one operation here takes a measureable amount of energy?
        if(bit == 0x1){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder_ext, &res_ext);
        }else{
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder_ext, &tmp);
        }
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &adder_ext, &adder_ext);
        i++;
    }
    //printf("Ende Runde for loop\n");
    

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    free_addition_registers();
    cc3xx_lowlevel_ec_free_extended_point(&tmp);
    cc3xx_lowlevel_ec_free_extended_point(&adder_ext);
    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);
    cc3xx_lowlevel_pka_free_reg(s);

    return 0;
}


cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult_generator(cc3xx_ec_curve_t *curve,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{
    return(cc3xx_lowlevel_ec_edwards_scalar_mult(curve, &curve->generator, scalar, res));
}



cc3xx_err_t cc3xx_lowlevel_ec_edwards_mult_and_add_slower(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p1,
                                                     cc3xx_ec_point_affine *p2,
                                                     uint32_t *scalar_a,
                                                     uint32_t *scalar_b,
                                                     cc3xx_ec_point_affine *res)
{

    //calculate a*p1 + b*p2 in one go
    /*
    res = 0
    adder = p
    tmp;
    bits = bits_in_scalar_beginning_with_least_significant_bit(scalar)
    for bit in bits:
      if bit == 1:
        res += adder
      else:
        tmp = res + adder //addition to hide bit decision
      double(adder)
    return res

    */
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //scalar to register
    //must be done modulo l
    cc3xx_pka_reg_id_t a = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t b = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(a, scalar_a, 32);
    cc3xx_lowlevel_pka_write_reg(b, scalar_b, 32);
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    
    cc3xx_lowlevel_pka_reduce(a);
    cc3xx_lowlevel_pka_reduce(b);
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);    

    //given points to extended
    cc3xx_ec_point_extended p1_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p1, &p1_ext);
    cc3xx_ec_point_extended p2_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p2, &p2_ext);

    //mach mal ein allocate neutral point für diesen Fall
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //adder
    cc3xx_ec_point_extended adder1_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(&p1_ext, &adder1_ext);
    cc3xx_ec_point_extended adder2_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(&p2_ext, &adder2_ext);

    //tmp point for uncondition addition
    cc3xx_ec_point_extended tmp = cc3xx_lowlevel_ec_allocate_extended_point();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_unmap_physical_registers();
    uint32_t amount_bits = curve->modulus_size * 8;
    uint32_t i = 0;
    uint32_t bit1 = 0x0;
    uint32_t bit2 = 0x0;
    while(i < amount_bits){ 
        bit1 = cc3xx_lowlevel_pka_test_bits_ui(a, 0, 1);
        bit2 = cc3xx_lowlevel_pka_test_bits_ui(b, 0, 1);
        
        cc3xx_lowlevel_pka_shift_right_fill_0_ui(a, 0x1, a);
        cc3xx_lowlevel_pka_shift_right_fill_0_ui(b, 0x1, b);
        
        //always do an addition
        //that shall avoid time based measurement of 0 or 1 
        //likely also energy based - assuming that one operation here takes a measureable amount of energy?
        if(bit1 == 0x1){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder1_ext, &res_ext);
        }else{
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder1_ext, &tmp);
        }
        if(bit2 == 0x1){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder2_ext, &res_ext);
        }else{
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &adder2_ext, &tmp);
        }
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &adder1_ext, &adder1_ext);
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &adder2_ext, &adder2_ext);
        i++;
    }
    //printf("Ende Runde for loop\n");
    

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&tmp);
    cc3xx_lowlevel_ec_free_extended_point(&adder2_ext);
    cc3xx_lowlevel_ec_free_extended_point(&adder1_ext);
    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p2_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p1_ext);
    cc3xx_lowlevel_pka_free_reg(b);
    cc3xx_lowlevel_pka_free_reg(a);

    return 0;
}
//daa-approach
cc3xx_err_t _cc3xx_lowlevel_ec_edwards_mult_and_add(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p1,
                                                     cc3xx_ec_point_affine *p2,
                                                     uint32_t *scalar_a,
                                                     uint32_t *scalar_b,
                                                     cc3xx_ec_point_affine *res)
{

    //calculate a*p1 + b*p2 in one go
    /*
    res = 0
    tmp;
    for i in 255 downto 0: //from msb to lsb
        res = res + res
        if s1[i] == 1: res = res + p1
        if s2[i] == 1: res = res + p2
    return res

    */
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //scalar to register
    //must be done modulo l - missing here todo
    cc3xx_pka_reg_id_t a = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t b = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg_swap_endian(a, scalar_a, 32);
    cc3xx_lowlevel_pka_write_reg_swap_endian(b, scalar_b, 32);
    
    //given points to extended
    cc3xx_ec_point_extended p1_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p1, &p1_ext);
    cc3xx_ec_point_extended p2_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p2, &p2_ext);

    //mach mal ein allocate neutral point für diesen Fall
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();
    cc3xx_lowlevel_pka_unmap_physical_registers();

    //tmp point for uncondition addition
    cc3xx_ec_point_extended tmp = cc3xx_lowlevel_ec_allocate_extended_point();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_unmap_physical_registers();
    allocate_addition_registers();
    cc3xx_lowlevel_pka_unmap_physical_registers();
    uint32_t bit1 = 0x0;
    uint32_t bit2 = 0x0;
    //32 bytes
    for(int i= 0; i<32;i++){
        uint32_t bytestart = i*8;
        //with 8 bit each that we want to start reading from behind to get the highest first
        for(int j = 7; j >= 0; j--){
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &res_ext, &res_ext);

        bit1 = cc3xx_lowlevel_pka_test_bits_ui(a, bytestart+j, 1);
        bit2 = cc3xx_lowlevel_pka_test_bits_ui(b, bytestart+j, 1);
        
        //always do two additions
        if(bit1 == 0x1){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p1_ext, &res_ext);
        }else{
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p1_ext, &tmp);
        }
        if(bit2 == 0x1){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p2_ext, &res_ext);
        }else{
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p2_ext, &tmp);
        }        
        }
    }
    //printf("Ende Runde for loop\n");
    

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);
    free_addition_registers();
    cc3xx_lowlevel_ec_free_extended_point(&tmp);
    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p2_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p1_ext);
    cc3xx_lowlevel_pka_free_reg(b);
    cc3xx_lowlevel_pka_free_reg(a);

    return 0;
}


void calculate_table(cc3xx_ec_curve_t *curve, cc3xx_ec_point_extended *p, cc3xx_ec_point_extended_data *table){

    //check if p is generator point, if so return pre-generated table
    if( cc3xx_lowlevel_pka_are_equal(curve->generator.x, p->x) && cc3xx_lowlevel_pka_are_equal(curve->generator.y, p->y)) {
        //printf("Generator point, ommitting table generation\n");
        memcpy(table, table_g, 16*32*4); // 2^window_size * bytesize_coordinate * coordinate_count_extended_point
        return;
    }
    
    int table_len = 16; //hardcoded for 4 bit window
    memcpy(&table[0], &edwards_extended_neutral_element_data, sizeof(cc3xx_ec_point_extended_data)); //table[0] = 0
    cc3xx_lowlevel_ec_extended_point_to_data(&table[1], p); //table[1] = P

    //get a point to do calculation with
    cc3xx_ec_point_extended tmp = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(p, &tmp);
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false,CC3XX_PKA_REG_NP);
    for(int i=2; i<table_len; i++){
        //tmp = tmp + p
        cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &tmp, p, &tmp);
        //table[i] = tmp
        cc3xx_lowlevel_ec_extended_point_to_data(&table[i], &tmp);
    }
    cc3xx_lowlevel_ec_free_extended_point(&tmp);

}


//4-bit-window-variant
cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{

    /*
    4 bit window method
    - build a table that contains the point to mult from 0*p till 15*p (2^4-1 = 15)
    - handle the scalar in 4 bit chunks: interpret 4 bit as one value (0-15)
        - per 4 bit double the result 4 times, add value*point, obtain that from table
        - potentially create that vector first
        - e.g. scalar 0b11101100 = 0xec -> 000e 000c
    R = 0
    for i in 255 till 0, where 255 is the most significant bit of the scalar s
      R = R + R
      if i % 4 == 0:
        R = R + table[s[i]]
    */
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //scalar to register
    //must be done modulo l
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    //printf("Wrote scalar\n");
    //given point to extended
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);

    allocate_addition_registers(); //table does additions
    cc3xx_lowlevel_pka_unmap_physical_registers(); //seems odd but some operations individuall allocate registers, thus free all before starting
    cc3xx_ec_point_extended_data table[16];
    calculate_table(curve, &p_ext, table);
    
    //there is no use for p_ext from here on, use as adder below

    //R
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    uint32_t amount_bits = curve->modulus_size * 8;
    int i = amount_bits-1;
    uint32_t halfbyte = 0x0;
    (void) halfbyte; //compiler whines that it ain't used
    while(i >= 0){ 
        //R = R + R
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &res_ext, &res_ext);
        
        if(i % 4 == 0){ //every fourth bit
            //take the most significant four bit as value
            halfbyte = cc3xx_lowlevel_pka_test_bits_ui(s, i, 4);
            //printf("i: %d, halfbyte: %lx \n",i, halfbyte);
            
            //read the appropriate point from the table
            cc3xx_lowlevel_ec_extended_point_from_data(&table[halfbyte], &p_ext);
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p_ext, &res_ext);
        }
        //debug_read_and_print_reg(res_ext.x, "X: ");
        i--;
    }

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    free_addition_registers();
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);
    cc3xx_lowlevel_pka_free_reg(s);

    return 0;
}

//4-bit-window-variant
cc3xx_err_t cc3xx_lowlevel_ec_edwards_mult_and_add(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p1,
                                                     cc3xx_ec_point_affine *p2,
                                                     uint32_t *scalar_a,
                                                     uint32_t *scalar_b,
                                                     cc3xx_ec_point_affine *res)
{

    //calculate a*p1 + b*p2 in one go using 4bit window method for mult
    /*
    calc table p1
    calc table p2
    res = 0
    tmp;
    for i in 255 downto 0: //from msb to lsb
        res = res + res
        if(i%4 == 0)
            if s1[i] == 1: res = res + table[p1]
            if s2[i] == 1: res = res + table[p2]
    return res

    */
    cc3xx_lowlevel_pka_unmap_physical_registers();
    //scalar to register
    //must be done modulo l - missing here todo
    cc3xx_pka_reg_id_t a = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t b = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(a, scalar_a, 32);
    cc3xx_lowlevel_pka_write_reg(b, scalar_b, 32);
    
    //given points to extended
    cc3xx_ec_point_extended p1_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p1, &p1_ext);
    
    allocate_addition_registers(); //table build needs addition registers, do this here to be able to free p2 after table build
    cc3xx_lowlevel_pka_unmap_physical_registers(); //seems odd but some operations individually allocate registers, thus free all before starting
    
    cc3xx_ec_point_extended p2_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p2, &p2_ext);

    
    cc3xx_ec_point_extended_data table_p1[16];
    cc3xx_ec_point_extended_data table_p2[16];
    calculate_table(curve, &p1_ext, table_p1);
    calculate_table(curve, &p2_ext, table_p2);

    //only keep p1 as adder below
    cc3xx_lowlevel_ec_free_extended_point(&p2_ext);

    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_neutral_point();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_unmap_physical_registers();

    uint32_t amount_bits = curve->modulus_size * 8;
    int i = amount_bits-1;
    uint32_t halfbyte = 0x0;
    (void) halfbyte; //compiler whines that it ain't used

    while(i >= 0){ 
        //R = R + R
        cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &res_ext, &res_ext);
        if(i % 4 == 0){ //every fourth bit
            //R = R + table_p1
            halfbyte = cc3xx_lowlevel_pka_test_bits_ui(a, i, 4);
            cc3xx_lowlevel_ec_extended_point_from_data(&table_p1[halfbyte], &p1_ext);
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p1_ext, &res_ext);

            //R = R + table_p2
            halfbyte = cc3xx_lowlevel_pka_test_bits_ui(b, i, 4);
            cc3xx_lowlevel_ec_extended_point_from_data(&table_p2[halfbyte], &p1_ext);
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res_ext, &p1_ext, &res_ext);
        }
        i--;
    }
    

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res_ext, res);

    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    free_addition_registers();
    cc3xx_lowlevel_ec_free_extended_point(&p1_ext);
    cc3xx_lowlevel_pka_free_reg(b);
    cc3xx_lowlevel_pka_free_reg(a);

    return 0;
}

/*
cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{

    //montgommery_ladder
    //constant operation time
    //but I guess it only works with montgommery curves and matching addition laws
    //res0 = 0
    //res1 = P
    //bits = bits_in_scalar_beginning_with_least_significant_bit(scalar)
    //for bit in bits:
    //  if bit == 0:
    //    res1 = res0 + res1
    //    res0 = res0*2
    //  else:
    //    res0 = res0 + res1
    //    res1 = res1 * 2
    //return res0

    

    //scalar to register
    //must be done modulo l
    cc3xx_pka_reg_id_t s = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_write_reg(s, scalar, 32);
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    
    cc3xx_lowlevel_pka_reduce(s);

    //doing this makes the *1 visible as it will take way less operations then 
    //other scalar multiplications
    
    if(cc3xx_lowlevel_pka_are_equal_si(s, 0x1)){ //1*P = P
        cc3xx_lowlevel_pka_copy(p->x, res->x);
        cc3xx_lowlevel_pka_copy(p->y, res->y);
        
        cc3xx_lowlevel_pka_free_reg(s);
        return CC3XX_ERR_SUCCESS;   
    }
    
    //TODO: if s == 0 % l return O ?


    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);    

    //given point to extended
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();    
    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);

    //mach mal ein allocate neutral point für diesen Fall
    cc3xx_ec_point_extended res0 = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_pka_clear(res0.x);
    cc3xx_lowlevel_pka_clear(res0.y);
    cc3xx_lowlevel_pka_clear(res0.z);
    cc3xx_lowlevel_pka_clear(res0.t);
    cc3xx_lowlevel_pka_add_si(res0.y, 0x1, res0.y);
    cc3xx_lowlevel_pka_add_si(res0.z, 0x1, res0.z);

    //adder
    cc3xx_ec_point_extended res1 = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(&p_ext, &res1);
    
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    

    uint8_t bit = 0x0;
    size_t bitlen = curve->modulus_size * 8;
    printf("bitlen: %d\n", bitlen);
    size_t bitpos = bitlen;
    char bits_as_seen_by_algo[bitlen+1];
    bits_as_seen_by_algo[bitlen] = '\0';
    uint32_t round = 0;
    while(bitpos > 0){//while s > 0
        //get least significant bit
        bit = cc3xx_lowlevel_pka_test_bits_ui(s, 0, 1);
        //shift right by one to get next bit next round
        cc3xx_lowlevel_pka_shift_right_fill_0_ui(s, 0x1, s); //s--
        //debug
        sprintf(&bits_as_seen_by_algo[round], "%d", bit & 0x1);
        printf("Round: %ld, Bit: %d, bitpos: %d\n", round, bit, bitpos);
        //reduce bitpos to eventually stop the loop
        bitpos--;
        
        if(bit == 0x0){
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res0, &res1, &res1);
            cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &res0, &res0);
        }else{
            cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &res0, &res1, &res0);
            cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &res1, &res1);
        }
        round++;
    }

    printf("Rounds done: %ld \nBits seen:%s\n", round, bits_as_seen_by_algo);
    

    cc3xx_lowlevel_ec_extended_to_affine(curve, &res0, res);

    cc3xx_lowlevel_ec_free_extended_point(&res1);
    cc3xx_lowlevel_ec_free_extended_point(&res0);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);
    cc3xx_lowlevel_pka_free_reg(s);

    return 0;
}
*/
