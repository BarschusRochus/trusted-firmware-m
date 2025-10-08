#include "cc3xx_ec_edwards.h"
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

    cc3xx_lowlevel_pka_read_reg(reg, debug, debug_len);
    print_debug(label, debug, debug_bytes);
}


void cc3xx_lowlevel_ec_edw_decompress_point(cc3xx_pka_reg_id_t reg_y, uint32_t isOddX, 
        cc3xx_ec_point_affine *decompressed_pt, cc3xx_ec_curve_t *curve){
        // decompress: (YP) -> (XP,YP,ZP=1,TP) 
        // tw. edw curve= ax^2 + y^2 = 1 + dx^2y^2 ==> x = sqrt(1-y^2 / 1-dy^2)

        uint32_t bit0;

        uint32_t debug[8] = {0};
        size_t debug_len = 32;
        size_t debug_bytes = 8;

        cc3xx_pka_reg_id_t reg_x = decompressed_pt->x;
        cc3xx_pka_reg_id_t t = cc3xx_lowlevel_pka_allocate_reg();
        cc3xx_pka_reg_id_t t3 = cc3xx_lowlevel_pka_allocate_reg();
        cc3xx_pka_reg_id_t t4 = cc3xx_lowlevel_pka_allocate_reg();
        cc3xx_pka_reg_id_t t5 = cc3xx_lowlevel_pka_allocate_reg();
        
        //edwards curve 25519 plumbing (d, sqrt_m1, q58) via curve
        //setzt N auf 2^255-19
        cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);

        cc3xx_lowlevel_pka_mod_mul(reg_y, reg_y, t3); //res = (r0 * r1) mod N.        //y^2 ohne reduce gibt im zweifel ein 64 byte resultat 
        cc3xx_lowlevel_pka_read_reg(t3, debug, debug_len);
        print_debug("t3: ",debug, debug_bytes);
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, EDW_REG_T3, rY, rY);                 // hwmmul(t3, y, y, n, np);  //t3 = y^2 mod n?
        cc3xx_lowlevel_pka_mod_mul(t3, curve->param_d, t4);                                                                             
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, EDW_REG_T4, EDW_REG_T3, EDW_REG_D);  // hwmmul(t4, t3, ec_d, n, np); //t4 = y^2*d mod n
        
        cc3xx_lowlevel_pka_sub_si(t3, 1, t3); //u
        //PKA_SUB_IM(LEN_ID_N_PKA_REG_BITS, EDW_REG_T3, EDW_REG_T3, 1);       // hwdec(t3, t3); //t3 = t3 - 1 = y^2 -1?
        cc3xx_lowlevel_pka_add_si(t4, 1, t4); //v
        //PKA_ADD_IM(LEN_ID_N_PKA_REG_BITS, EDW_REG_T4, EDW_REG_T4, 1);       // hwinc(t4, t4); //t4 = t4 + 1 = y^2*d + 1
        
        cc3xx_lowlevel_pka_mod_mul(t4, t4, t); //t=v^2
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, EDW_REG_T, EDW_REG_T4, EDW_REG_T4);  // hwmmul(t, t4, t4, n, np); // t = t4 * t4 = y^2*d + 1 * y^2*d + 1
        cc3xx_lowlevel_pka_mod_mul(t4, t, t); //t=v^3
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, EDW_REG_T, EDW_REG_T4, EDW_REG_T);   // hwmmul(t, t4, t, n, np); // t = t4 * t = y^2*d + 1 * y^2*d + 1 * y^2*d + 1 
        cc3xx_lowlevel_pka_mod_mul(t, t, reg_x); //x = v^6
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, rX, EDW_REG_T, EDW_REG_T);           // hwmmul(x, t, t, n, np); // x = t * t = y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1
        cc3xx_lowlevel_pka_mod_mul(t4, reg_x, reg_x); //x = v^6 * v = v^7
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, rX, rX, EDW_REG_T4);                 // hwmmul(x, x, t4, n, np); // x = x * t4 = y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1 * y^2*d + 1
        cc3xx_lowlevel_pka_mod_mul(t3, reg_x, t5); //t5 = u*v^7
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, EDW_REG_T5, rX, EDW_REG_T3);         // hwmmul(t5, x, t3, n, np); // t5 = x * t3 = (y^2*d + 1)^7 * (y^2-1)

        //selbes spiel wie mit d -> q58 bei init vordefinieren oder hier ad hoc setzen
        //geht ohne mod? muss ohne mod?
        cc3xx_lowlevel_pka_mod_exp(t5, curve->q58, reg_x); // x = (u*v^7)^((p-5)/8)
        //PKA_MOD_EXP(LEN_ID_N_BITS, rX, EDW_REG_T5, EDW_REG_Q58);            // hwmexp(x, t5, q58, n, np); // x = t5^reg_q58 // Wurzel ziehen in Magic? q58 = = (P - 5)/8 // ja, genau
        cc3xx_lowlevel_pka_mod_mul(t3, reg_x, reg_x); //x = (u*v^7)^((p-5)/8) * u
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, rX, rX, EDW_REG_T3);                 // hwmmul(x, x, t3, n, np); // x = x * t3 = 
        
        cc3xx_lowlevel_pka_mod_mul(t, reg_x, reg_x); // x = (u*v^7)^((p-5)/8) * uv^3
        //cc3xx_lowlevel_pka_read_reg(reg_x, debug, debug_len);
        //print_debug("x: ",debug, debug_bytes);
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, rX, rX, EDW_REG_T);                  // hwmmul(x, x, t, n,np); // x = x * t
        
        cc3xx_lowlevel_pka_mod_mul(reg_x, reg_x, t); // t = ((u*v^7)^((p-5)/8) * uv^3)^2 = x^2
        //cc3xx_lowlevel_pka_read_reg(t, debug, debug_len);
        //print_debug("t = x*x: ",debug, debug_bytes);
        //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, EDW_REG_T, rX, rX);                  // hwmmul(t, x, x, n,np); // t = x * x
        
        //mul acc haben wir nicht, daher erst mul, dann acc um t = t4 * t + t3 zu erreichen
        cc3xx_lowlevel_pka_mod_mul(t4, t, t); // t = t4 * t <=> t = x^2 * v
        //cc3xx_lowlevel_pka_read_reg(t, debug, debug_len);
        //print_debug("t = t4 + t: ",debug, debug_bytes);

        cc3xx_lowlevel_pka_add(t3, t, t); // t = t + t3 <=> t = x^2 * v + u
        //cc3xx_lowlevel_pka_read_reg(t, debug, debug_len);
        //print_debug("t = t3 + t: ",debug, debug_bytes);
        //PKA_MOD_MUL_ACC(LEN_ID_N_BITS, EDW_REG_T, EDW_REG_T4, EDW_REG_T, EDW_REG_T3); // hwmlap(t,t4, t, t3, n, np, 0); //t = t4 * t + t3

        
        cc3xx_lowlevel_pka_div(t, curve->field_modulus, t4, t); //t4 = t / N, t = t mod N <=> t4 = (x^2 * v + u) / N, t = (x^2 * v + u) mod N
        printf("equal to python impl. till here\n");
        // Divide:  Res =  OpA / OpB , OpA = OpA mod OpB - division,  #define   PKA_DIV(lenId, Res, OpA, OpB) //ahhh, also landet in opA der remainder
        //PKA_DIV(LEN_ID_N_PKA_REG_BITS, EDW_REG_T4, EDW_REG_T, EDW_REG_N); //t4 = t / n where n is the modulus, so either 2^255-19 or the curve order...


        //#define PKA_COMPARE_IM_STATUS(lenId, a, b,stat)
        
        //  CompareImmediate:  OpA ^ OpB . Rsult of compare in ZeroBitOfStatus:  If OpA == OpB then status Z = 1 
        //#define   PKA_COMPARE_IM(lenId, OpA, OpBim)   
        //PKA_COMPARE_IM(lenId,a,b); 

        //Returns the ALU Zero-bit from PKA_STATUS register 
        //#define PKA_GET_STATUS_ALU_OUT_ZERO(status)  
        //PKA_GET_STATUS_ALU_OUT_ZERO(stat); 

        //PKA_COMPARE_IM_STATUS(LEN_ID_N_PKA_REG_BITS, EDW_REG_T, 0 - im.val, bit0 -status); // t xor 0? 0^0 = 0;  1^1 = 0;  1^0 = 1;  0^1 = 1 --> bit0 = t xor 0 
        //this shoul do the check for the second case of the rfc: (x^2 * v + u) = 0 mod N ? -> if so, x = x * 2^((p-1)/4) = x * sqrt(-1)
        //I think this shall simply check whether t == 0, if so we run *sq-1, else we do not, this is case 2 of decode from RFC8032
        bit0 = cc3xx_lowlevel_pka_are_equal_si(t, 0); 
        if(bit0) {// bit0 == 0 --> t was 0 -> (x^2 * v + u) = 0 mod N
                printf("%ld\n", bit0);
                printf("reg t is 0\n");
                //sqrt_minus_one auch irgendwo hardcoden oder ad hoc einbauen
                cc3xx_lowlevel_pka_mod_mul(reg_x, curve->sqrt_m1, reg_x); //x = x * sqrt(-1) //case 2 of rfc
                //PKA_MOD_MUL_NFR(LEN_ID_N_BITS, rX, rX, EDW_REG_SQRTM1); // x = x * sqrt(-1)
        }else{
            printf("%ld\n", bit0);
            printf("reg t is not 0\n");
        }
        
        // das hier ist glaube ich nur mod fuer arme
        // oder x = [(u*v^7)^((p-5)/8) * uv^3 *sqrt(-1)] / N
        cc3xx_lowlevel_pka_div(reg_x, curve->field_modulus, t4, reg_x); //t4 = x / N <=> x = (u*v^7)^((p-5)/8) * uv^3 / N falls vorheriger Fall nicht hinghauen hat         
        //PKA_DIV(LEN_ID_N_PKA_REG_BITS, EDW_REG_T4, rX, EDW_REG_N); //t4 = x / N

        //das ist der letzte case aus dem decode aus 8032
        bit0 = cc3xx_lowlevel_pka_test_bits_ui(reg_x, 0, 1); //in der hoffnung, dass das das gleiche ist
        printf("%ld\n", bit0);
        //PKA_READ_BIT0(LEN_ID_N_PKA_REG_BITS, rX, bit0 -bit0); //bit0 = rX[0] ? -> bit0 ist 1 -> x ist ungerade
        if(bit0 != isOddX){ // stimmen istUngerade und sollUngerade nicht überein, dann:
            cc3xx_lowlevel_pka_sub(curve->field_modulus, reg_x, reg_x);       
            cc3xx_lowlevel_pka_read_reg(reg_x, debug, debug_len);
            print_debug("x: ",debug, debug_bytes);
            //PKA_SUB(LEN_ID_N_PKA_REG_BITS, rX, EDW_REG_N, rX); // x = N - x
        }

        //TODO: hier fehlt doch der case, dass x nicht berechnet werden kann, oder?
        
        //jetzt nehmen wir an, dass x fertig berechnet ist, x und y müssen nun also in den mitgegebenen punkt
        //TODO: msb von y auf 0 setzen
        cc3xx_lowlevel_pka_copy(reg_y, decompressed_pt->y);
        
        //TODO: free all the registers
        cc3xx_lowlevel_pka_free_reg(t5);
        cc3xx_lowlevel_pka_free_reg(t4);
        cc3xx_lowlevel_pka_free_reg(t3);
        cc3xx_lowlevel_pka_free_reg(t);

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
    return rc;

}
