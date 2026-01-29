/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
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
static int bits;


#define MATCH_ENCODING(s, r) \
	((r) == AUDIO_ENCODING_ULINEAR ? \
	  ((s) == AUDIO_ENCODING_ULINEAR || (s) == native_unsigned) : \
	  ((s) == AUDIO_ENCODING_SLINEAR || (s) == native_signed))

#define ANY_LINEAR(s) \
	((s) == AUDIO_ENCODING_ULINEAR || (s) == native_unsigned || \
	 (s) == AUDIO_ENCODING_SLINEAR || (s) == native_signed)

static void to_fmt(int *precision, int *encoding, const struct options *options)
{
	/* The NE defines only exist in kernel space for some reason... */
	int native_signed = is_big_endian()   ? AUDIO_ENCODING_SLINEAR_BE :
						AUDIO_ENCODING_SLINEAR_LE;
	int native_unsigned = is_big_endian() ? AUDIO_ENCODING_ULINEAR_BE :
						AUDIO_ENCODING_ULINEAR_LE;

	audio_encoding_t enc;
	int match_precision = -1;
	int match_encoding = -1;
	int depth, uns, i;

	depth = get_bits_from_format(options);
	uns = get_signed_from_format(options) ? AUDIO_ENCODING_SLINEAR :
						AUDIO_ENCODING_ULINEAR;

	memset(&enc, 0, sizeof(enc));

	for (i = 0; i < 100; i++) {
		enc.index = i;
		if (ioctl(audioctl_fd, AUDIO_GETENC, &enc) < 0) {
			break;
		}
		if (enc.precision < depth ||
		    (enc.precision > depth && match_precision == depth) ||
		    !ANY_LINEAR(enc.encoding)) {
			continue;
		}
		if (enc.precision == depth || MATCH_ENCODING(enc.encoding, uns)) {
			match_precision = enc.precision;
			match_encoding = enc.encoding;
		}
		if (enc.precision == depth && MATCH_ENCODING(enc.encoding, uns)) {
			break;
		}
	}
	if (match_precision >= 0) {
		*precision = match_precision;
		*encoding = match_encoding;
	}
}

static int is_signed(int encoding)
{
	return encoding == AUDIO_ENCODING_SLINEAR ||
		encoding == AUDIO_ENCODING_SLINEAR_BE ||
		encoding == AUDIO_ENCODING_SLINEAR_LE;
}

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	audio_info_t ainfo;
	int gain = 128;
	int bsize = 32 * 1024;
	int precision = 16;
	int encoding = AUDIO_ENCODING_SLINEAR;

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

	to_fmt(&precision, &encoding, options);

	close(audioctl_fd);

	if (gain < AUDIO_MIN_GAIN)
		gain = AUDIO_MIN_GAIN;
	if (gain > AUDIO_MAX_GAIN)
		gain = AUDIO_MAX_GAIN;

	AUDIO_INITINFO(&ainfo);

	ainfo.mode = AUMODE_PLAY_ALL;
	ainfo.play.sample_rate = options->rate;
	ainfo.play.channels = get_channels_from_format(options);
	ainfo.play.precision = precision;
	ainfo.play.encoding = encoding;
	ainfo.play.gain = gain;
	ainfo.play.buffer_size = bsize;
	ainfo.blocksize = 0;

	if (ioctl(audio_fd, AUDIO_SETINFO, &ainfo) == -1) {
		close(audio_fd);
		return -1;
	}

	update_format_bits(options, precision);
	update_format_signed(options, is_signed(encoding));
	bits = precision;

	return 0;
}

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
