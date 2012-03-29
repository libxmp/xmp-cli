dnl AC_CONFIG_AUX_DIR(./scripts)
AC_INIT
0>confdefs.h

AC_ARG_WITH(libxmp, [  --with-libxmp=<path>    libxmp prefix (optional)],
  libxmp_path="$withval")

AC_PROG_CC
AC_C_BIGENDIAN(AC_DEFINE(ENDIAN_BIG))

if test "$libxmp_path" != ""; then
  CPPFLAGS="${CPPFLAGS} -I${libxmp_path}/include"
  LIBS="${LIBS} -L${libxmp_path}/lib"
fi

AC_DEFUN([AC_CHECK_DEFINED],[
  AS_VAR_PUSHDEF([ac_var],[ac_cv_defined_$1])dnl
  AC_CACHE_CHECK([for $1 defined], ac_var,
  AC_TRY_COMPILE(,[
    #ifdef $1
    int ok;
    #else
    choke me
    #endif
  ],AS_VAR_SET(ac_var, yes),AS_VAR_SET(ac_var, no)))
  AS_IF([test AS_VAR_GET(ac_var) != "no"], [$2], [$3])dnl
  AS_VAR_POPDEF([ac_var])dnl
])

AC_CHECK_HEADERS(xmp.h getopt.h signal.h termios.h)

system=unknown

AC_CHECK_DEFINED(_WIN32,[system=win32])
AC_CHECK_DEFINED(__BEOS__,[system=beos])
AC_CHECK_DEFINED(__HAIKU__,[system=beos])

if test "$system" = "unknown"; then
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h,,[
    AC_CHECK_HEADERS(sys/soundcard.h alsa/asoundlib.h sndio.h sys/audioio.h)
  ])
fi

if test "${ac_cv_header_xmp_h}" = "yes"; then
  AC_CHECK_LIB(xmp,xmp_player_start,,
    AC_MSG_ERROR(Can't find libxmp))
else
  AC_MSG_ERROR(Can't find libxmp header files)
fi

case "$system" in
beos)
  AC_DEFINE(SOUND_BEOS)
  DRIVERS="${DRIVERS} sound_beos.o"
  LIBS="${LIBS} -lmedia -lbe"
  ;;
win32)
  AC_DEFINE(SOUND_WIN32)
  DRIVERS="${DRIVERS} sound_win32.o"
  LIBS="${LIBS} -lwinmm"
  ;;
*)
  if test "${ac_cv_header_CoreAudio_CoreAudio_h}" = "yes"; then
    AC_DEFINE(SOUND_COREAUDIO)
    DRIVERS="${DRIVERS} sound_coreaudio.o"
    LIBS="${LIBS} -framework AudioToolbox -framework AudioUnit -framework CoreServices"
  else
    if test "${ac_cv_header_sndio_h}" = "yes"; then
      AC_DEFINE(SOUND_SNDIO)
      DRIVERS="${DRIVERS} sound_sndio.o"
      LIBS="${LIBS} -lsndio"
    fi
    if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
      AC_DEFINE(SOUND_BSD)
      DRIVERS="${DRIVERS} sound_bsd.o"
    fi
    if test "${ac_cv_header_sys_soundcard_h}" = "yes"; then
      AC_DEFINE(SOUND_OSS)
      DRIVERS="${DRIVERS} sound_oss.o"
    fi
    if test "${ac_cv_header_alsa_asoundlib_h}" = "yes"; then
      AC_DEFINE(SOUND_ALSA)
      DRIVERS="${DRIVERS} sound_alsa.o"
      LIBS="${LIBS} -lasound"
    fi
  fi
  ;;
esac

dnl XMP_TRY_COMPILE(<message>,<cache-var>,<flags>,<program>,<ifyes>,<ifno>)
define(XMP_TRY_COMPILE,[
  AC_CACHE_CHECK([$1],[$2],[
    oldcflags="${CFLAGS}"
    CFLAGS="${CFLAGS} $3"
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[$4]])],[$2=yes],[$2=no],[true])
    CFLAGS="${oldcflags}"])
  if test "x$$2" = xyes; then
    true; $5
  else
    true; $6
  fi])

if test "${GCC-no}" = yes; then
  XMP_TRY_COMPILE(whether compiler understands -Wmultichar,
    ac_cv_c_flag_w_multichar,
    -Wmultichar,[
    int main(){}],
    CFLAGS="${CFLAGS} -Wno-multichar")  
fi

AC_CHECK_FUNCS(kill getopt_long)
AC_PROG_INSTALL
AC_SUBST(DRIVERS)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
