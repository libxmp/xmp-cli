/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
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

static int audio_fd;

static int fragnum, fragsize;
static int do_sync = 1;
static int bits;

static const char *desc_default = "OSS PCM audio";
static char descbuf[80] = {0};

static int to_fmt(const struct options *options)
{
	int fmt;

	switch (get_bits_from_format(options)) {
	case 8:
		fmt = AFMT_U8 | AFMT_S8;
		break;
	case 16:
	default:
		fmt = AFMT_S16_NE | AFMT_U16_NE;
		break;
#if defined(AFMT_S24_NE)
	case 24:
		return AFMT_S24_NE;
#endif
#if defined(AFMT_S32_NE)
	case 32:
		return AFMT_S32_NE;
#endif
	}

	if (!get_signed_from_format(options))
		fmt &= AFMT_U8 | AFMT_U16_LE | AFMT_U16_BE;
	else
		fmt &= AFMT_S8 | AFMT_S16_LE | AFMT_S16_BE;

	return fmt;
}

static void from_fmt(struct options *options, int fmt, int channels)
{
	int sgn = 1;
	bits = 16;

#if defined(AFMT_S32_NE)
	if (fmt == AFMT_S32_NE) {
		bits = 32;
	} else
#endif
#if defined(AFMT_S24_NE)
	if (fmt == AFMT_S24_NE) {
		bits = 24;
	} else
#endif
	if (!(fmt & (AFMT_S16_LE | AFMT_S16_BE | AFMT_U16_LE | AFMT_U16_BE))) {
		bits = 8;
	}

	if (fmt & (AFMT_U8 | AFMT_U16_LE | AFMT_U16_BE)) {
		sgn = 0;
	}
	update_format_bits(options, bits);
	update_format_signed(options, sgn);
	update_format_channels(options, channels);
}

static void setaudio(struct options *options)
{
	static int fragset = 0;
	int frag = 0;
	int stereo;
	int fmt;

	frag = (fragnum << 16) + fragsize;

	fmt = to_fmt(options);
	ioctl(audio_fd, SNDCTL_DSP_SETFMT, &fmt);

	stereo = get_channels_from_format(options) > 1 ? 1 : 0;
	ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo);

	ioctl(audio_fd, SNDCTL_DSP_SPEED, &options->rate);

	from_fmt(options, fmt, stereo ? 2 : 1);

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

	setaudio(options);

	if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info) == 0) {
		snprintf(descbuf, sizeof(descbuf), "%s [%d fragments of %d bytes]",
			 desc_default, info.fragstotal,
			 info.fragsize);
	}

	return 0;
}

/* Build and write one tick (one PAL frame or 1/50 s in standard vblank
 * timed mods) of audio data to the output device.
 */
static void play(void *b, int i)
{
	int j;

	if (bits == 24) {
		downmix_32_to_24_aligned(b, i);
	}

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
	descbuf[0] = 0;
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

static const char *description(void)
{
	if (descbuf[0])
		return descbuf;
	return desc_default;
}

static const char *const help[] = {
	"frag=num,size", "Set the number and size of fragments",
	"dev=<device_name>", "Audio device to use (default /dev/dsp)",
	"nosync", "Don't flush OSS buffers between modules",
	NULL
};

const struct sound_driver sound_oss = {
	"oss",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
