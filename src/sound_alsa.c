
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include "sound.h"

static snd_pcm_t *pcm_handle;

static int init(int *rate, int *format, char **parm)
{
	snd_pcm_hw_params_t *hwparams;
	int ret;
	unsigned int channels, fmt;
	unsigned int btime = 250000;	/* 250ms */
	unsigned int ptime = 50000;	/* 50ms */
	char *card_name = "default";

	parm_init();
	chkparm1("buffer", btime = 1000 * strtoul(token, NULL, 0));
	chkparm1("period", btime = 1000 * strtoul(token, NULL, 0));
	chkparm1("card", card_name = token);
	parm_end();

	if ((ret = snd_pcm_open(&pcm_handle, card_name,
		SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Unable to initialize ALSA pcm device: %s\n",
					snd_strerror(ret));
		return -1;
	}

	channels = *format & XMP_FORMAT_MONO ? 1 : 2;
	if (*format & XMP_FORMAT_UNSIGNED) {
		fmt = *format & XMP_FORMAT_8BIT ?
				SND_PCM_FORMAT_U8 : SND_PCM_FORMAT_U16;
	} else {
		fmt = *format & XMP_FORMAT_8BIT ?
				SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_S16;
	}

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_hw_params_any(pcm_handle, hwparams);
	snd_pcm_hw_params_set_access(pcm_handle, hwparams,
				SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, hwparams, fmt);
	snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams,
				(unsigned int *)rate, 0);
	snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &channels);
	snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &btime, 0);
	snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &ptime, 0);
	snd_pcm_nonblock(pcm_handle, 0);

	if ((ret = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
		fprintf(stderr, "Unable to set ALSA output parameters: %s\n",
					snd_strerror(ret));
		return -1;
	}

	if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
		fprintf(stderr, "Unable to prepare ALSA: %s\n",
					snd_strerror(ret));
		return -1;
	}
  
	if (channels == 1) {
		*format |= XMP_FORMAT_MONO;
	} else {
		*format &= ~XMP_FORMAT_MONO;
	}
	
	return 0;
}

static void play(void *b, int i)
{
	int frames;

	frames = snd_pcm_bytes_to_frames(pcm_handle, i);
	if (snd_pcm_writei(pcm_handle, b, frames) < 0) {
		snd_pcm_prepare(pcm_handle);
	}
}

static void deinit()
{
	snd_pcm_close(pcm_handle);
}

static void flush()
{
	snd_pcm_drain(pcm_handle);
}

static void onpause()
{
}

static void onresume()
{
}


static char *help[] = {
	"buffer=num", "Set the ALSA buffer time in milliseconds",
	"period=num", "Set the ALSA period time in milliseconds",
	"card <name>", "Select sound card to use",
	NULL
};

struct sound_driver sound_alsa = {
	"alsa",
	"ALSA PCM audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume	
};
