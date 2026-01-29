/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdlib.h>
#include <string.h>
#include "sound.h"

const struct sound_driver *const sound_driver_list[] = {
#ifdef SOUND_AHI
	&sound_ahi,
#endif
#ifdef SOUND_BEOS
	&sound_beos,
#endif
#ifdef SOUND_SNDIO
	&sound_sndio,
#endif
#ifdef SOUND_NETBSD
	&sound_netbsd,
#endif
#ifdef SOUND_BSD
	&sound_bsd,
#endif
#ifdef SOUND_SOLARIS
	&sound_solaris,
#endif
#ifdef SOUND_SGI
	&sound_sgi,
#endif
#ifdef SOUND_HPUX
	&sound_hpux,
#endif
#ifdef SOUND_AIX
	&sound_aix,
#endif
#ifdef SOUND_COREAUDIO
	&sound_coreaudio,
#endif
#ifdef SOUND_OS2DART
	&sound_os2dart,
#endif
#ifdef SOUND_WIN32
	&sound_win32,
#endif
#ifdef SOUND_PULSEAUDIO
	&sound_pulseaudio,
#endif
#ifdef SOUND_ALSA
	&sound_alsa,
#endif
#ifdef SOUND_ALSA05
	&sound_alsa05,
#endif
#ifdef SOUND_OSS
	&sound_oss,
#endif
#ifdef SOUND_QNX
	&sound_qnx,
#endif
#ifdef SOUND_SB
	&sound_sb,
#endif
	&sound_wav,
	&sound_aiff,
	&sound_file,
	&sound_null,
	NULL
};

const struct sound_driver *select_sound_driver(struct options *options)
{
	const struct sound_driver *sd;
	const char *pref = options->driver_id;
	int i;

	if (pref) {
		for (i = 0; sound_driver_list[i] != NULL; i++) {
			sd = sound_driver_list[i];
			if (strcmp(sd->id, pref) == 0) {
				if (sd->init(options) == 0) {
					return sd;
				}
			}
		}
	} else {
		for (i = 0; sound_driver_list[i] != NULL; i++) {
			sd = sound_driver_list[i];
			/* Probing */
			if (sd->init(options) == 0) {
				/* found */
				return sd;
			}
		}
	}

	return NULL;
}

/* Downmix 32-bit to 24-bit aligned (in-place) */
void downmix_32_to_24_aligned(unsigned char *buffer, int buffer_bytes)
{
	/* Note: most 24-bit support ignores the high byte.
	 * sndio, however, expects 24-bit to be sign extended. */
	int *buf32 = (int *)buffer;
	int i;

	for (i = 0; i < buffer_bytes; i += 4) {
		*buf32 >>= 8;
		buf32++;
	}
}

/* Downmix 32-bit to 24-bit packed (in-place).
 * Returns the new number of useful bytes in the buffer. */
int downmix_32_to_24_packed(unsigned char *buffer, int buffer_bytes)
{
	unsigned char *out = buffer;
	int buffer_samples = buffer_bytes >> 2;
	int i;

	/* Big endian 32-bit (22 11 00 XX) -> 24-bit (22 11 00)
	 * Little endian 32-bit (XX 00 11 22) -> 24-bit (00 11 22)
	 * Skip the first byte for little endian to allow reusing the same loop.
	 */
	if (!is_big_endian()) {
		buffer++;
	}

	for (i = 0; i < buffer_samples; i++) {
		out[0] = buffer[0];
		out[1] = buffer[1];
		out[2] = buffer[2];
		out += 3;
		buffer += 4;
	}

	return buffer_samples * 3;
}


/* Convert native endian 16-bit samples for file IO */
void convert_endian_16bit(unsigned char *buffer, int buffer_bytes)
{
	unsigned char b;
	int i;

	for (i = 0; i < buffer_bytes; i += 2) {
		b = buffer[0];
		buffer[0] = buffer[1];
		buffer[1] = b;
		buffer += 2;
	}
}

/* Convert native endian 24-bit unaligned samples for file IO */
void convert_endian_24bit(unsigned char *buffer, int buffer_bytes)
{
	unsigned char b;
	int i;

	for (i = 0; i < buffer_bytes; i += 3) {
		b = buffer[0];
		buffer[0] = buffer[2];
		buffer[2] = b;
		buffer += 3;
	}
}

/* Convert native endian 32-bit samples for file IO */
void convert_endian_32bit(unsigned char *buffer, int buffer_bytes)
{
	unsigned char a, b;
	int i;

	for (i = 0; i < buffer_bytes; i += 4) {
		a = buffer[0];
		b = buffer[1];
		buffer[0] = buffer[3];
		buffer[1] = buffer[2];
		buffer[2] = b;
		buffer[3] = a;
		buffer += 4;
	}
}

/* Convert native endian 16-bit, 24-bit, or 32-bit samples for file IO */
void convert_endian(unsigned char *buffer, int buffer_bytes, int bits)
{
	switch (bits) {
	case 16:
		convert_endian_16bit(buffer, buffer_bytes);
		break;
	case 24:
		convert_endian_24bit(buffer, buffer_bytes);
		break;
	case 32:
		convert_endian_32bit(buffer, buffer_bytes);
		break;
	}
}
