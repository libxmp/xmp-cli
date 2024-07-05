/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/*
 * Based on Bjornar Henden's driver for Mikmod
 */

#include <windows.h>
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

static HWAVEOUT hwaveout;
static WAVEHDR header[MAXBUFFERS];
static LPSTR buffer[MAXBUFFERS];		/* pointers to buffers */
static WORD freebuffer;				/*  */
static WORD nextbuffer;				/* next buffer to be mixed */
static int num_buffers;

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

	fprintf(stderr, "Error: %s", msg);
}

static void CALLBACK wave_callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                                  DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE) {
		freebuffer++;
		freebuffer %= num_buffers;
	}
}

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	MMRESULT res;
	WAVEFORMATEX wfe;
	int i;

	num_buffers = 10;

	parm_init(parm);
	chkparm1("buffers", num_buffers = strtoul(token, NULL, 0));
	parm_end();

	if (num_buffers > MAXBUFFERS)
		num_buffers = MAXBUFFERS;

	if (!waveOutGetNumDevs())
		return -1;

	wfe.wFormatTag = WAVE_FORMAT_PCM;
	wfe.wBitsPerSample = options->format & XMP_FORMAT_8BIT ? 8 : 16;
	wfe.nChannels = options->format & XMP_FORMAT_MONO ? 1 : 2;
	wfe.nSamplesPerSec = options->rate;
	wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nChannels *
	    wfe.wBitsPerSample / 8;
	wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample / 8;

	res = waveOutOpen(&hwaveout, WAVE_MAPPER, &wfe, (DWORD_PTR) wave_callback,
			  0, CALLBACK_FUNCTION);

	if (res != MMSYSERR_NOERROR) {
		show_error(res);
		return -1;
	}

	waveOutReset(hwaveout);

	for (i = 0; i < num_buffers; i++) {
		buffer[i] = (LPSTR) malloc(OUT_MAXLEN);
		header[i].lpData = buffer[i];

		if (!buffer[i] || res != MMSYSERR_NOERROR) {
			show_error(res);
			return -1;
		}
	}

	freebuffer = nextbuffer = 0;

	return 0;
}

static void play(void *b, int len)
{
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

static const char *const help[] = {
	"buffers=val", "Number of buffers (default 10)",
	NULL
};

const struct sound_driver sound_win32 = {
	"win32",
	"Windows WinMM sound output",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

