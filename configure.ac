dnl AC_CONFIG_AUX_DIR(./scripts)
AC_INIT
0>confdefs.h

AC_ARG_WITH(libxmp, [  --with-libxmp=<path>    libxmp prefix (optional)],
  libxmp_path="$withval")

AC_PROG_CC

if test "$libxmp_path" != ""; then
  CPPFLAGS="${CPPFLAGS} -I${libxmp_path}/include"
  LIBS="${LIBS} -L${libxmp_path}/lib"
fi

AC_CHECK_HEADERS(xmp.h getopt.h signal.h termios.h)
AC_CHECK_HEADER(windows.h,,[
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h,,[
    AC_CHECK_HEADER(alsa/asoundlib.h)
  ])
])

if test "${ac_cv_header_xmp_h}" = "yes"; then
  AC_CHECK_LIB(xmp,xmp_player_start,,
    AC_MSG_ERROR(Can't find libxmp))
else
  AC_MSG_ERROR(Can't find libxmp header files)
fi

if test "${ac_cv_header_windows_h}" = "yes"; then
  AC_SUBST(SOUND_DRIVER,sound_win32)
  LIBS="${LIBS} -lwinmm"
elif test "${ac_cv_header_CoreAudio_CoreAudio_h}" = "yes"; then
  AC_SUBST(SOUND_DRIVER,sound_coreaudio)
  LIBS="${LIBS} -framework AudioToolbox -framework AudioUnit -framework CoreServices"
elif test "${ac_cv_header_alsa_asoundlib_h}" = "yes"; then
  AC_SUBST(SOUND_DRIVER,sound_alsa)
  AC_CHECK_LIB(asound,snd_pcm_open,,
    AC_MSG_ERROR(Can't find libasound))
fi

AC_CHECK_FUNCS(kill getopt_long)

AC_PROG_INSTALL
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
