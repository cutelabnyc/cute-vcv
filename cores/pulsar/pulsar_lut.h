#include "stdint.h"
#include "string.h"
#include "math.h"
#ifdef   __cplusplus
extern "C"
{
#endif

#pragma once

// #ifndef _INT8_T
// #define _INT8_T
// typedef signed char int8_t;
// #endif /* _INT8_T */

// #ifndef _INT16_T
// #define _INT16_T
// typedef short int16_t;
// #endif /* _INT16_T */

// #ifndef _INT32_T
// #define _INT32_T
// typedef int int32_t;
// #endif /* _INT32_T */

// #ifndef _INT64_T
// #define _INT64_T
// typedef long long int64_t;
// #endif /* _INT64_T */

// #ifndef _UINT16_T
// #define _UINT16_T
// typedef unsigned short uint16_t;
// #endif /* _UINT16_T */

// #ifndef _UINT32_T
// #define _UINT32_T
// typedef unsigned int uint32_t;
// #endif /* _UINT32_T */

  /**
   * @brief Macros required for reciprocal calculation in Normalized LMS
   */

#define DELTA_Q31          (0x100)
#define DELTA_Q15          0x5
#define INDEX_MASK         0x0000003F
#ifndef PI
  #define PI               3.14159265358979f
#endif

  /**
   * @brief Macros required for SINE and COSINE Fast math approximations
   */

#define FAST_MATH_TABLE_SIZE  512
#define FAST_MATH_Q31_SHIFT   (32 - 10)
#define FAST_MATH_Q15_SHIFT   (16 - 10)
#define CONTROLLER_Q31_SHIFT  (32 - 9)
#define TABLE_SPACING_Q31     0x400000
#define TABLE_SPACING_Q15     0x80

  /**
   * @brief Macros required for SINE and COSINE Controller functions
   */
  /* 1.31(q31) Fixed value of 2/360 */
  /* -1 to +1 is divided into 360 values so total spacing is (2/360) */
#define INPUT_SPACING         0xB60B61

  /**
   * @brief Macro for Unaligned Support
   */
#ifndef UNALIGNED_SUPPORT_DISABLE
    #define ALIGN4
#else
  #if defined  (__GNUC__)
    #define ALIGN4 __attribute__((aligned(4)))
  #else
    #define ALIGN4 __align(4)
  #endif
#endif   /* #ifndef UNALIGNED_SUPPORT_DISABLE */

  /**
   * @brief 8-bit fractional data type in 1.7 format.
   */
  typedef int8_t q7_t;

  /**
   * @brief 16-bit fractional data type in 1.15 format.
   */
  typedef int16_t q15_t;

  /**
   * @brief 32-bit fractional data type in 1.31 format.
   */
  typedef int32_t q31_t;

  /**
   * @brief 64-bit fractional data type in 1.63 format.
   */
  typedef int64_t q63_t;

  /**
   * @brief 32-bit floating-point type definition.
   */
  typedef float float32_t;

  /**
   * @brief 64-bit floating-point type definition.
   */
  typedef double float64_t;

  /**
   * @brief  Fast approximation to the trigonometric sine function for floating-point data.
   * @param[in] x  input value in radians.
   * @return  sin(x).
   */
  float32_t arm_sin_f32(
  float32_t x);


  /**
   * @brief  Fast approximation to the trigonometric sine function for Q31 data.
   * @param[in] x  Scaled input value in radians.
   * @return  sin(x).
   */
  q31_t arm_sin_q31(
  q31_t x);

  /**
   * @brief  Fast arbitrary lookup table for unsigned 32 bit input. Table must be a power of two in size, plus one
   * @param[in] x Phase input. 0 corresponds to the first element in the table, 4,294,967,295 is maximum phase.
   * @return  linear interpolation of the two nearest values in the table.
   */
  float32_t lookup_u32(
  uint32_t x,
  const float32_t *table,
  uint32_t table_size);

  /**
   * @brief  Fast approximation to the trigonometric sine function for Q15 data.
   * @param[in] x  Scaled input value in radians.
   * @return  sin(x).
   */
  q15_t arm_sin_q15(
  q15_t x);


  /**
   * @brief  Fast approximation to the trigonometric cosine function for floating-point data.
   * @param[in] x  input value in radians.
   * @return  cos(x).
   */
  float32_t arm_cos_f32(
  float32_t x);


  /**
   * @brief Fast approximation to the trigonometric cosine function for Q31 data.
   * @param[in] x  Scaled input value in radians.
   * @return  cos(x).
   */
  q31_t arm_cos_q31(
  q31_t x);


  /**
   * @brief  Fast approximation to the trigonometric cosine function for Q15 data.
   * @param[in] x  Scaled input value in radians.
   * @return  cos(x).
   */
  q15_t arm_cos_q15(
  q15_t x);

  /**
   * @brief  Fast generic LUT for q31 input. Table assumed to be FAST_MATH_TABLE_SIZE long.
   * @param[in] x  Scaled input value in radians.
   * @return  lut[x].
   */
  q31_t arm_lut_q31(
  q31_t x,
  const q31_t *table);

  /* Tables for Fast Math Sine and Cosine */
extern const float32_t sinTable_f32[FAST_MATH_TABLE_SIZE + 1];
extern const q31_t sinTable_q31[FAST_MATH_TABLE_SIZE + 1];
extern const q15_t sinTable_q15[FAST_MATH_TABLE_SIZE + 1];
extern const q31_t sinc_lut_q31[FAST_MATH_TABLE_SIZE + 1];
extern const q31_t ramp_lut_q31[FAST_MATH_TABLE_SIZE + 1];

/* Farey sequence lookup */
#define FAREY_SIZE 58
extern const float farey[FAREY_SIZE];

/* Just some fun ratios */
#define FUN_RATIOS_SIZE 14
extern const float fun_ratios[FUN_RATIOS_SIZE];

#ifdef   __cplusplus
}
#endif