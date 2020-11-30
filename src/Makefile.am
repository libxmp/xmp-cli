# -*- Makefile -*-

AM_CPPFLAGS = -DSYSCONFDIR=\"${sysconfdir}\" ${LIBXMP_CFLAGS} \
              ${alsa_CFLAGS} ${pulseaudio_CFLAGS}
AM_CFLAGS   = -Wall

bin_PROGRAMS = xmp

xmp_SOURCES = \
	commands.c common.h getopt_long.h list.h getopt_long.c info.c main.c \
	options.c read_config.c sound.c sound.h sound_file.c sound_null.c \
	sound_wav.c sound_aiff.c terminal.c
xmp_LDADD   = ${LIBXMP_LIBS}
xmp_LDFLAGS = ${XMP_DARWIN_LDFLAGS}

if SOUND_OSS
xmp_SOURCES += sound_oss.c
endif

if SOUND_ALSA
xmp_SOURCES += sound_alsa.c
xmp_LDADD   += ${alsa_LIBS}
endif

if SOUND_PULSEAUDIO
xmp_SOURCES += sound_pulseaudio.c
xmp_LDADD   += ${pulseaudio_LIBS}
endif

if SOUND_AHI
xmp_SOURCES += sound_ahi.c
endif

if SOUND_COREAUDIO
xmp_SOURCES += sound_coreaudio.c
endif

if SOUND_SNDIO
xmp_SOURCES += sound_sndio.c
xmp_LDADD   += -lsndio
endif

if SOUND_BSD
xmp_SOURCES += sound_bsd.c
endif

if SOUND_NETBSD
xmp_SOURCES += sound_netbsd.c
endif

if SOUND_SOLARIS
xmp_SOURCES += sound_solaris.c
endif

if SOUND_HPUX
xmp_SOURCES += sound_hpux.c
endif

if SOUND_AIX
xmp_SOURCES += sound_aix.c
endif

if SOUND_SGI
xmp_SOURCES += sound_sgi.c
xmp_LDADD   += -laudio
endif

if SOUND_QNX
xmp_SOURCES += sound_qnx.c
endif

if SOUND_ALSA05
xmp_SOURCES += sound_alsa05.c
xmp_LDADD   += -lasound
endif

if SOUND_WIN32
xmp_SOURCES += sound_win32.c
xmp_LDADD   += -lwinmm
endif

if SOUND_BEOS
xmp_SOURCES += sound_beos.cpp
xmp_LDADD   += -lmedia -lbe
endif

man_MANS = xmp.1

pkgsysconfdir   = ${sysconfdir}/${PACKAGE_NAME}
pkgsysconf_DATA = modules.conf xmp.conf

EXTRA_DIST = ${man_MANS} ${pkgsysconf_DATA} win32/usleep.c win32/unistd.h

# unused sources
EXTRA_DIST += sound_dart.c
