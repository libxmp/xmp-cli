/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/*
 * Based on the AIX XMMS output plugin by Peter Alm, Thomas Nilsson
 * and Olle Hallnas.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/audio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sound.h"

static int audio_fd;
static audio_control control;
static audio_change change;

#define AUDIO_MIN_GAIN 0
#define AUDIO_MAX_GAIN 100


static int init(struct options *options)
{
	char **parm = options->driver_parm;
	audio_init ainit;
	int gain = 128;
	int bsize = 32 * 1024;

	parm_init(parm);
	chkparm1("gain", gain = strtoul(token, NULL, 0));
	/* chkparm1 ("buffer", bsize = strtoul(token, NULL, 0)); */
	parm_end();

	if (gain < AUDIO_MIN_GAIN)
		gain = AUDIO_MIN_GAIN;
	if (gain > AUDIO_MAX_GAIN)
		gain = AUDIO_MAX_GAIN;

	if ((audio_fd = open("/dev/paud0/1", O_WRONLY)) == -1)
		return -1;

	init.mode = PCM;		/* audio format */
	init.srate = options->rate;	/* sample rate */
	init.operation = PLAY;		/* PLAY or RECORD */
	init.channels = options->format & XMP_FORMAT_MONO ? 1 : 2;
	init.bits_per_sample = options->format & XMP_FORMAT_8BIT ? 8 : 16;
	init.flags = BIG_ENDIAN | TWOS_COMPLEMENT;

	if (ioctl(audio_fd, AUDIO_INIT, &init) < 0) {
		close(audio_fd);
		return -1;
	}

	/* full blast; range: 0-0x7fffffff */
	change.volume = 0x7fffffff * (1.0 * gain / 200.0);
	change.monitor = AUDIO_IGNORE;	/* monitor what's recorded ? */
	change.input = AUDIO_IGNORE;	/* input to record from */
	change.output = OUTPUT_1;	/* line-out */
	change.balance = 0x3FFFFFFF;

	control.ioctl_request = AUDIO_CHANGE;
	control.request_info = (char *)&change;
	if (ioctl(audio_fd, AUDIO_CONTROL, &control) < 0) {
		close(audio_fd);
		return -1;
	}

	/* start playback - won't actually start until write() calls occur */
	control.ioctl_request = AUDIO_START;
	control.position = 0;
	if (ioctl(audio_fd, AUDIO_CONTROL, &control) < 0) {
		close(audio_fd);
		return -1;
	}
	return 0;
}

static void play(void *b, int i)
{
	int j;

	while (i) {
		if ((j = write(audio_fd, b, i)) > 0) {
			i -= j;
			(char *)b += j;
		} else
			break;
	};
}

static void deinit(void)
{
	control.ioctl_request = AUDIO_STOP;
	ioctl(audio_fd, AUDIO_CONTROL, &control);
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

static const char *const help[] = {
	"gain=val", "Audio output gain (0 to 255)",
	/* "buffer=val", "Audio buffer size (default is 32768)", */
	NULL
};

struct sound_driver sound_bsd = {
	"aix",
	"AIX PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

