#include "cc3xx_ec_edwards.h"
#include "cc3xx_aes.h"
#include "cc3xx_ec.h"
#include "cc3xx_ec_edw_extended_point.h"
#include "cc3xx_error.h"
#include "cc3xx_pka.h"
#include <stddef.h>
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

    cc3xx_pka_reg_id_t A = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t B = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t C = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t D = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t E = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t F = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t G = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t H = cc3xx_lowlevel_pka_allocate_reg();

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

    cc3xx_lowlevel_pka_free_reg(H);
    cc3xx_lowlevel_pka_free_reg(G);
    cc3xx_lowlevel_pka_free_reg(F);
    cc3xx_lowlevel_pka_free_reg(E);
    cc3xx_lowlevel_pka_free_reg(D);
    cc3xx_lowlevel_pka_free_reg(C);
    cc3xx_lowlevel_pka_free_reg(B);
    cc3xx_lowlevel_pka_free_reg(A);

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
    
    //I think i need to start with making points projective
    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_ec_point_extended q_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();

    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);
    cc3xx_lowlevel_ec_affine_to_extended(curve, q, &q_ext);
    
    //affine to ext sets to order, resetting to field here
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

    //Then, I can add them using Explicit formulas database: add-2008-hwcd-3
    cc3xx_lowlevel_ec_edwards_add_extended_points(curve, &p_ext, &q_ext, &res_ext);

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
    
    bool is_z_one = false;
    is_z_one = cc3xx_lowlevel_pka_are_equal_si(p->z, 0x1);

    //ensure that there is space
    cc3xx_lowlevel_pka_unmap_physical_registers();

    cc3xx_pka_reg_id_t A = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t B = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t C; //only allocate if needed
    cc3xx_pka_reg_id_t D = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t E = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t F; //only allocate if needed
    cc3xx_pka_reg_id_t G = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_pka_reg_id_t H = cc3xx_lowlevel_pka_allocate_reg();
    
    //hyperelliptic.com -> EFD -> mdbl-2008-hwcd, assumes Z=1. 
    //if that is not the case 
    //use the dbl-2008-hwcd formula, same source, doesn't assume Z=1
    if(!is_z_one){
         C = cc3xx_lowlevel_pka_allocate_reg();
         F = cc3xx_lowlevel_pka_allocate_reg();
    }
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

    if(!is_z_one){
         cc3xx_lowlevel_pka_free_reg(F);
         cc3xx_lowlevel_pka_free_reg(C);
    }
    cc3xx_lowlevel_pka_free_reg(H);
    cc3xx_lowlevel_pka_free_reg(G);
    cc3xx_lowlevel_pka_free_reg(E);
    cc3xx_lowlevel_pka_free_reg(D);
    cc3xx_lowlevel_pka_free_reg(B);
    cc3xx_lowlevel_pka_free_reg(A);

}

cc3xx_err_t cc3xx_lowlevel_ec_edwards_double_point(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_affine *p, cc3xx_ec_point_affine *res)
{

    cc3xx_ec_point_extended p_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();

    cc3xx_lowlevel_ec_affine_to_extended(curve, p, &p_ext);

    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_ec_edwards_double_extended_points(curve, &p_ext, &res_ext);

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
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_pka_clear(res_ext.x);
    cc3xx_lowlevel_pka_clear(res_ext.y);
    cc3xx_lowlevel_pka_clear(res_ext.z);
    cc3xx_lowlevel_pka_clear(res_ext.t);
    cc3xx_lowlevel_pka_add_si(res_ext.y, 0x1, res_ext.y);
    cc3xx_lowlevel_pka_add_si(res_ext.z, 0x1, res_ext.z);
    
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
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_pka_clear(res_ext.x);
    cc3xx_lowlevel_pka_clear(res_ext.y);
    cc3xx_lowlevel_pka_clear(res_ext.z);
    cc3xx_lowlevel_pka_clear(res_ext.t);
    cc3xx_lowlevel_pka_add_si(res_ext.y, 0x1, res_ext.y);
    cc3xx_lowlevel_pka_add_si(res_ext.z, 0x1, res_ext.z);

    
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

cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res)
{

    //double and add with unconditional add as seen in c25519 library 
    //don't know a proper name - constant time double and add?
    //this is sidechannel resistant in so far as that it will take the same time for bit 0 and 1
    //it also constantly does 255 operations
    //TODO: seek some sources before claiming that...
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
    cc3xx_ec_point_extended res_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_pka_clear(res_ext.x);
    cc3xx_lowlevel_pka_clear(res_ext.y);
    cc3xx_lowlevel_pka_clear(res_ext.z);
    cc3xx_lowlevel_pka_clear(res_ext.t);
    cc3xx_lowlevel_pka_add_si(res_ext.y, 0x1, res_ext.y);
    cc3xx_lowlevel_pka_add_si(res_ext.z, 0x1, res_ext.z);

    
    //adder
    cc3xx_ec_point_extended adder_ext = cc3xx_lowlevel_ec_allocate_extended_point();
    cc3xx_lowlevel_ec_copy_extended_point(&p_ext, &adder_ext);
    
    //tmp point for uncondition addition
    cc3xx_ec_point_extended tmp = cc3xx_lowlevel_ec_allocate_extended_point();
    
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    
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

    cc3xx_lowlevel_ec_free_extended_point(&tmp);
    cc3xx_lowlevel_ec_free_extended_point(&adder_ext);
    cc3xx_lowlevel_ec_free_extended_point(&res_ext);
    cc3xx_lowlevel_ec_free_extended_point(&p_ext);
    cc3xx_lowlevel_pka_free_reg(s);

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
