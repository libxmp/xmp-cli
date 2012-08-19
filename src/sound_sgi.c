#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <dmedia/audio.h>
#include <fcntl.h>
#include "sound.h"

static ALport audio_port;

/* Hack to get 16 bit sound working - 19990706 bdowning */
static int al_sample_16;

/*
 * audio port sample rates (these are the only ones supported by the library)
 */

static int srate[] = {
	48000,
	44100,
	32000,
	22050,
	16000,
	11025,
	8000,
	0
};


static int init(struct options *options)
{
	char **parm = options->driver_parm;
	int bsize = 32 * 1024;
	ALconfig config;
	long pvbuffer[2];
	int i;

	parm_init(parm);
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	parm_end();

	if ((config = ALnewconfig()) == 0)
		return -1;

	/*
	 * Set sampling rate
	 */

	pvbuffer[0] = AL_OUTPUT_RATE;

	for (i = 0; srate[i]; i++) ;	/* find the end of the array */

	while (i-- > 0) {
		if (srate[i] >= options->rate) {
			pvbuffer[1] = options->rate = srate[i];
			break;
		}
	}

	if (i == 0)
		pvbuffer[1] = options->rate = srate[0];

	if (ALsetparams(AL_DEFAULT_DEVICE, pvbuffer, 2) < 0)
		return -1;

	/*
	 * Set sample format to signed integer
	 */

	if (ALsetsampfmt(config, AL_SAMPFMT_TWOSCOMP) < 0)
		return -1;

	/*
	 * Set sample width; 24 bit samples are not currently supported by xmp
	 */

	if (options->format & XMP_MIX_8BIT) {
		if (ALsetwidth(config, AL_SAMPLE_8) < 0) {
			if (ALsetwidth(config, AL_SAMPLE_16) < 0)
				return -1;
			options->format &= ~XMP_MIX_8BIT;
		} else {
			al_sample_16 = 0;
		}
	} else {
		if (ALsetwidth(config, AL_SAMPLE_16) < 0) {
			if (ALsetwidth(config, AL_SAMPLE_8) < 0)
				return -1;
			options->format |= XMP_MIX_8BIT;
		} else {
			al_sample_16 = 1;
		}
	}

	/*
	 * Set number of channels; 4 channel output is not currently supported
	 */

	if (options->format & XMP_MIX_MONO) {
		if (ALsetchannels(config, AL_MONO) < 0) {
			if (ALsetchannels(config, AL_STEREO) < 0)
				return -1;
			options->format &= ~XMP_MIX_MONO;
		}
	} else {
		if (ALsetchannels(config, AL_STEREO) < 0) {
			if (ALsetchannels(config, AL_MONO) < 0)
				return -1;
			options->format |= XMP_MIX_MONO;
		}
	}

	/*
	 * Set buffer size
	 */

	if (ALsetqueuesize(config, bsize) < 0)
		return -1;

	/*
	 * Open the audio port
	 */

	if ((audio_port = ALopenport("xmp", "w", config)) == 0)
		return -1;

	return 0;
}

/* Build and write one tick (one PAL frame or 1/50 s in standard vblank
 * timed mods) of audio data to the output device.
 *
 * Apparently ALwritesamps requires the number of samples instead of
 * the number of bytes, which is what I assume i is.  This was a
 * trial-and-error fix, but it appears to work. - 19990706 bdowning
 */
static void play(void *b, int i)
{
	if (al_sample_16)
		i /= 2;

	ALwritesamps(audio_port, b, i);
}

static void deinit()
{
	ALcloseport(audio_port);
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
	"buffer=val", "Audio buffer size",
	NULL
};

struct sound_driver sound_sgi = {
	"sgi",
	"SGI IRIX PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
