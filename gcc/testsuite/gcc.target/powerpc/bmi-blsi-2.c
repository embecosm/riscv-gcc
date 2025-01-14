/* { dg-do run } */
/* { dg-options "-O3 -fno-inline" } */
/* { dg-require-effective-target lp64 } */

#define NO_WARN_X86_INTRINSICS 1
#include <x86intrin.h>
#include "bmi-check.h"

/* To fool the compiler, so it does not generate blsi here. */
int calc_blsi_u32 (int src1, int src2)
{
  return (-src1) & (src2);
}

static void
bmi_test()
{
  unsigned i;
  int src = 0xfacec0ff;
  int res, res_ref;

  for (i=0; i<5; ++i) {
    src = (i + src) << i;

    res_ref = calc_blsi_u32 (src, src);
    res = __blsi_u32 (src);

    if (res != res_ref)
      abort();
  }
}
