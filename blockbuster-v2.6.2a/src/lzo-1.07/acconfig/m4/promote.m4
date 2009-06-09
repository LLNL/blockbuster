## --------------------------------------------------------- ##
## Check how the compiler promotes integrals.                ##
## --------------------------------------------------------- ##

# serial 1

AC_DEFUN(mfx_PROG_CC_INTEGRAL_PROMOTION,
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_CACHE_CHECK(how the compiler promotes integrals,
mfx_cv_prog_cc_integral_promotion,
[AC_TRY_RUN([int main(){ unsigned char c; int s;
  c = (unsigned char) (1 << (8 * sizeof(char) - 1));
  s = 8 * (int) (sizeof(int) - sizeof(char));
  exit(((c << s) > 0) ? 1 : 0);}],
mfx_cv_prog_cc_integral_promotion="ANSI (value-preserving)",
mfx_cv_prog_cc_integral_promotion="Classic (unsigned-preserving)",
mfx_cv_prog_cc_integral_promotion="unknown")])
])
