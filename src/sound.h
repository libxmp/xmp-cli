#ifndef __SOUND_H
#define __SOUND_H

#include <xmp.h>
#include "list.h"

struct sound_driver {
	char *id;
	char *description;
	char **help;
	int (*init)(int *, int *);
        void (*deinit)(void);
	void (*play)(void *, int);
        void (*flush)(void);
        void (*pause)(void);
        void (*resume)(void);
        struct list_head list;
};

void init_sound_drivers(void);
struct sound_driver *select_sound_driver(char *, int *, int *);

#endif
