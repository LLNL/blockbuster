## --------------------------------------------------------- ##
## Includes <stdlib.h> and <stddef.h> if available.          ##
## Adapted from AC_CHECK_SIZEOF (Autoconf 2.12).             ##
## --------------------------------------------------------- ##

# serial 1

dnl mfx_CHECK_SIZEOF(TYPE [, CROSS-SIZE])
AC_DEFUN(mfx_CHECK_SIZEOF,
[changequote(<<, >>)dnl
dnl The name to #define.
define(<<AC_TYPE_NAME>>, translit(sizeof_$1, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$1, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING([size of $1])
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
int main() {
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof($1));
  exit(0);
}], AC_CV_NAME=`cat conftestval`, AC_CV_NAME=0, ifelse([$2], , , AC_CV_NAME=$2))])dnl
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME)
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])
