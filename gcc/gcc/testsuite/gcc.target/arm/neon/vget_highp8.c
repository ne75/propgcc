/* Test the `vget_highp8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vget_highp8 (void)
{
  poly8x8_t out_poly8x8_t;
  poly8x16_t arg0_poly8x16_t;

  out_poly8x8_t = vget_high_p8 (arg0_poly8x16_t);
}

/* { dg-final { cleanup-saved-temps } } */