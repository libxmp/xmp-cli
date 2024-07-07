/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/*
 * Based on the QNX4 port of nspmod by Mike Gorchak <malva@selena.kherson.ua>
 */

#include <sys/audio.h>
#include <sys/ioctl.h>
#include "sound.h"

static int fd_audio;

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	int rc, rate, bits, stereo, bsize;
	const char *dev;

	parm_init(parm);
	chkparm1("dev", dev = token);
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	parm_end();

	rate = options->rate;
	bits = options->format & XMP_FORMAT_8BIT ? 8 : 16;
	stereo = 1;
	bufsize = 32 * 1024;

	fd_audio = open(dev, O_WRONLY);
	if (fd_audio < 0) {
		fprintf(stderr, "can't open audio device\n");
		return -1;
	}

	if (options->outfmt & XMP_FORMAT_MONO)
		stereo = 0;

	if (ioctl(fd_audio, SOUND_PCM_WRITE_BITS, &bits) < 0) {
		perror("can't set resolution");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_STEREO, &stereo) < 0) {
		perror("can't set channels");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) < 0) {
		perror("can't set rate");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &buf.size) < 0) {
		perror("can't set rate");
		goto error;
	}

	if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &bufsize) < 0) {
		perror("can't set buffer");
		goto error;
	}

	return 0;

    error:
	close(fd_audio);
	return -1;
}

static void play(void *b, int i)
{
	int j;

	do {
		if ((j = write(fd_audio, b, i)) > 0) {
			i -= j;
			b = (char *)b + j;
		} else {
			break;
		}
	} while (i);
}

static void deinit(void)
{
	close(fd_audio);
}

static void flush(void)
{
	ioctl(fd, SNDCTL_DSP_SYNC, NULL);
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char *description(void)
{
	return "QNX PCM audio";
}

static const char *const help[] = {
	"dev=<device_name>", "Audio device name (default is /dev/dsp)",
	"buffer=val", "Audio buffer size (default is 32768)",
	NULL
};

const struct sound_driver sound_qnx = {
	"QNX",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
