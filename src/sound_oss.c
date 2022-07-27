/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/*
 * devfs /dev/sound/dsp support by Dirk Jagdmann
 * resume/onpause by Test Rat <ttsestt@gmail.com>
 */

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#if defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#endif

#include "sound.h"

#ifndef AFMT_U16_NE
#  if AFMT_S16_NE == AFMT_S16_LE
#    define AFMT_U16_NE AFMT_U16_LE
#  else
#    define AFMT_U16_NE AFMT_U16_BE
#  endif
#endif

struct sound_driver sound_oss;

static int audio_fd;

static int fragnum, fragsize;
static int do_sync = 1;

static int to_fmt(int format)
{
	int fmt;

	if (format & XMP_FORMAT_8BIT)
		fmt = AFMT_U8 | AFMT_S8;
	else {
		fmt = AFMT_S16_NE | AFMT_U16_NE;
	}

	if (format & XMP_FORMAT_UNSIGNED)
		fmt &= AFMT_U8 | AFMT_U16_LE | AFMT_U16_BE;
	else
		fmt &= AFMT_S8 | AFMT_S16_LE | AFMT_S16_BE;

	return fmt;
}

static int from_fmt(int fmt)
{
	int format = 0;

	if (!(fmt & (AFMT_S16_LE | AFMT_S16_BE | AFMT_U16_LE | AFMT_U16_BE))) {
		format |= XMP_FORMAT_8BIT;
	}

	if (fmt & (AFMT_U8 | AFMT_U16_LE | AFMT_U16_BE)) {
		format |= XMP_FORMAT_UNSIGNED;
	}

	return format;
}

static void setaudio(int *rate, int *format)
{
	static int fragset = 0;
	int frag = 0;
	int fmt;

	frag = (fragnum << 16) + fragsize;

	fmt = to_fmt(*format);
	ioctl(audio_fd, SNDCTL_DSP_SETFMT, &fmt);
	*format = from_fmt(fmt);

	fmt = !(*format & XMP_FORMAT_MONO);
	ioctl(audio_fd, SNDCTL_DSP_STEREO, &fmt);
	if (fmt) {
		*format &= ~XMP_FORMAT_MONO;
	} else {
		*format |= XMP_FORMAT_MONO;
	}

	ioctl(audio_fd, SNDCTL_DSP_SPEED, rate);

	/* Set the fragments only once */
	if (!fragset) {
		if (fragnum && fragsize)
			ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag);
		fragset++;
	}
}

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	static const char *dev_audio[] = {
#ifdef DEVOSSAUDIO
		DEVOSSAUDIO,	/* pkgsrc */
#endif
		"/dev/dsp",
		"/dev/sound/dsp",
		"/dev/audio"	/* NetBSD and SunOS */
	};
	audio_buf_info info;
	static char buf[80];
	int i;

	fragnum = 16;		/* default number of fragments */
	i = 1024;		/* default size of fragment */

	parm_init(parm);
	chkparm2("frag", "%d,%d", &fragnum, &i);
	chkparm1("dev", dev_audio[0] = token);
	chkparm0("nosync", do_sync = 0);
	parm_end();

	for (fragsize = 0; i >>= 1; fragsize++) ;
	if (fragsize < 4)
		fragsize = 4;

	for (i = 0; i < sizeof(dev_audio) / sizeof(dev_audio[0]); i++)
		if ((audio_fd = open(dev_audio[i], O_WRONLY)) >= 0)
			break;
	if (audio_fd < 0)
		return -1;

	setaudio(&options->rate, &options->format);

	if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info) == 0) {
		snprintf(buf, 80, "%s [%d fragments of %d bytes]",
			 sound_oss.description, info.fragstotal,
			 info.fragsize);
		sound_oss.description = buf;
	}

	return 0;
}

/* Build and write one tick (one PAL frame or 1/50 s in standard vblank
 * timed mods) of audio data to the output device.
 */
static void play(void *b, int i)
{
	int j;

	while (i) {
		if ((j = write(audio_fd, b, i)) > 0) {
			i -= j;
			b = (char *)b + j;
		} else
			break;
	};
}

static void deinit(void)
{
	ioctl(audio_fd, SNDCTL_DSP_RESET, NULL);
	close(audio_fd);
}

static void flush(void)
{
}

static void onpause(void)
{
#ifdef SNDCTL_DSP_SETTRIGGER
	int trig = 0;
	ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &trig);
#else
	ioctl(audio_fd, SNDCTL_DSP_RESET, NULL);
#endif

	if (do_sync)
		ioctl(audio_fd, SNDCTL_DSP_SYNC, NULL);
}

static void onresume(void)
{
#ifdef SNDCTL_DSP_SETTRIGGER
	int trig = PCM_ENABLE_OUTPUT;
	ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &trig);
#endif
}

static const char *const help[] = {
	"frag=num,size", "Set the number and size of fragments",
	"dev=<device_name>", "Audio device to use (default /dev/dsp)",
	"nosync", "Don't flush OSS buffers between modules",
	NULL
};

struct sound_driver sound_oss = {
	"oss",
	"OSS PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
