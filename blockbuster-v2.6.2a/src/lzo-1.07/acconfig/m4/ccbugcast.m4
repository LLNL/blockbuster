## --------------------------------------------------------- ##
## Checking if compiler correctly cast signed to unsigned.   ##
## Adapted from zsh.                                         ##
## --------------------------------------------------------- ##

# serial 1

AC_DEFUN(mfx_PROG_CC_BUG_SIGNED_TO_UNSIGNED_CASTING,
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
AC_CACHE_CHECK(whether signed to unsigned casting is broken,
mfx_cv_prog_cc_bug_signed_to_unsigned_casting,
[AC_TRY_RUN([int main(){return((int)(unsigned char)((char) -1) != 255);}],
mfx_cv_prog_cc_bug_signed_to_unsigned_casting=no,
mfx_cv_prog_cc_bug_signed_to_unsigned_casting=yes,
mfx_cv_prog_cc_bug_signed_to_unsigned_casting=unknown)])
])
