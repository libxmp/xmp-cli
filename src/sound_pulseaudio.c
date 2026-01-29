/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <unistd.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include "sound.h"

static pa_simple *s;
static int bits;


static int init(struct options *options)
{
	pa_sample_spec ss;
	int format = -1;
	int error;

	bits = get_bits_from_format(options);
	switch (bits) {
	case 8:
		format = PA_SAMPLE_U8;
		break;
	case 16:
	default:
		format = PA_SAMPLE_S16NE;
		break;
#ifdef PA_SAMPLE_S24_32NE
	case 24:
		format = PA_SAMPLE_S24_32NE;
		break;
#endif
#ifdef PA_SAMPLE_S32NE
	case 32:
		format = PA_SAMPLE_S32NE;
		bits = 32;
		break;
#endif
	}

	ss.format = format;
	ss.channels = get_channels_from_format(options);
	ss.rate = options->rate;

	s = pa_simple_new(NULL,		/* Use the default server */
		"xmp",			/* Our application's name */
		PA_STREAM_PLAYBACK,
		NULL,			/* Use the default device */
		"Music",		/* Description of our stream */
		&ss,			/* Our sample format */
		NULL,			/* Use default channel map */
		NULL,			/* Use default buffering attributes */
		&error);		/* Ignore error code */

	if (s == NULL) {
		fprintf(stderr, "pulseaudio error: %s\n", pa_strerror(error));
		return -1;
	}

	update_format_bits(options, bits);
	update_format_signed(options, bits != 8);
	return 0;
}

static void play(void *b, int i)
{
	int j, error;

	if (bits == 24) {
		downmix_32_to_24_aligned(b, i);
	}

	do {
		if ((j = pa_simple_write(s, b, i, &error)) > 0) {
			i -= j;
			b = (char *)b + j;
		} else
			break;
	} while (i);

	if (j < 0) {
		fprintf(stderr, "pulseaudio error: %s\n", pa_strerror(error));
	}
}

static void deinit(void)
{
	pa_simple_free(s);
}

static void flush(void)
{
	int error;

	if (pa_simple_drain(s, &error) < 0) {
		fprintf(stderr, "pulseaudio error: %s\n", pa_strerror(error));
	}
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char *description(void)
{
	return "PulseAudio sound output";
}

const struct sound_driver sound_pulseaudio = {
	"pulseaudio",
	NULL,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
