## --------------------------------------------------------- ##
## Check if gcc suffers the '-fschedule-insns' bug.          ##
## --------------------------------------------------------- ##

# serial 1

AC_DEFUN(mfx_PROG_GCC_BUG_SCHEDULE_INSNS,
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
if test "$ac_cv_prog_gcc" = yes; then
mfx_save_cflags="$CFLAGS"
CFLAGS="-O2 -fschedule-insns -fschedule-insns2"
AC_CACHE_CHECK(whether ${CC-cc} suffers the -fschedule-insns bug,
mfx_cv_prog_gcc_bug_schedule_insns,
[AC_TRY_RUN([int main() {
/* gcc schedule-insns optimization bug on RS6000 platforms.
 * Adapted from bug-report by Assar Westerlund <assar@sics.se>
 * Compile and run it using gcc -O2 -fno-schedule-insns and
 * gcc -O2 -fschedule-insns.
 */
  const int clone[] = {1, 2, 0};
  const int *q;
  q = clone; if (*q) { return 0; }
  return 1; }],
mfx_cv_prog_gcc_bug_schedule_insns=no,
mfx_cv_prog_gcc_bug_schedule_insns=yes,
mfx_cv_prog_gcc_bug_schedule_insns=unknown)])
CFLAGS="$mfx_save_cflags"
fi
])
