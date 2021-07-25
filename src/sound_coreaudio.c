/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreServices/CoreServices.h>
#include "sound.h"

static AudioUnit au;

#ifndef HAVE_AUDIOUNIT_AUDIOCOMPONENT_H
#define AudioComponent Component
#define	AudioComponentDescription ComponentDescription
#define AudioComponentFindNext FindNextComponent
#define AudioComponentInstanceNew OpenAComponent
#define AudioComponentInstanceDispose CloseComponent
#endif

/*
 * CoreAudio helpers by Timothy J. Wood from mplayer/libao
 * The player fills a ring buffer, OSX retrieves data from the buffer
 */

static int paused;
static unsigned char *buffer;
static int buffer_len;
static int buf_write_pos;
static int buf_read_pos;
static int num_chunks;
static int chunk_size;
static int packet_size;


/* return minimum number of free bytes in buffer, value may change between
 * two immediately following calls, and the real number of free bytes
 * might actually be larger!  */
static int buf_free(void)
{
	int free = buf_read_pos - buf_write_pos - chunk_size;
	if (free < 0)
		free += buffer_len;
	return free;
}

/* return minimum number of buffered bytes, value may change between
 * two immediately following calls, and the real number of buffered bytes
 * might actually be larger! */
static int buf_used(void)
{
	int used = buf_write_pos - buf_read_pos;
	if (used < 0)
		used += buffer_len;
	return used;
}

/* add data to ringbuffer */
static int write_buffer(unsigned char *data, int len)
{
	int first_len = buffer_len - buf_write_pos;
	int free = buf_free();

	if (len > free)
		len = free;
	if (first_len > len)
		first_len = len;

	/* till end of buffer */
	memcpy(buffer + buf_write_pos, data, first_len);
	if (len > first_len) {	/* we have to wrap around */
		/* remaining part from beginning of buffer */
		memcpy(buffer, data + first_len, len - first_len);
	}
	buf_write_pos = (buf_write_pos + len) % buffer_len;

	return len;
}

/* remove data from ringbuffer */
static int read_buffer(unsigned char *data, int len)
{
	int first_len = buffer_len - buf_read_pos;
	int buffered = buf_used();

	if (len > buffered)
		len = buffered;
	if (first_len > len)
		first_len = len;

	/* till end of buffer */
	memcpy(data, buffer + buf_read_pos, first_len);
	if (len > first_len) {	/* we have to wrap around */
		/* remaining part from beginning of buffer */
		memcpy(data + first_len, buffer, len - first_len);
	}
	buf_read_pos = (buf_read_pos + len) % buffer_len;

	return len;
}

OSStatus render_proc(void *inRefCon,
		AudioUnitRenderActionFlags *inActionFlags,
		const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
		UInt32 inNumFrames, AudioBufferList *ioData)
{
	int amt = buf_used();
	int req = inNumFrames * packet_size;

	if (amt > req)
		amt = req;

	read_buffer((unsigned char *)ioData->mBuffers[0].mData, amt);
	ioData->mBuffers[0].mDataByteSize = amt;

        return noErr;
}

/*
 * end of CoreAudio helpers
 */


static int init(struct options *options)
{
	AudioStreamBasicDescription ad;
	AudioComponent comp;
	AudioComponentDescription cd;
	AURenderCallbackStruct rc;
	OSStatus status;
	UInt32 size, max_frames;
	int latency = 250;
	char **parm = options->driver_parm;

	parm_init(parm);
	chkparm1("buffer", latency = strtoul(token, NULL, 0));
	parm_end();

	if (latency < 20)
		latency = 20;

	ad.mSampleRate = options->rate;
	ad.mFormatID = kAudioFormatLinearPCM;
	ad.mFormatFlags = kAudioFormatFlagIsPacked |
			kAudioFormatFlagsNativeEndian;

	if (~options->format & XMP_FORMAT_UNSIGNED) {
		ad.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
	}

	ad.mChannelsPerFrame = options->format & XMP_FORMAT_MONO ? 1 : 2;
	ad.mBitsPerChannel = options->format & XMP_FORMAT_8BIT ? 8 : 16;

	if (options->format & XMP_FORMAT_8BIT) {
		ad.mBytesPerFrame = ad.mChannelsPerFrame;
	} else {
		ad.mBytesPerFrame = 2 * ad.mChannelsPerFrame;
	}
	ad.mBytesPerPacket = ad.mBytesPerFrame;
	ad.mFramesPerPacket = 1;

        packet_size = ad.mFramesPerPacket * ad.mChannelsPerFrame *
						(ad.mBitsPerChannel / 8);

	cd.componentType = kAudioUnitType_Output;
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;
	cd.componentManufacturer = kAudioUnitManufacturer_Apple;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;

	if ((comp = AudioComponentFindNext(NULL, &cd)) == NULL)
		goto err;

	if ((status = AudioComponentInstanceNew(comp, &au)))
		goto err1;

	if ((status = AudioUnitInitialize(au)))
		goto err1;

	if ((status = AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Input, 0, &ad, sizeof(ad))))
		goto err1;

	size = sizeof(UInt32);
        if ((status = AudioUnitGetProperty(au, kAudioDevicePropertyBufferSize,
			kAudioUnitScope_Input, 0, &max_frames, &size)))
		goto err1;

	chunk_size = max_frames;
        num_chunks = (options->rate * ad.mBytesPerFrame * latency / 1000
						+ chunk_size - 1) / chunk_size;
        buffer_len = (num_chunks + 1) * chunk_size;
	if ((buffer = calloc(num_chunks + 1, chunk_size)) == NULL)
		goto err;

	rc.inputProc = render_proc;
	rc.inputProcRefCon = 0;

        buf_read_pos = 0;
        buf_write_pos = 0;
	paused = 1;

	if ((status = AudioUnitSetProperty(au,
			kAudioUnitProperty_SetRenderCallback,
			kAudioUnitScope_Input, 0, &rc, sizeof(rc))))
		goto err2;
	
	return 0;

    err2:
	free(buffer);
    err1:
	fprintf(stderr, "initialization error: %d\n", (int)status);
    err:
	return -1;
}


/* Build and write one tick (one PAL frame or 1/50 s in standard vblank
 * timed mods) of audio data to the output device.
 */
static void play(void *b, int i)
{
	int j = 0;

	/* block until we have enough free space in the buffer */
	while (buf_free() < i)
		delay_ms(100);

	while (i) {
        	if ((j = write_buffer(b, i)) > 0) {
			i -= j;
			b += j;
		} else
			break;
	}

	if (paused) {
		AudioOutputUnitStart(au);
		paused = 0;
	}
}


static void deinit(void)
{
        AudioOutputUnitStop(au);
	AudioUnitUninitialize(au);
	AudioComponentInstanceDispose(au);
	free(buffer);
}

static void flush(void)
{
}

static void onpause(void)
{
        AudioOutputUnitStop(au);
}

static void onresume(void)
{
	AudioOutputUnitStart(au);
}

struct sound_driver sound_coreaudio = {
	"coreaudio",
	"CoreAudio sound output",
	NULL,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
