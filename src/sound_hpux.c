/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/* Tested on a 9000/710 running HP-UX 9.05 with 8 kHz, 16 bit mono output.  */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/audio.h>
#include <fcntl.h>
#include "sound.h"

static int audio_fd;

/* Standard sampling rates */
static const int srate[] = {
	44100, 32000, 22050, 16000, 11025, 8000, 0
};


static int init(struct options *options)
{
	char **parm = options->driver_parm;
	int flags;
	int gain = 128;
	int bsize = 32 * 1024;
	int port = AUDIO_OUT_SPEAKER;
	int nch = options->format & XMP_FORMAT_MONO ? 1 : 2;
	struct audio_gains agains;
	struct audio_describe adescribe;
	int i;

	parm_init(parm);
	chkparm1("gain", gain = strtoul(token, NULL, 0));
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	chkparm1("port", port = (int)*token)
	parm_end();

	switch (port) {
	case 'h':
		port = AUDIO_OUT_HEADPHONE;
		break;
	case 'l':
		port = AUDIO_OUT_LINE;
		break;
	default:
		port = AUDIO_OUT_SPEAKER;
	}

	if ((audio_fd = open("/dev/audio", O_WRONLY)) == -1)
		goto err;

	if ((flags = fcntl(audio_fd, F_GETFL, 0)) < 0)
		goto err1;

	flags |= O_NDELAY;
	if ((flags = fcntl(audio_fd, F_SETFL, flags)) < 0)
		goto err1;

	options->format &= ~XMP_FORMAT_8BIT;
	if (ioctl(audio_fd, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT) == -1)
		goto err1;

	if (ioctl(audio_fd, AUDIO_SET_CHANNELS, nch) == -1) {
		options->format ^= XMP_FORMAT_MONO;
		nch = options->format & XMP_FORMAT_MONO ? 1 : 2;

		if (ioctl(audio_fd, AUDIO_SET_CHANNELS, nch) == -1) {
			goto err1;
		}
	}

	ioctl(audio_fd, AUDIO_SET_OUTPUT, port);

	for (i = 0; ; i++) {
		if (ioctl(audio_fd, AUDIO_SET_SAMPLE_RATE, options->rate) == 0)
			break;

		if ((options->rate = srate[i]) == 0)
			goto err1;
	}

	if (ioctl(audio_fd, AUDIO_DESCRIBE, &adescribe) == -1)
		goto err1;

	if (ioctl(audio_fd, AUDIO_GET_GAINS, &agains) == -1)
		goto err1;

	agains.transmit_gain = adescribe.min_transmit_gain +
		(adescribe.max_transmit_gain - adescribe.min_transmit_gain) *
		gain / 256;

	if (ioctl(audio_fd, AUDIO_SET_GAINS, &agains) == -1)
		goto err1;

	ioctl(audio_fd, AUDIO_SET_TXBUFSIZE, bsize);

	return 0;

    err1:
	close(audio_fd);
    err:
	return -1;
}

static void play(void *b, int i)
{
	int j;

	while (i) {
		if ((j = write(audio_fd, b, i)) > 0) {
			i -= j;
			b = (char *)b + j;
		} else
			break;
	}
}

static void deinit(void)
{
	close(audio_fd);
}

static void flush(void)
{
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char *description(void)
{
	return "HP-UX PCM audio";
}

static const char *const help[] = {
	"gain=val", "Audio output gain (0 to 255)",
	"port={s|h|l}", "Audio port (s[peaker], h[eadphones], l[ineout])",
	"buffer=val", "Audio buffer size",
	NULL
};

const struct sound_driver sound_hpux = {
	"hpux",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

