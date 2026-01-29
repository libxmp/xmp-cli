/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include "sound.h"

static snd_pcm_t *pcm_handle;
static int bits;

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	snd_pcm_hw_params_t *hwparams;
	int ret;
	unsigned int channels;
	snd_pcm_format_t fmt;
	unsigned int btime = 250000;	/* 250ms */
	unsigned int ptime = 50000;	/* 50ms */
	const char *card_name = "default";
	unsigned int rate = options->rate;
	int sgn;

	parm_init(parm);
	chkparm1("buffer", btime = 1000 * strtoul(token, NULL, 0));
	chkparm1("period", btime = 1000 * strtoul(token, NULL, 0));
	chkparm1("card", card_name = token);
	parm_end();

	if ((ret = snd_pcm_open(&pcm_handle, card_name,
		SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "Unable to initialize ALSA pcm device %s: %s\n",
						card_name, snd_strerror(ret));
		return -1;
	}

	bits = get_bits_from_format(options);
	sgn = get_signed_from_format(options);
	channels = get_channels_from_format(options);

	switch (bits) {
	case 8:
		fmt = sgn ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_U8;
		break;
	case 16:
	default:
		fmt = sgn ? SND_PCM_FORMAT_S16 : SND_PCM_FORMAT_U16;
		break;
	case 24:
		fmt = sgn ? SND_PCM_FORMAT_S24 : SND_PCM_FORMAT_U24;
		break;
	case 32:
		fmt = sgn ? SND_PCM_FORMAT_S32 : SND_PCM_FORMAT_U32;
		break;
	}

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_hw_params_any(pcm_handle, hwparams);
	snd_pcm_hw_params_set_access(pcm_handle, hwparams,
						SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, hwparams, fmt);
	snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL);
	snd_pcm_hw_params_set_channels_near(pcm_handle, hwparams, &channels);
	snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &btime, NULL);
	snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &ptime, NULL);
	snd_pcm_nonblock(pcm_handle, 0);

	if ((ret = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
		fprintf(stderr, "Unable to set ALSA output parameters: %s\n", snd_strerror(ret));
		return -1;
	}

	if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
		fprintf(stderr, "Unable to prepare ALSA: %s\n", snd_strerror(ret));
		return -1;
	}

	snd_pcm_hw_params_get_format(hwparams, &fmt);
	switch (fmt) {
	case SND_PCM_FORMAT_S8:
	case SND_PCM_FORMAT_U8:
		bits = 8;
		break;
	case SND_PCM_FORMAT_S16:
	case SND_PCM_FORMAT_U16:
		bits = 16;
		break;
	case SND_PCM_FORMAT_S24:
	case SND_PCM_FORMAT_U24:
		bits = 24;
		break;
	case SND_PCM_FORMAT_S32:
	case SND_PCM_FORMAT_U32:
		bits = 32;
		break;
	default:
		break;
	}
	switch (fmt) {
	case SND_PCM_FORMAT_U8:
	case SND_PCM_FORMAT_U16:
	case SND_PCM_FORMAT_U24:
	case SND_PCM_FORMAT_U32:
		sgn = 0;
		break;
	default:
		sgn = 1;
		break;
	}
	update_format_bits(options, bits);
	update_format_signed(options, sgn);
	update_format_channels(options, channels);
	options->rate = rate;

	return 0;
}

static void play(void *b, int i)
{
	int frames = snd_pcm_bytes_to_frames(pcm_handle, i);

	if (bits == 24) {
		downmix_32_to_24_aligned((unsigned char *)b, i);
	}

	if (snd_pcm_writei(pcm_handle, b, frames) < 0) {
		snd_pcm_prepare(pcm_handle);
	}
}

static void deinit(void)
{
	snd_pcm_close(pcm_handle);
}

static void flush(void)
{
	snd_pcm_drain(pcm_handle);
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char *description(void)
{
	return "ALSA PCM audio";
}

static const char *const help[] = {
	"buffer=num", "Set the ALSA buffer time in milliseconds",
	"period=num", "Set the ALSA period time in milliseconds",
	"card <name>", "Select sound card to use",
	NULL
};

const struct sound_driver sound_alsa = {
	"alsa",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
