/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/* Based on bsd.c and solaris.c */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sound.h"

static int audio_fd;
static int audioctl_fd;


static int init(struct options *options)
{
	char **parm = options->driver_parm;
	audio_info_t ainfo;
	int gain = 128;
	int bsize = 32 * 1024;

	parm_init(parm);
	chkparm1("gain", gain = strtoul(token, NULL, 0));
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	parm_end();

	if ((audio_fd = open("/dev/audio", O_WRONLY)) == -1)
		return -1;

	/* try to open audioctldevice */
	if ((audioctl_fd = open("/dev/audioctl", O_RDWR)) < 0) {
		fprintf(stderr, "couldn't open audioctldevice\n");
		close(audio_fd);
		return -1;
	}

	/* empty buffers before change config */
	ioctl(audio_fd, AUDIO_DRAIN, 0);	/* drain everything out */
	ioctl(audio_fd, AUDIO_FLUSH);		/* flush audio */
	ioctl(audioctl_fd, AUDIO_FLUSH);	/* flush audioctl */

	/* get audio parameters. */
	if (ioctl(audioctl_fd, AUDIO_GETINFO, &ainfo) < 0) {
		fprintf(stderr, "AUDIO_GETINFO failed!\n");
		close(audio_fd);
		close(audioctl_fd);
		return -1;
	}

	close(audioctl_fd);

	if (gain < AUDIO_MIN_GAIN)
		gain = AUDIO_MIN_GAIN;
	if (gain > AUDIO_MAX_GAIN)
		gain = AUDIO_MAX_GAIN;

	AUDIO_INITINFO(&ainfo);

	ainfo.mode = AUMODE_PLAY_ALL;
	ainfo.play.sample_rate = options->rate;
	ainfo.play.channels = options->format & XMP_FORMAT_MONO ? 1 : 2;

	if (options->format & XMP_FORMAT_8BIT) {
		ainfo.play.precision = 8;
		ainfo.play.encoding = AUDIO_ENCODING_ULINEAR;
		options->format |= XMP_FORMAT_UNSIGNED;
	} else {
		ainfo.play.precision = 16;
		ainfo.play.encoding = AUDIO_ENCODING_SLINEAR;
		options->format &= ~XMP_FORMAT_UNSIGNED;
	}

	ainfo.play.gain = gain;
	ainfo.play.buffer_size = bsize;
	ainfo.blocksize = 0;

	if (ioctl(audio_fd, AUDIO_SETINFO, &ainfo) == -1) {
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
	return "NetBSD PCM audio";
}

static const char *const help[] = {
	"gain=val", "Audio output gain (0 to 255)",
	"buffer=val", "Audio buffer size (default is 32768)",
	NULL
};

const struct sound_driver sound_netbsd = {
	"netbsd",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
