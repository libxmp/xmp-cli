AC_INIT([xmp], [4.0.9])
AC_CONFIG_AUX_DIR([build-aux])
0>confdefs.h

AC_ARG_ENABLE(oss,
  [  --disable-oss           don't compile OSS support])
AC_ARG_ENABLE(alsa,
  [  --disable-alsa          don't compile ALSA support])
AC_ARG_ENABLE(pulseaudio,
  [  --enable-pulseaudio     compile PulseAudio support])

AC_SUBST(LD_VERSCRIPT)
AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
case "$host_os" in
beos*|haiku*)
	AS_IF([! which "$CXX" >/dev/null 2>/dev/null],
		[AC_MSG_ERROR([C++ compiler required on this platform ($host_os)])])
	;;
*)
	# Do not error out on linking when g++ is absent.
	CXXLD='${CCLD}'
	AC_SUBST([CXXLD])
	;;
esac

AC_PROG_INSTALL
AM_INIT_AUTOMAKE([foreign subdir-objects tar-pax])

PKG_CHECK_MODULES([libxmp], [libxmp >= 4.3],
  AC_CHECK_LIB(xmp, xmp_version, [], [exit 1]),
  [echo "You need libxmp version 4.3 or later to build this package"; exit 1])

dnl Don't use things like /usr/etc or /usr/var

fix_prefix=no
if test "x$prefix" = xNONE; then
    fix_prefix=yes
elif test "x$prefix" = x/usr -o "x$prefix" = x/usr/local; then
    fix_prefix=yes
fi

if test $fix_prefix = yes -a $sysconfdir = '${prefix}/etc'; then
    sysconfdir=/etc
    localstatedir=/var
fi

dnl XMP_TRY_COMPILE(<message>,<cache-var>,<flags>,<program>,<ifyes>,<ifno>)
AC_DEFUN([XMP_TRY_COMPILE],[
  AC_CACHE_CHECK([$1],[$2],[
    oldcflags="${CFLAGS}"
    CFLAGS="${CFLAGS} $3"
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[$4]])],[$2=yes],[$2=no],[true])
    CFLAGS="${oldcflags}"])
  AS_IF([test "x$$2" = xyes], [$5], [$6])])

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

AC_CHECK_HEADERS([getopt.h signal.h termios.h sys/time.h sys/audioio.h])

case "$host_cpu" in
powerpc64)
  CFLAGS="${CFLAGS} -m64"
  LDFLAGS="${LDFLAGS} -m64"
  ;;
esac

AM_CONDITIONAL([SOUND_AHI], [false])
AM_CONDITIONAL([SOUND_AIX], [false])
AM_CONDITIONAL([SOUND_ALSA05], [false])
AM_CONDITIONAL([SOUND_ALSA], [false])
AM_CONDITIONAL([SOUND_BEOS], [false])
AM_CONDITIONAL([SOUND_BSD], [false])
AM_CONDITIONAL([SOUND_COREAUDIO], [false])
AM_CONDITIONAL([SOUND_HPUX], [false])
AM_CONDITIONAL([SOUND_NETBSD], [false])
AM_CONDITIONAL([SOUND_OSS], [false])
AM_CONDITIONAL([SOUND_PULSEAUDIO], [false])
AM_CONDITIONAL([SOUND_QNX], [false])
AM_CONDITIONAL([SOUND_SGI], [false])
AM_CONDITIONAL([SOUND_SNDIO], [false])
AM_CONDITIONAL([SOUND_SOLARIS], [false])
AM_CONDITIONAL([SOUND_WIN32], [false])

if test "${enable_oss}" != "no"; then
  AC_CHECK_HEADERS(sys/soundcard.h machine/soundcard.h)
  if test "${ac_cv_header_sys_soundcard_h}" = "yes" -o "${ac_cv_header_machine_soundcard_h}" = "yes"; then
    AC_DEFINE(SOUND_OSS)
    AM_CONDITIONAL([SOUND_OSS], [true])
  fi
fi

AS_IF([test "$enable_alsa" = "yes"], [
  PKG_CHECK_MODULES([alsa], [alsa >= 1], [enable_alsa=yes])
], [
  AS_IF([test "$enable_alsa" != "no"], [
    PKG_CHECK_MODULES([alsa], [alsa >= 1],
      AC_CHECK_LIB(asound, snd_pcm_open, [enable_alsa=yes]),
      [true])
  ])
])
AS_IF([test "$enable_alsa" = "yes"], [
  AC_DEFINE(SOUND_ALSA)
  AM_CONDITIONAL([SOUND_ALSA], [true])
])

AS_IF([test "$enable_pulseaudio" = "yes"], [
  PKG_CHECK_MODULES([pulseaudio], [libpulse-simple], [enable_pulseaudio=yes])
], [
  AS_IF([test "$enable_pulseaudio" != "no"], [
    PKG_CHECK_MODULES([pulseaudio], [libpulse-simple],
      AC_CHECK_LIB(pulse-simple, pa_simple_new,	[enable_pulseaudio=yes]),
      [true])
  ])
])
AS_IF([test "$enable_pulseaudio" = "yes"], [
  AC_DEFINE(SOUND_PULSEAUDIO)
  AM_CONDITIONAL([SOUND_PULSEAUDIO], [true])
])

case "${host_os}" in
amigaos*|aros)
  AC_DEFINE(SOUND_AHI)
  AM_CONDITIONAL([SOUND_AHI], [true])
  ;;
darwin*)
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h)
  if test "${ac_cv_header_CoreAudio_CoreAudio_h}" = "yes"; then
    AC_DEFINE(SOUND_COREAUDIO)
    AM_CONDITIONAL([SOUND_COREAUDIO], [true])
    XMP_DARWIN_LDFLAGS="-framework AudioToolbox -framework AudioUnit -framework CoreServices"
    AC_SUBST([XMP_DARWIN_LDFLAGS])
  fi
  ;;
openbsd*)
  AC_CHECK_HEADER(sndio.h)
  if test "${ac_cv_header_sndio_h}" = "yes"; then
    AC_DEFINE(SOUND_SNDIO)
    AM_CONDITIONAL([SOUND_SNDIO], [true])
  fi
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_BSD)
    AM_CONDITIONAL([SOUND_BSD], [true])
  fi
  ;;
netbsd*)
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_NETBSD)
    AM_CONDITIONAL([SOUND_NETBSD], [true])
  fi
  ;;
solaris*)
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_SOLARIS)
    AM_CONDITIONAL([SOUND_SOLARIS], [true])
  fi
  ;;
hpux*)
  AC_CHECK_HEADER(sys/audio.h)
  if test "${ac_cv_header_sys_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_HPUX)
    AM_CONDITIONAL([SOUND_HPUX], [true])
  fi
  ;;
aix*)
  AC_CHECK_HEADER(audio.h)
  if test "${ac_cv_header_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_AIX)
    AM_CONDITIONAL([SOUND_AIX], [true])
  fi
  ;;
irix*)
  AC_CHECK_HEADERS(dmedia/audio.h)
  if test "${ac_cv_header_dmedia_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_SGI)
    AM_CONDITIONAL([SOUND_SGI], [true])
  fi
  ;;
qnx*)
  AC_CHECK_HEADERS(sys/audio.h)
  if test "${ac_cv_header_sys_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_QNX)
    AM_CONDITIONAL([SOUND_QNX], [true])
  fi
  ;;
nto-qnx*)
  AC_CHECK_HEADERS(sys/asoundlib.h)
  if test "${ac_cv_header_sys_asoundlib_h}" = "yes"; then
    AC_DEFINE(SOUND_ALSA05)
    AM_CONDITIONAL([SOUND_ALSA05], [true])
  fi
  ;;
cygwin*|mingw*)
  AC_DEFINE(SOUND_WIN32)
  AM_CONDITIONAL([SOUND_WIN32], [true])
  ;;
beos*|haiku*)
  AC_DEFINE(SOUND_BEOS)
  CFLAGS="${CFLAGS} -Wno-multichar"
  AM_CONDITIONAL([SOUND_BEOS], [true])
  ;;
*)
  AS_IF([test "$ac_cv_header_sys_audioio_h" = "yes"], [
    AC_DEFINE([SOUND_BSD])
    AM_CONDITIONAL([SOUND_BSD], [true])
  ])
  ;;
esac

XMP_TRY_COMPILE(whether compiler understands -Wall,
  ac_cv_c_flag_w_all,
  -Wall,[
  int main(){}],
  CFLAGS="${CFLAGS} -Wall") 

XMP_TRY_COMPILE(whether compiler understands -Wunused-result,
  ac_cv_c_flag_w_unused_result,
  -Wunused-result,[
  int main(){}],
  CFLAGS="${CFLAGS} -Wno-unused-result")  

AC_CHECK_FUNCS(kill getopt_long)
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
