/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/*
 * This should work for OS/2 Dart
 * History:
 *	1.0 - By Kevin Langman
 */

#undef VERSION /* stop conflict with os2medef.h */

#include <stdlib.h>
#include <string.h>

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <mcios2.h>
#include <meerror.h>
#include <os2medef.h>

#include "sound.h"

#define BUFFERCOUNT 4
#define BUF_MIN 8
#define BUF_MAX 32

static MCI_MIX_BUFFER MixBuffers[BUFFERCOUNT];
static MCI_MIXSETUP_PARMS MixSetupParms;
static MCI_BUFFER_PARMS BufferParms;
static MCI_GENERIC_PARMS GenericParms;

static ULONG DeviceID = 0;
static int bsize = 16;
static short next = 2;
static short ready = 1;

static HMTX mutex;

/* Buffer update thread (created and called by DART) */
static LONG APIENTRY OS2_Dart_UpdateBuffers
    (ULONG ulStatus, PMCI_MIX_BUFFER pBuffer, ULONG ulFlags) {

	if ((ulFlags == MIX_WRITE_COMPLETE) ||
	    ((ulFlags == (MIX_WRITE_COMPLETE | MIX_STREAM_ERROR)) &&
	     (ulStatus == ERROR_DEVICE_UNDERRUN))) {
		DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT);
		ready++;
		DosReleaseMutexSem(mutex);
	}

	return (TRUE);
}


static int init(struct options *options)
{
	char **parm = options->driver_parm;
	char sharing = 0;
	int device = 0;
	int flags;
	int i;
	MCI_AMP_OPEN_PARMS AmpOpenParms;

	parm_init(parm);
	chkparm1("sharing", sharing = *token);
	chkparm1("device", device = atoi(token));
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	parm_end();

	if (DosCreateMutexSem(NULL, &mutex, 0, 0) != NO_ERROR)
		return -1;

	if ((bsize < BUF_MIN || bsize > BUF_MAX) && bsize != 0) {
		bsize = 16 * 1024;
	} else {
		bsize *= 1024;
	}

	MixBuffers[0].pBuffer = NULL;	/* marker */
	memset(&GenericParms, 0, sizeof(MCI_GENERIC_PARMS));

	/* open AMP device */
	memset(&AmpOpenParms, 0, sizeof(MCI_AMP_OPEN_PARMS));
	AmpOpenParms.usDeviceID = 0;

	AmpOpenParms.pszDeviceType =
	    (PSZ) MAKEULONG(MCI_DEVTYPE_AUDIO_AMPMIX, (USHORT) device);

	flags = MCI_WAIT | MCI_OPEN_TYPE_ID;
	if (sharing == 'Y' || sharing == 'y') {
		flags = flags | MCI_OPEN_SHAREABLE;
	}

	if (mciSendCommand(0, MCI_OPEN, flags,
			   (PVOID) & AmpOpenParms, 0) != MCIERR_SUCCESS) {
		return -1;
	}

	DeviceID = AmpOpenParms.usDeviceID;

	/* setup playback parameters */
	memset(&MixSetupParms, 0, sizeof(MCI_MIXSETUP_PARMS));

	MixSetupParms.ulBitsPerSample =
			options->format & XMP_FORMAT_8BIT ? 8 : 16;
	MixSetupParms.ulFormatTag = MCI_WAVE_FORMAT_PCM;
	MixSetupParms.ulSamplesPerSec = options->rate;
	MixSetupParms.ulChannels = options->format & XMP_FORMAT_MONO ? 1 : 2;
	MixSetupParms.ulFormatMode = MCI_PLAY;
	MixSetupParms.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
	MixSetupParms.pmixEvent = OS2_Dart_UpdateBuffers;

	if (mciSendCommand(DeviceID, MCI_MIXSETUP,
			   MCI_WAIT | MCI_MIXSETUP_INIT,
			   (PVOID) & MixSetupParms, 0) != MCIERR_SUCCESS) {

		mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT,
			       (PVOID) & GenericParms, 0);
		return -1;
	}

	/* take in account the DART suggested buffer size... */
	if (bsize == 0) {
		bsize = MixSetupParms.ulBufferSize;
	}
	/*printf("Dart Buffer Size = %d\n", bsize);*/

	BufferParms.ulNumBuffers = BUFFERCOUNT;
	BufferParms.ulBufferSize = bsize;
	BufferParms.pBufList = MixBuffers;

	if (mciSendCommand(DeviceID, MCI_BUFFER,
			   MCI_WAIT | MCI_ALLOCATE_MEMORY,
			   (PVOID) & BufferParms, 0) != MCIERR_SUCCESS) {
		mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT,
			       (PVOID) & GenericParms, 0);
		return -1;
	}

	for (i = 0; i < BUFFERCOUNT; i++) {
		MixBuffers[i].ulBufferLength = bsize;
	}

	/* Start Playback */
	memset(MixBuffers[0].pBuffer, /*32767 */ 0, bsize);
	memset(MixBuffers[1].pBuffer, /*32767 */ 0, bsize);
	MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle, MixBuffers, 2);

	return 0;
}

static void play(void *b, int i)
{
	static int index = 0;

	if (index + i > bsize) {
		do {
			DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT);
			if (ready != 0) {
				DosReleaseMutexSem(mutex);
				break;
			}
			DosReleaseMutexSem(mutex);
			DosSleep(20);
		} while (TRUE);

		MixBuffers[next].ulBufferLength = index;
		MixSetupParms.pmixWrite(MixSetupParms.ulMixHandle,
					&(MixBuffers[next]), 1);
		ready--;
		next++;
		index = 0;
		if (next == BUFFERCOUNT) {
			next = 0;
		}
	}
	memcpy(&((char *)MixBuffers[next].pBuffer)[index], b, i);
	index += i;

}

static void deinit(void)
{
	if (MixBuffers[0].pBuffer) {
		mciSendCommand(DeviceID, MCI_BUFFER,
			       MCI_WAIT | MCI_DEALLOCATE_MEMORY, &BufferParms,
			       0);
		MixBuffers[0].pBuffer = NULL;
	}
	if (DeviceID) {
		mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT,
			       (PVOID) & GenericParms, 0);
		DeviceID = 0;
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
	return "OS/2 Direct Audio Realtime";
}

static const char *const help[] = {
	"sharing={Y,N}", "Device Sharing    (default is N)",
	"device=val", "OS/2 Audio Device (default is 0 auto-detect)",
	"buffer=val", "Audio buffer size (default is 16)",
	NULL
};

const struct sound_driver sound_os2dart = {
	"dart",
	help,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

