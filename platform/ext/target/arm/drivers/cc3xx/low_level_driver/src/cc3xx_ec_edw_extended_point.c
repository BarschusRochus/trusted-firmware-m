/*
 * Copyright (c) 2024, The TrustedFirmware-M Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "cc3xx_ec_edw_extended_point.h"
#include "cc3xx_error.h"

#ifndef CC3XX_CONFIG_FILE
#include "cc3xx_config.h"
#else
#include CC3XX_CONFIG_FILE
#endif
#include "cc3xx_pka.h"
#include "cc3xx_ec.h"

#include "fatal_error.h"

cc3xx_ec_point_extended cc3xx_lowlevel_ec_allocate_extended_point(void)
{
    cc3xx_ec_point_extended res;

    res.x = cc3xx_lowlevel_pka_allocate_reg();
    res.y = cc3xx_lowlevel_pka_allocate_reg();
    res.z = cc3xx_lowlevel_pka_allocate_reg();
    res.t = cc3xx_lowlevel_pka_allocate_reg();

    return res;
}

//free in reverse order as allocation happened
void cc3xx_lowlevel_ec_free_extended_point(cc3xx_ec_point_extended *p){
    cc3xx_lowlevel_pka_free_reg(p->t);
    cc3xx_lowlevel_pka_free_reg(p->z);
    cc3xx_lowlevel_pka_free_reg(p->y);
    cc3xx_lowlevel_pka_free_reg(p->x);
}

//no assertions, just assume that register is set and in use
void cc3xx_lowlevel_ec_copy_extended_point(cc3xx_ec_point_extended *p,
                                             cc3xx_ec_point_extended *res){
    cc3xx_lowlevel_pka_copy(p->x, res->x);
    cc3xx_lowlevel_pka_copy(p->y, res->y);
    cc3xx_lowlevel_pka_copy(p->z, res->z);
    cc3xx_lowlevel_pka_copy(p->t, res->t);
}

void cc3xx_lowlevel_ec_affine_to_extended(cc3xx_ec_curve_t *curve,
            cc3xx_ec_point_affine *p, cc3xx_ec_point_extended *res)
{
    //x = x
    cc3xx_lowlevel_pka_copy(p->x, res->x);
    //y = y
    cc3xx_lowlevel_pka_copy(p->y, res->y);
    
    //z = 1 = 0 + 1
    //that is apparently what we do when going extended
    cc3xx_lowlevel_pka_clear(res->z);
    cc3xx_lowlevel_pka_add_si(res->z, 0x1, res->z);
    
    //t = x * y
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    cc3xx_lowlevel_pka_mod_mul(p->x, p->y, res->t);

    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);
}


cc3xx_err_t cc3xx_lowlevel_ec_extended_to_affine(cc3xx_ec_curve_t *curve,
                                                 cc3xx_ec_point_extended *p,
                                                 cc3xx_ec_point_affine *res)
{
    cc3xx_lowlevel_pka_set_modulus(curve->field_modulus, false, CC3XX_PKA_REG_NP);
    //(x,y) = (x / z = x * z^-1), (y/z = y *z^-1)
    
    //z_inv = z^-1
    cc3xx_pka_reg_id_t z_inv = cc3xx_lowlevel_pka_allocate_reg();
    cc3xx_lowlevel_pka_mod_inv_prime_modulus(p->z, z_inv);

    //x = x * z^-1; y = y * z^-1
    cc3xx_lowlevel_pka_mod_mul(p->x, z_inv, res->x);
    cc3xx_lowlevel_pka_mod_mul(p->y, z_inv, res->y);

    cc3xx_lowlevel_pka_free_reg(z_inv);
    cc3xx_lowlevel_pka_set_modulus(curve->order, false, CC3XX_PKA_REG_NP);

    return CC3XX_ERR_SUCCESS;

}
