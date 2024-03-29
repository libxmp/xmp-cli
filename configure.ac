AC_INIT([xmp], [4.2.0])
AM_SILENT_RULES([yes])
AC_CONFIG_AUX_DIR([build-aux])
0>confdefs.h

AC_ARG_ENABLE(oss,
  [  --disable-oss           don't compile OSS support])
AC_ARG_ENABLE(alsa,
  [  --disable-alsa          don't compile ALSA support])
AC_ARG_ENABLE(pulseaudio,
  [  --enable-pulseaudio     compile PulseAudio support])
AC_ARG_ENABLE(sndio,
  [  --disable-sndio         don't compile sndio support])

AC_SUBST(LD_VERSCRIPT)
AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
case "$host_os" in
beos*|haiku*)
	AS_IF([! which "$CXX" >/dev/null 2>/dev/null],
		[AC_MSG_ERROR([C++ compiler required on this platform ($host_os)])])
	CXXLD='$(CXX)'
	;;
*)
	# Do not error out on linking when g++ is absent.
	CXXLD='$(CCLD)'
	;;
esac
AC_SUBST([CXXLD])

AC_PROG_INSTALL
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_ARG_WITH([libxmp-lite],
  AS_HELP_STRING([--with-libxmp-lite],[build against libxmp-lite instead of libxmp]),
  [xmplib="$withval"],[xmplib=xmp])
case "$xmplib" in
  no|xmp) xmplib=xmp ;;
  *) xmplib=xmp-lite ;;
esac

old_LIBS="${LIBS}"
old_CPPFLAGS="${CPPFLAGS}"
PKG_CHECK_MODULES([LIBXMP], [lib$xmplib >= 4.4],
  AC_MSG_CHECKING(linkage to lib$xmplib)
  LIBS="${LIBXMP_LIBS}"
  CPPFLAGS="${CPPFLAGS} ${LIBXMP_CFLAGS}"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <xmp.h>]], [[return xmp_set_player((char*)0,0,0);]])],
    [LIBS="${old_LIBS}"
     AC_MSG_RESULT(success)],
dnl In case libxmp is a static build and depend on libm:
dnl PKG_CHECK_MODULES_STATIC might not be present, so doing it
dnl manually:
 [AC_MSG_RESULT(failed:)
  AC_MSG_CHECKING(linkage to lib$xmplib with -lm)
  LIBS="${LIBS} -lm"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <xmp.h>]], [[return xmp_set_player((char*)0,0,0);]])],
    [LIBS="${old_LIBS} -lm"
     AC_MSG_RESULT(success)],
    [AC_MSG_RESULT(failed)
     AC_MSG_FAILURE(linkage to lib$xmplib failed, 1)])]
  ),
 [echo "You need libxmp version 4.4 or later to build this package"; exit 1]
)
CPPFLAGS="${old_CPPFLAGS}"

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

AC_CHECK_HEADERS([sys/types.h unistd.h getopt.h signal.h sys/select.h sys/time.h sys/audioio.h])
case $host_os in
dnl don't check half-baked termios for amiga or dos targets.
  amigaos*|aros*|morphos*|*djgpp) ;;
  *) AC_CHECK_HEADERS([termios.h]) ;;
esac

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
AM_CONDITIONAL([SOUND_SB], [false])
AM_CONDITIONAL([SOUND_SGI], [false])
AM_CONDITIONAL([SOUND_SNDIO], [false])
AM_CONDITIONAL([SOUND_SOLARIS], [false])
AM_CONDITIONAL([SOUND_OS2DART], [false])
AM_CONDITIONAL([SOUND_WIN32], [false])

if test "${enable_oss}" != "no"; then
  AC_CHECK_HEADERS(sys/soundcard.h machine/soundcard.h)
  if test "${ac_cv_header_sys_soundcard_h}" = "yes" -o "${ac_cv_header_machine_soundcard_h}" = "yes"; then
    AC_DEFINE(SOUND_OSS, 1, [ ])
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
  AC_DEFINE(SOUND_ALSA, 1, [ ])
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
  AC_DEFINE(SOUND_PULSEAUDIO, 1, [ ])
  AM_CONDITIONAL([SOUND_PULSEAUDIO], [true])
])

if test "${enable_sndio}" != "no"; then
  AC_CHECK_HEADER(sndio.h)
  if test "${ac_cv_header_sndio_h}" = "yes"; then
    AC_DEFINE(SOUND_SNDIO, 1, [ ])
    AM_CONDITIONAL([SOUND_SNDIO], [true])
  fi
fi

case "${host_os}" in
amigaos*|aros*|morphos*)
  AC_DEFINE(SOUND_AHI, 1, [ ])
  AM_CONDITIONAL([SOUND_AHI], [true])
  ;;
darwin*)
  AC_CHECK_HEADERS(AudioUnit/AudioComponent.h CoreAudio/CoreAudio.h)
  if test "${ac_cv_header_CoreAudio_CoreAudio_h}" = "yes"; then
    AC_DEFINE(SOUND_COREAUDIO, 1, [ ])
    AM_CONDITIONAL([SOUND_COREAUDIO], [true])
    XMP_DARWIN_LDFLAGS="-framework AudioToolbox -framework AudioUnit -framework CoreServices"
    AC_SUBST([XMP_DARWIN_LDFLAGS])
  fi
  ;;
netbsd*)
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_NETBSD, 1, [ ])
    AM_CONDITIONAL([SOUND_NETBSD], [true])
  fi
  if test "${ac_cv_header_sys_soundcard_h}" = "yes"; then
    AC_CHECK_LIB(ossaudio, _oss_ioctl)
  fi
  ;;
solaris*)
  if test "${ac_cv_header_sys_audioio_h}" = "yes"; then
    AC_DEFINE(SOUND_SOLARIS, 1, [ ])
    AM_CONDITIONAL([SOUND_SOLARIS], [true])
  fi
  ;;
hpux*)
  AC_CHECK_HEADER(sys/audio.h)
  if test "${ac_cv_header_sys_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_HPUX, 1, [ ])
    AM_CONDITIONAL([SOUND_HPUX], [true])
  fi
  ;;
aix*)
  AC_CHECK_HEADER(audio.h)
  if test "${ac_cv_header_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_AIX, 1, [ ])
    AM_CONDITIONAL([SOUND_AIX], [true])
  fi
  ;;
irix*)
  AC_CHECK_HEADERS(dmedia/audio.h)
  if test "${ac_cv_header_dmedia_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_SGI, 1, [ ])
    AM_CONDITIONAL([SOUND_SGI], [true])
  fi
  ;;
qnx*)
  AC_CHECK_HEADERS(sys/audio.h)
  if test "${ac_cv_header_sys_audio_h}" = "yes"; then
    AC_DEFINE(SOUND_QNX, 1, [ ])
    AM_CONDITIONAL([SOUND_QNX], [true])
  fi
  ;;
nto-qnx*)
  AC_CHECK_HEADERS(sys/asoundlib.h)
  if test "${ac_cv_header_sys_asoundlib_h}" = "yes"; then
    AC_DEFINE(SOUND_ALSA05, 1, [ ])
    AM_CONDITIONAL([SOUND_ALSA05], [true])
  fi
  ;;
cygwin*|mingw*)
  AC_DEFINE(SOUND_WIN32, 1, [ ])
  AM_CONDITIONAL([SOUND_WIN32], [true])
  ;;
*djgpp)
  AC_DEFINE(SOUND_SB, 1, [ ])
  AM_CONDITIONAL([SOUND_SB], [true])
  ;;
beos*|haiku*)
  AC_DEFINE(SOUND_BEOS, 1, [ ])
  CFLAGS="${CFLAGS} -Wno-multichar"
  AM_CONDITIONAL([SOUND_BEOS], [true])
  ;;
*)
dnl Modern OpenBSD has sys/audioio.h but does not support this driver.
  AS_IF([test "$ac_cv_header_sys_audioio_h" = "yes"], [
    AC_CHECK_MEMBER([audio_info_t.play],
      [ AC_DEFINE([SOUND_BSD], 1, [ ])
        AM_CONDITIONAL([SOUND_BSD], [true])
      ], , [[#include <sys/audioio.h>]])
  ])
  ;;
esac

XMP_TRY_COMPILE(whether compiler understands -Wall,
  ac_cv_c_flag_w_all,
  -Wall,[
  int main(){}],
  CFLAGS="${CFLAGS} -Wall")

XMP_TRY_COMPILE(whether compiler understands -Wwrite-strings,
  ac_cv_c_flag_w_write_strings,
  -Wwrite-strings,[
  int main(){}],
  CFLAGS="${CFLAGS} -Wwrite-strings")

XMP_TRY_COMPILE(whether compiler understands -Wunused-result,
  ac_cv_c_flag_w_unused_result,
  -Wunused-result,[
  int main(){}],
  CFLAGS="${CFLAGS} -Wno-unused-result")

AC_CHECK_FUNCS(kill getopt_long usleep select)
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
