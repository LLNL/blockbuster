## --------------------------------------------------------- ##
## Check for 8-bit clean memcmp.                             ##
## Adapted from AC_FUNC_MEMCMP.                              ##
## --------------------------------------------------------- ##

# serial 1

AC_DEFUN(mfx_FUNC_MEMCMP,
[AC_CACHE_CHECK(for 8-bit clean memcmp, mfx_cv_func_memcmp,
[AC_TRY_RUN([int main() {
  char c0 = 0x40, c1 = 0x80, c2 = 0x81;
  exit(memcmp(&c0, &c2, 1) < 0 && memcmp(&c1, &c2, 1) < 0 ? 0 : 1); }],
mfx_cv_func_memcmp=yes,
mfx_cv_func_memcmp=no,
mfx_cv_func_memcmp=unknown)])
if test "$mfx_cv_func_memcmp" = no; then
  AC_DEFINE(NO_MEMCMP)
fi
])
