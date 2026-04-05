/* Amiga AHI driver for Extended Module Player
 * Based on a MikMod driver written by Szilard Biro, which was loosely
 * based on an old AmigaOS4 version by Fredrik Wikstrom.
 *
 * Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#ifdef __amigaos4__
#define SHAREDMEMFLAG MEMF_SHARED
#define __USE_INLINE__
#else
#define SHAREDMEMFLAG MEMF_PUBLIC
#endif

#include <stdlib.h>
#include <string.h>
#include "sound.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/ahi.h>

#define BUFFERSIZE (4 << 10)

static struct MsgPort *AHImp = NULL;
static struct AHIRequest *AHIReq[2] = { NULL, NULL };
static int active = 0;
static signed char *AHIBuf[2] = { NULL, NULL };

static void close_libs(void)
{
	if (AHIReq[1]) {
		/* in case req[1] is linked to req[0] */
		AHIReq[0]->ahir_Link = NULL;

		if (CheckIO((struct IORequest *) AHIReq[1]) == NULL) {
			AbortIO((struct IORequest *) AHIReq[1]);
			WaitIO((struct IORequest *) AHIReq[1]);
		}
		FreeVec(AHIReq[1]);
		AHIReq[1] = NULL;
	}
	if (AHIReq[0]) {
		if (CheckIO((struct IORequest *) AHIReq[0]) == NULL) {
			AbortIO((struct IORequest *) AHIReq[0]);
			WaitIO((struct IORequest *) AHIReq[0]);
		}
		if (AHIReq[0]->ahir_Std.io_Device) {
			CloseDevice((struct IORequest *) AHIReq[0]);
			AHIReq[0]->ahir_Std.io_Device = NULL;
		}
		DeleteIORequest((struct IORequest *) AHIReq[0]);
		AHIReq[0] = NULL;
	}
	if (AHImp) {
		DeleteMsgPort(AHImp);
		AHImp = NULL;
	}
	if (AHIBuf[0]) {
		FreeVec(AHIBuf[0]);
		AHIBuf[0] = NULL;
	}
	if (AHIBuf[1]) {
		FreeVec(AHIBuf[1]);
		AHIBuf[1] = NULL;
	}
}

static int init(struct options *options)
{
	unsigned long fmt;
	int channels;
	int bits;

	AHImp = CreateMsgPort();
	if (AHImp == NULL) {
		return -1;
	}

	AHIReq[0] = (struct AHIRequest *)
			CreateIORequest(AHImp, sizeof(struct AHIRequest));
	if (AHIReq[0] == NULL) {
		goto err;
	}
	AHIReq[0]->ahir_Version = 4;

	if (OpenDevice((CONST_STRPTR)AHINAME, AHI_DEFAULT_UNIT,
			(struct IORequest *)AHIReq[0], 0) != 0) {
		goto err;
	}

	bits = get_bits_from_format(options);
	channels = get_channels_from_format(options);

	switch (bits) {
	case 8:
		fmt = (channels == 1) ? AHIST_M8S : AHIST_S8S;
		bits = 8;
		break;
	case 16:
	default:
		fmt = (channels == 1) ? AHIST_M16S : AHIST_S16S;
		bits = 16;
		break;
#if defined(AHIST_M32S) && defined(AHIST_S32S)
	case 24:
	case 32:
		fmt = (channels == 1) ? AHIST_M32S : AHIST_S32S;
		bits = 32;
		break;
#endif
	}

	AHIReq[0]->ahir_Std.io_Command = CMD_WRITE;
	AHIReq[0]->ahir_Std.io_Data = NULL;
	AHIReq[0]->ahir_Std.io_Offset = 0;
	AHIReq[0]->ahir_Frequency = options->rate;
	AHIReq[0]->ahir_Type = fmt;
	AHIReq[0]->ahir_Volume = 0x10000;
	AHIReq[0]->ahir_Position = 0x8000;

	AHIReq[1] = AllocVec(sizeof(struct AHIRequest), SHAREDMEMFLAG);
	if (AHIReq[1] == NULL) {
		goto err;
	}
	CopyMem(AHIReq[0], AHIReq[1], sizeof(struct AHIRequest));

	AHIBuf[0] = AllocVec(BUFFERSIZE, SHAREDMEMFLAG | MEMF_CLEAR);
	AHIBuf[1] = AllocVec(BUFFERSIZE, SHAREDMEMFLAG | MEMF_CLEAR);
	if (AHIBuf[0] == NULL || AHIBuf[1] == NULL) {
		goto err;
	}

	update_format_bits(options, bits);
	update_format_signed(options, 1);

	active = 0;
	return 0;

    err:
	close_libs();
	return -1;
}

static void deinit(void)
{
	close_libs();
}

static void play(void *b, int len)
{
	signed char *in = (signed char *)b;
	int chunk;

	while (len > 0) {
		if (AHIReq[active]->ahir_Std.io_Data) {
			WaitIO((struct IORequest *) AHIReq[active]);
		}
		chunk = (len < BUFFERSIZE) ? len : BUFFERSIZE;
		memcpy(AHIBuf[active], in, chunk);
		len -= chunk;
		in += chunk;

		AHIReq[active]->ahir_Std.io_Data = AHIBuf[active];
		AHIReq[active]->ahir_Std.io_Length = chunk;
		AHIReq[active]->ahir_Link =
			CheckIO((struct IORequest *) AHIReq[active ^ 1]) == NULL ?
			AHIReq[active ^ 1] : NULL;
		SendIO((struct IORequest *) AHIReq[active]);
		active ^= 1;
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
	return "Amiga AHI audio";
}

const struct sound_driver sound_ahi = {
	"ahi",
	NULL,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
