#include <unistd.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include "sound.h"

static pa_simple *s;


static int init(struct options *options)
{
	pa_sample_spec ss;
	int error;

	options->format &= ~(XMP_MIX_UNSIGNED | XMP_MIX_8BIT);

	ss.format = PA_SAMPLE_S16NE;
	ss.channels = options->format & XMP_MIX_MONO ? 1 : 2;
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

	return 0;
}

static void play(void *b, int i)
{
	int j, error;

	do {
		if ((j = pa_simple_write(s, b, i, &error)) > 0) {
			i -= j;
			b += j;
		} else
			break;
	} while (i);

	if (j < 0) {
		fprintf(stderr, "pulseaudio error: %s\n", pa_strerror(error));
	}
}

static void deinit()
{
	pa_simple_free(s);
}

static void flush()
{
	int error;

	if (pa_simple_drain(s, &error) < 0) {
		fprintf(stderr, "pulseaudio error: %s\n", pa_strerror(error));
	}
}

static void onpause()
{
}

static void onresume()
{
}


struct sound_driver sound_pulseaudio = {
	"pulseaudio",
	"PulseAudio",
	NULL,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
