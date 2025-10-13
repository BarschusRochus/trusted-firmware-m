#ifndef __CC3XX_EC_EDW_H__
#define __CC3XX_EC_EDW_H__

#include <stdint.h>
#include <stddef.h>

#include "cc3xx_error.h"
#include "cc3xx_ec.h"
#include "cc3xx_ec_edw_extended_point.h"


#ifdef __cplusplus
extern "C" {
#endif

//remove me when no longer needed
void debug_read_and_print_reg(cc3xx_pka_reg_id_t reg, char* label);

/**
  @brief Decompress a given y coordinate and compute affine coordinates x and y
         of a curve point.

  @param compressed     [in] Y coordinate to decompress
  @param curve          [in] curve object
  @param decompressed   [out] X coordinate calculated from Y
 */
cc3xx_err_t cc3xx_lowlevel_ec_edw_decompress_point(uint32_t *compressed, 
                cc3xx_ec_curve_t *curve, cc3xx_ec_point_affine *decompressed);


/**
 * @brief Compress a point from (X,Y) to just an Y coordinte with msb set to 
          mod 2 of x.
 * 
 * @param curve 
 * @param point 
 * @param compressed 
 */
void cc3xx_lowlevel_ec_edw_compress_point(cc3xx_ec_curve_t *curve, cc3xx_ec_point_affine *point, 
                    uint32_t *compressed);

/**
  @brief Checks wether a given point is on the curve. 
         I.e. wether the x,y coordinates fullfill the curve equation.

  @param p [in] Point to verify
  @param curve [in] Curve object
  @return true in case point is on curve, false otherwise
 */
bool cc3xx_lowlevel_ec_edw_is_point_on_curve(cc3xx_ec_point_affine *p, cc3xx_ec_curve_t *curve);

/**
 * @brief                        Add two affine points
 *
 * @param[in]  curve             A pointer to an initialized edwards curve
 *                               object
 * @param[in]  p                 A pointer to the first affine point object to
 *                               add.
 * @param[in]  q                 A pointer to the second affine point object to
 *                               add.
 * @param[out] res               A pointer to the affine point object which the
 *                               result will be written to.
 *
 * @return                       CC3XX_ERR_SUCCESS on success, another
 *                               cc3xx_err_t on error.
 */
cc3xx_err_t cc3xx_lowlevel_ec_edwards_add_points(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     cc3xx_ec_point_affine *q,
                                                     cc3xx_ec_point_affine *res);



/**
 * @brief Doubles the given affine point ( res = pt + pt)
 * 
 * @param curve 
 * @param p 
 * @param res 
 * @return cc3xx_err_t 
 */
cc3xx_err_t cc3xx_lowlevel_ec_edwards_double_point(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_affine *p, cc3xx_ec_point_affine *res);

//remove me
void cc3xx_lowlevel_ec_edwards_double_extended_points(cc3xx_ec_curve_t *curve,
        cc3xx_ec_point_extended *p, cc3xx_ec_point_extended *res);


/**
 * @brief 
 * 
 * @param curve 
 * @param p 
 * @param scalar 
 * @param res 
 */
cc3xx_err_t cc3xx_lowlevel_ec_edwards_scalar_mult(cc3xx_ec_curve_t *curve,
                                                     cc3xx_ec_point_affine *p,
                                                     uint32_t *scalar,
                                                     cc3xx_ec_point_affine *res);
#ifdef __cplusplus
}
#endif

#endif /* __CC3XX_EC_EDW_H */
