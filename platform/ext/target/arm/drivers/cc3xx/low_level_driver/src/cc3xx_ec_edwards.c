#include "cc3xx_ec_edwards.h"
#include "cc3xx_aes.h"
#include "cc3xx_ec.h"
#include "cc3xx_pka.h"
#include <stdint.h>

//debug
#include <stdio.h>

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
void cc3xx_lowlevel_ec_edw_decompress_point_pka(cc3xx_ec_point_affine *decompressed_pt, 
            uint32_t isOddX, cc3xx_ec_curve_t *curve){
        // decompress: (YP) -> (XP,YP,ZP=1,TP) 
        // tw. edw curve= ax^2 + y^2 = 1 + dx^2y^2 ==> x = sqrt(1-y^2 / 1-dy^2)

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
        


        //last part of RFC decode procedure
        bit0 = cc3xx_lowlevel_pka_test_bits_ui(reg_x, 0, 1); //read bit[0]
        
        //TODO: if calculated_x == 0 but isOddX = 1
        //then we're in an obvious problematic situation and decoding should fail

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

}

void cc3xx_lowlevel_ec_edw_decompress_point(uint32_t *compressed, 
                    cc3xx_ec_curve_t *curve, cc3xx_ec_point_affine *decompressed){
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
    cc3xx_lowlevel_ec_edw_decompress_point_pka(decompressed, 
        isOddX, curve);

    //setzt N auf curve oder
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);    
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
    
    debug_read_and_print_reg(rhs, "RHS: ");
    debug_read_and_print_reg(lhs, "LHS: ");

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
