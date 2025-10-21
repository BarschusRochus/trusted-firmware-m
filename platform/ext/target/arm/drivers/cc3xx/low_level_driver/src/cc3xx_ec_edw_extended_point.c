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
#include <string.h>
cc3xx_ec_point_extended_data edwards_extended_neutral_element_data = 
    {.x = {0x0}, 
     .y = {0x1},
     .z = {0x1},
     .t = {0x0}};

cc3xx_ec_point_extended cc3xx_lowlevel_ec_allocate_extended_point(void)
{
    cc3xx_ec_point_extended res;

    res.x = cc3xx_lowlevel_pka_allocate_reg();
    res.y = cc3xx_lowlevel_pka_allocate_reg();
    res.z = cc3xx_lowlevel_pka_allocate_reg();
    res.t = cc3xx_lowlevel_pka_allocate_reg();

    return res;
}

cc3xx_ec_point_extended cc3xx_lowlevel_ec_allocate_extended_neutral_point(void)
{
    cc3xx_ec_point_extended res;

    res.x = cc3xx_lowlevel_pka_allocate_reg();
    res.y = cc3xx_lowlevel_pka_allocate_reg();
    res.z = cc3xx_lowlevel_pka_allocate_reg();
    res.t = cc3xx_lowlevel_pka_allocate_reg();

    cc3xx_lowlevel_ec_extended_point_from_data(&edwards_extended_neutral_element_data, &res);

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

bool cc3xx_lowlevel_ec_extended_point_is_neutral(cc3xx_ec_point_extended *p){
    uint8_t ret = 0;
    
    ret += cc3xx_lowlevel_pka_are_equal_si(p->x, 0x0);
    ret += cc3xx_lowlevel_pka_are_equal_si(p->y, 0x1);
    //does that make sense? Isn't there something about z being arbitrary?
    ret += cc3xx_lowlevel_pka_are_equal_si(p->z, 0x1);
    ret += cc3xx_lowlevel_pka_are_equal_si(p->t, 0x0);
    
    return(ret != 0 );
}

void cc3xx_lowlevel_ec_extended_point_from_data(cc3xx_ec_point_extended_data *data, cc3xx_ec_point_extended *pt){
    cc3xx_lowlevel_pka_clear(pt->x);
    cc3xx_lowlevel_pka_write_reg(pt->x, data->x, 32);
    
    cc3xx_lowlevel_pka_clear(pt->y);
    cc3xx_lowlevel_pka_write_reg(pt->y, data->y, 32);
    
    cc3xx_lowlevel_pka_clear(pt->z);
    cc3xx_lowlevel_pka_write_reg(pt->z, data->z, 32);
    
    cc3xx_lowlevel_pka_clear(pt->t);
    cc3xx_lowlevel_pka_write_reg(pt->t, data->t, 32);
}

void cc3xx_lowlevel_ec_extended_point_to_data(cc3xx_ec_point_extended_data *data, cc3xx_ec_point_extended *pt){
    memset(data->x, 0x0, 32);
    cc3xx_lowlevel_pka_read_reg(pt->x, data->x, 32);
    
    memset(data->y, 0x0, 32);
    cc3xx_lowlevel_pka_read_reg(pt->y, data->y, 32);
    
    memset(data->z, 0x0, 32);
    cc3xx_lowlevel_pka_read_reg(pt->z, data->z, 32);
    
    memset(data->t, 0x0, 32);
    cc3xx_lowlevel_pka_read_reg(pt->t, data->t, 32);
}
