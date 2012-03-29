#include <stdlib.h>
#include <string.h>
#include "sound.h"

extern struct sound_driver sound_wav;
extern struct sound_driver sound_oss;
extern struct sound_driver sound_alsa;
extern struct sound_driver sound_win32;
extern struct sound_driver sound_coreaudio;
extern struct sound_driver sound_sndio;
extern struct sound_driver sound_openbsd;
extern struct sound_driver sound_bsd;
extern struct sound_driver sound_beos;

LIST_HEAD(sound_driver_list);

static void register_sound_driver(struct sound_driver *sd)
{
	list_add_tail(&sd->list, &sound_driver_list);	
}

void init_sound_drivers()
{
#ifdef SOUND_BEOS
	register_sound_driver(&sound_beos);
#endif
#ifdef SOUND_SNDIO
	register_sound_driver(&sound_sndio);
#endif
#ifdef SOUND_OPENBSD
	register_sound_driver(&sound_openbsd);
#endif
#ifdef SOUND_BSD
	register_sound_driver(&sound_bsd);
#endif
#ifdef SOUND_COREAUDIO
	register_sound_driver(&sound_coreaudio);
#endif
#ifdef SOUND_WIN32
	register_sound_driver(&sound_win32);
#endif
#ifdef SOUND_OSS
	register_sound_driver(&sound_oss);
#endif
#ifdef SOUND_ALSA
	register_sound_driver(&sound_alsa);
#endif
	register_sound_driver(&sound_wav);
}

struct sound_driver *select_sound_driver(struct options *options)
{
	struct list_head *head;
	struct sound_driver *sd;
	char *pref = options->drv_id;

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
