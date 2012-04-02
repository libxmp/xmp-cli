dnl AC_CONFIG_AUX_DIR(./scripts)
AC_INIT
0>confdefs.h

AC_ARG_ENABLE(oss,
  [  --disable-oss           don't compile OSS support])
AC_ARG_ENABLE(alsa,
  [  --disable-alsa          don't compile ALSA support])
AC_ARG_ENABLE(pulseaudio,
  [  --enable-pulseaudio     compile PulseAudio support])
AC_ARG_WITH(libxmp,
  [  --with-libxmp=<path>    libxmp prefix (optional)],
  libxmp_path="$withval")

AC_CANONICAL_HOST
AC_PROG_CC

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

if test "${ac_cv_header_xmp_h}" = "yes"; then
  AC_CHECK_LIB(xmp,xmp_player_start,,
    AC_MSG_ERROR(Can't find libxmp))
else
  AC_MSG_ERROR(Can't find libxmp header files)
fi

case "$host_cpu" in
powerpc64)
  CFLAGS="${CFLAGS} -m64"
  LDFLAGS="${LDFLAGS} -m64"
  ;;
esac

AC_CHECK_HEADERS(sys/audioio.h)

if test "${enable_oss}" != "no"; then
  AC_CHECK_HEADER(sys/soundcard.h machine/soundcard.h)
  if test "${ac_cv_header_sys_soundcard_h}" = "yes" -o "${ac_cv_header_machine_soundcard_h}" = "yes"; then
    AC_DEFINE(SOUND_OSS)
    DRIVERS="${DRIVERS} sound_oss.o"
  fi
fi
if test "${enable_alsa}" != "no"; then
  AC_CHECK_HEADER(alsa/asoundlib.h)
  if test "${ac_cv_header_alsa_asoundlib_h}" = "yes"; then
    AC_DEFINE(SOUND_ALSA)
    DRIVERS="${DRIVERS} sound_alsa.o"
    LIBS="${LIBS} -lasound"
  fi
fi
if test "${enable_pulseaudio}" = "yes"; then
  AC_CHECK_HEADER(pulse/simple.h)
  if test "${ac_cv_header_pulse_simple_h}" = "yes"; then
    AC_DEFINE(SOUND_PULSEAUDIO)
    DRIVERS="${DRIVERS} pulseaudio.o"
    LIBS="${LIBS} -lpulse-simple -lpulse"
  fi
fi

case "${host_os}" in
amigaos*|aros)
  DRIVERS="${DRIVERS} sound_ahi.o"
  AC_DEFINE(SOUND_AHI)
  ;;
darwin*)
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h)
  if test "${ac_cv_header_CoreAudio_CoreAudio_h}" = "yes"; then
    AC_DEFINE(SOUND_COREAUDIO)
    DRIVERS="${DRIVERS} sound_coreaudio.o"
    LIBS="${LIBS} -framework AudioToolbox -framework AudioUnit -framework CoreServices"
  fi
  ;;
openbsd*)
  AC_CHECK_HEADER(sndio.h)
  if test "${ac_cv_header_sndio_h}" = "yes"; then
    AC_DEFINE(SOUND_SNDIO)
    DRIVERS="${DRIVERS} sound_sndio.o"
    LIBS="${LIBS} -lsndio"
  fi
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_BSD)
    DRIVERS="${DRIVERS} sound_bsd.o"
  fi
  ;;
netbsd*)
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_NETBSD)
    DRIVERS="${DRIVERS} sound_netbsd.o"
  fi
  ;;
solaris*)
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_SOLARIS)
    DRIVERS="${DRIVERS} sound_solaris.o"
  fi
  ;;
hpux*)
  AC_CHECK_HEADER(sys/audio.h)
  if test "${ac_cv_header_sys_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_HPUX)
    DRIVERS="${DRIVERS} sound_hpux.o"
  fi
  ;;
aix*)
  AC_CHECK_HEADER(audio.h)
  if test "${ac_cv_header_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_AIX)
    DRIVERS="${DRIVERS} sound_aix.o"
  fi
  ;;
irix*)
  AC_CHECK_HEADERS(dmedia/audio.h)
  if test "${ac_cv_header_dmedia_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_SGI)
    DRIVERS="${DRIVERS} sound_sgi.o"
    LIBS="${LIBS} -laudio"
  fi
  ;;
cygwin*|mingw*)
  AC_DEFINE(SOUND_WIN32)
  DRIVERS="${DRIVERS} sound_win32.o"
  LIBS="${LIBS} -lwinmm"
  ;;
beos*|haiku*)
  AC_DEFINE(SOUND_BEOS)
  DRIVERS="${DRIVERS} sound_beos.o"
  LIBS="${LIBS} -lmedia -lbe"
  ;;
esac

if test -z "${DRIVERS}" -a "${ac_cv_header_sys_audioio_h}" = "yes"; then
  AC_DEFINE(SOUND_BSD)
  DRIVERS="${DRIVERS} sound_bsd.o"
fi

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
