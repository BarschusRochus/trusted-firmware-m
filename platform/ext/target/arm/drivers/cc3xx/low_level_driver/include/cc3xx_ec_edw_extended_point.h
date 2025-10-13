/*
 * Copyright (c) 2024, The TrustedFirmware-M Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __CC3XX_EC_EXTENDED_POINT_H__
#define __CC3XX_EC_EXTENDED_POINT_H__

#include <stdint.h>
#include <stddef.h>

#include "cc3xx_ec.h"

typedef struct {
    cc3xx_pka_reg_id_t x;
    cc3xx_pka_reg_id_t y;
    cc3xx_pka_reg_id_t z;
    cc3xx_pka_reg_id_t t;
} cc3xx_ec_point_extended;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief                        Allocate a extended EC point.
 *
 * @return                       An initialized projective EC point.
 */
cc3xx_ec_point_extended cc3xx_lowlevel_ec_allocate_extended_point(void);

/**
 * @brief Allocate an extended point initialised as neutral point
          (X=0, Y=1, Z=1, T=0)
 * 
 * @return cc3xx_ec_point_extended 
 */
cc3xx_ec_point_extended cc3xx_lowlevel_ec_allocate_extended_neutral_point(void);

/**
 * @brief                        Free an extended EC point.
 *
 * @param[in]  p                 A pointer to the extended point to free.
 */
void cc3xx_lowlevel_ec_free_extended_point(cc3xx_ec_point_extended *p);

/**
 * @brief                        Copy an extended EC point.
 *
 * @param[in]  p                 A pointer to the extended point to copy.
 * @param[out] res               A pointer to the extended point to copy into.
 */
void cc3xx_lowlevel_ec_copy_extended_point(cc3xx_ec_point_extended *p,
                                             cc3xx_ec_point_extended *res);

/**
 * @brief                        Convert an affine point to an extended
 *                               point. 
                                 (x,y,z,t) = (x, y, 1, x*y)
 *                               

 * @param[in]  curve             A pointer to an initialized curve object
 * @param[in]  p                 A pointer to the affine point to convert from.
 * @param[out] res               A pointer to the extended point to convert to.
 */
void cc3xx_lowlevel_ec_affine_to_extended(cc3xx_ec_curve_t *curve,
                                          cc3xx_ec_point_affine *p,
                                          cc3xx_ec_point_extended *res);



/**
 * @brief                        Convert an extended point to an affine point.
 *                               (x,y) = (x / z = x * z^-1), (y/z = y *z^-1)
                                 And t is simply ignored.
 *
 * @param[in]  curve             A pointer to an initialized curve object
 * @param[in]  p                 A pointer to the extended point to convert from.
 * @param[out] res               A pointer to the affine point to convert to.
 */
cc3xx_err_t cc3xx_lowlevel_ec_extended_to_affine(cc3xx_ec_curve_t *curve,
                                                 cc3xx_ec_point_extended *p,
                                                 cc3xx_ec_point_affine *res);


/**
 * @brief                        Test if an extended point is the neutral point.
 *
 * @param[in]  p                 A pointer to the extended point to test.
 *
 * @return                       true if the extended point is the identity
 *                               element, false if it isn't.
 */
bool cc3xx_lowlevel_ec_extended_point_is_neutral(cc3xx_ec_point_extended *p);

/**
 * @brief                        Convert an affine point to a Jacobian-form
 *                               projective point, with a random Z coordinate.
 *                               Xa = Xj / Zj^2, Ya = Yj / Zj^2.
 *
 * @note                         If DPA mitigations are disabled by config, this
 *                               function falls back to having a Z coordinate of
 *                               1.
 *
 * @param[in]  curve             A pointer to an initialized curve object
 * @param[in]  p                 A pointer to the affine point to convert.
 * @param[out] res               A pointer to the Jacobian point object.
 */
//void cc3xx_lowlevel_ec_affine_to_jacobian_with_random_z(cc3xx_ec_curve_t *curve,
//                                                        cc3xx_ec_point_affine *p,
//                                                        cc3xx_ec_point_projective *res);


#ifdef __cplusplus
}
#endif

#endif /* __CC3XX_EC_EXTENDED_POINT_H__ */
