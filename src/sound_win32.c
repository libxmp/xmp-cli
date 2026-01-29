/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/*
 * Based on Bjornar Henden's driver for Mikmod
 */

#include <windows.h>
#include <mmreg.h>
#include <stdio.h>
#include "sound.h"

#if defined(_MSC_VER) && (_MSC_VER < 1300)
typedef DWORD DWORD_PTR;
#endif

#define MAXBUFFERS	32			/* max number of buffers */
#define BUFFERSIZE	120			/* buffer size in ms */

/* frame rate = (50 * bpm / 125) Hz */
/* frame size = (sampling rate * channels * size) / frame rate */
#define OUT_MAXLEN 0x8000

#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE 0xfffe		/* mmreg.h, 98SE and later */
#endif

#pragma pack(push,1)
typedef struct _XMP_WAVEFORMATEXTENSIBLE {	/* mmreg.h */
	WAVEFORMATEX Format;
	union {
		WORD wValidBitsPerSample;
		WORD wSamplesPerBlock;
		WORD wReserved;
	} Samples;
	DWORD dwChannelMask;
	GUID SubFormat;
} XMP_WAVEFORMATEXTENSIBLE;
#pragma pack(pop)

static const GUID XMP_KSDATAFORMAT_SUBTYPE_PCM = { /* mmreg.h, ksuser.dll */
	WAVE_FORMAT_PCM, 0x0000, 0x0010,
	{ 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
};

static HWAVEOUT hwaveout;
static WAVEHDR header[MAXBUFFERS];
static LPSTR buffer[MAXBUFFERS];		/* pointers to buffers */
static WORD freebuffer;				/*  */
static WORD nextbuffer;				/* next buffer to be mixed */
static int num_buffers;
static int bits_per_sample;

static void show_error(int res)
{
	const char *msg;

	switch (res) {
	case MMSYSERR_ALLOCATED:
		msg = "Device is already open";
		break;
	case MMSYSERR_BADDEVICEID:
		msg = "Device is out of range";
		break;
	case MMSYSERR_NODRIVER:
		msg = "No audio driver in this system";
		break;
	case MMSYSERR_NOMEM:
		msg = "Unable to allocate sound memory";
		break;
	case WAVERR_BADFORMAT:
		msg = "Audio format not supported";
		break;
	case WAVERR_SYNC:
		msg = "The device is synchronous";
		break;
	default:
		msg = "Unknown media error";
	}

	fprintf(stderr, "Error: %s\n", msg);
}

static void CALLBACK wave_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
				   DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE) {
		freebuffer++;
		freebuffer %= num_buffers;
	}
}

static void set_waveformatex(WAVEFORMATEX *wfe, int rate, int bits, int chn)
{
	wfe->wBitsPerSample = bits;
	wfe->nChannels = chn;
	wfe->nSamplesPerSec = rate;
	wfe->nAvgBytesPerSec = wfe->nSamplesPerSec * chn * bits / 8;
	wfe->nBlockAlign = chn * bits / 8;
}

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	MMRESULT res;
	int chn, i;

	num_buffers = 10;

	parm_init(parm);
	chkparm1("buffers", num_buffers = strtoul(token, NULL, 0));
	parm_end();

	if (num_buffers > MAXBUFFERS)
		num_buffers = MAXBUFFERS;

	if (!waveOutGetNumDevs())
		return -1;

	bits_per_sample = get_bits_from_format(options);
	chn = get_channels_from_format(options);

	if (bits_per_sample > 16) {
		XMP_WAVEFORMATEXTENSIBLE wfx;
		set_waveformatex(&wfx.Format, options->rate, bits_per_sample, chn);
		wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfx.Format.cbSize = sizeof(wfx) - sizeof(wfx.Format);
		wfx.Samples.wValidBitsPerSample = bits_per_sample;
		wfx.dwChannelMask = 0;
		wfx.SubFormat = XMP_KSDATAFORMAT_SUBTYPE_PCM;

		res = waveOutOpen(&hwaveout, WAVE_MAPPER, (LPCWAVEFORMATEX)&wfx,
				  (DWORD_PTR)wave_callback, 0, CALLBACK_FUNCTION);

		if (res != MMSYSERR_NOERROR) {
			/* May be Windows 95; try 16-bit audio instead */
			bits_per_sample = 16;
		}
	}

	if (bits_per_sample <= 16) {
		WAVEFORMATEX wfe;
		set_waveformatex(&wfe, options->rate, bits_per_sample, chn);
		wfe.wFormatTag = WAVE_FORMAT_PCM;
		wfe.cbSize = 0;

		res = waveOutOpen(&hwaveout, WAVE_MAPPER, &wfe,
				  (DWORD_PTR)wave_callback, 0, CALLBACK_FUNCTION);
	}

	if (res != MMSYSERR_NOERROR) {
		show_error(res);
		return -1;
	}

	waveOutReset(hwaveout);

	for (i = 0; i < num_buffers; i++) {
		buffer[i] = (LPSTR) malloc(OUT_MAXLEN);
		header[i].lpData = buffer[i];

		if (!buffer[i]) {
			show_error(MMSYSERR_NOMEM);
			return -1;
		}
	}

	update_format_bits(options, bits_per_sample);
	update_format_signed(options, bits_per_sample != 8);
	freebuffer = nextbuffer = 0;

	return 0;
}

static void play(void *b, int len)
{
	if (bits_per_sample == 24) {
		len = downmix_32_to_24_packed((unsigned char *)b, len);
	}
	memcpy(buffer[nextbuffer], b, len);

	while ((nextbuffer + 1) % num_buffers == freebuffer)
		Sleep(10);

	header[nextbuffer].dwBufferLength = len;
	waveOutPrepareHeader(hwaveout, &header[nextbuffer], sizeof(WAVEHDR));
	waveOutWrite(hwaveout, &header[nextbuffer], sizeof(WAVEHDR));

	nextbuffer++;
	nextbuffer %= num_buffers;
}

static void deinit(void)
{
	int i;

	if (hwaveout) {
		waveOutReset(hwaveout);

		for (i = 0; i < num_buffers; i++) {
			if (header[i].dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(hwaveout, &header[i],
						       sizeof(WAVEHDR));
			free(buffer[i]);
		}
		while (waveOutClose(hwaveout) == WAVERR_STILLPLAYING)
			Sleep(10);
		hwaveout = NULL;
	}
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
	return "Windows WinMM sound output";
}

static const char *const help[] = {
	"buffers=val", "Number of buffers (default 10)",
	NULL
};

const struct sound_driver sound_win32 = {
	"win32",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
