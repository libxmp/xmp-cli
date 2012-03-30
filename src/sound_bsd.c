/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sound.h"

static int audio_fd;

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

	if (gain < AUDIO_MIN_GAIN)
		gain = AUDIO_MIN_GAIN;
	if (gain > AUDIO_MAX_GAIN)
		gain = AUDIO_MAX_GAIN;

	if ((audio_fd = open("/dev/sound", O_WRONLY)) == -1)
		return -1;

	AUDIO_INITINFO(&ainfo);

	ainfo.play.sample_rate = options->rate;
	ainfo.play.channels = options->format & XMP_FORMAT_MONO ? 1 : 2;
	ainfo.play.gain = gain;
	ainfo.play.buffer_size = bsize;

	if (options->format & XMP_FORMAT_8BIT) {
		ainfo.play.precision = 8;
		ainfo.play.encoding = AUDIO_ENCODING_ULINEAR;
		options->format |= XMP_FORMAT_UNSIGNED;
	} else {
		ainfo.play.precision = 16;
		ainfo.play.encoding = AUDIO_ENCODING_SLINEAR;
		options->format &= ~XMP_FORMAT_UNSIGNED;
	}

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

static void deinit()
{
	close(audio_fd);
}

static void flush()
{
}

static void onpause()
{
}

static void onresume()
{
}

static char *help[] = {
	"gain=val", "Audio output gain (0 to 255)",
	"buffer=val", "Audio buffer size (default is 32768)",
	NULL
};

struct sound_driver sound_bsd = {
	"bsd",
	"BSD PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
