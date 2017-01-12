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

extern struct sound_driver sound_null;
extern struct sound_driver sound_wav;
extern struct sound_driver sound_aiff;
extern struct sound_driver sound_file;
extern struct sound_driver sound_qnx;
extern struct sound_driver sound_alsa05;
extern struct sound_driver sound_oss;
extern struct sound_driver sound_alsa;
extern struct sound_driver sound_os2dart;
extern struct sound_driver sound_win32;
extern struct sound_driver sound_pulseaudio;
extern struct sound_driver sound_coreaudio;
extern struct sound_driver sound_hpux;
extern struct sound_driver sound_sndio;
extern struct sound_driver sound_sgi;
extern struct sound_driver sound_solaris;
extern struct sound_driver sound_netbsd;
extern struct sound_driver sound_bsd;
extern struct sound_driver sound_beos;
extern struct sound_driver sound_aix;
extern struct sound_driver sound_ahi;

LIST_HEAD(sound_driver_list);

static void register_sound_driver(struct sound_driver *sd)
{
	list_add_tail(&sd->list, &sound_driver_list);	
}

void init_sound_drivers(void)
{
#ifdef SOUND_AHI
	register_sound_driver(&sound_ahi);
#endif
#ifdef SOUND_BEOS
	register_sound_driver(&sound_beos);
#endif
#ifdef SOUND_SNDIO
	register_sound_driver(&sound_sndio);
#endif
#ifdef SOUND_NETBSD
	register_sound_driver(&sound_netbsd);
#endif
#ifdef SOUND_BSD
	register_sound_driver(&sound_bsd);
#endif
#ifdef SOUND_SOLARIS
	register_sound_driver(&sound_solaris);
#endif
#ifdef SOUND_SGI
	register_sound_driver(&sound_sgi);
#endif
#ifdef SOUND_HPUX
	register_sound_driver(&sound_hpux);
#endif
#ifdef SOUND_AIX
	register_sound_driver(&sound_aix);
#endif
#ifdef SOUND_COREAUDIO
	register_sound_driver(&sound_coreaudio);
#endif
#ifdef SOUND_OS2DART
	register_sound_driver(&sound_os2dart);
#endif
#ifdef SOUND_WIN32
	register_sound_driver(&sound_win32);
#endif
#ifdef SOUND_PULSEAUDIO
	register_sound_driver(&sound_pulseaudio);
#endif
#ifdef SOUND_ALSA
	register_sound_driver(&sound_alsa);
#endif
#ifdef SOUND_ALSA05
	register_sound_driver(&sound_alsa05);
#endif
#ifdef SOUND_OSS
	register_sound_driver(&sound_oss);
#endif
#ifdef SOUND_QNX
	register_sound_driver(&sound_qnx);
#endif
	register_sound_driver(&sound_wav);
	register_sound_driver(&sound_aiff);
	register_sound_driver(&sound_file);
	register_sound_driver(&sound_null);
}

struct sound_driver *select_sound_driver(struct options *options)
{
	struct list_head *head;
	struct sound_driver *sd;
	char *pref = options->driver_id;

	if (pref) {
		list_for_each(head, &sound_driver_list) {
			sd = list_entry(head, struct sound_driver, list);
			if (strcmp(sd->id, pref) == 0) {
				if (sd->init(options) == 0) {
					return sd;
				}
			}
		}
	} else {
		list_for_each(head, &sound_driver_list) {
			sd = list_entry(head, struct sound_driver, list);
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
