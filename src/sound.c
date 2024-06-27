/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
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

/* Convert little-endian 16 bit samples to big-endian */
void convert_endian(unsigned char *p, int l)
{
	unsigned char b;
	int i;

	for (i = 0; i < l; i++) {
		b = p[0];
		p[0] = p[1];
		p[1] = b;
		p += 2;
	}
}
