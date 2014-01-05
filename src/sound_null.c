/* Extended Module Player
 * Copyright (C) 1996-2014 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdlib.h>
#include "sound.h"

static int init(struct options *options)
{
	return 0;
}

static void play(void *b, int i)
{
}

static void deinit(void)
{
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


struct sound_driver sound_null = {
	"null",
	"null output",
	NULL,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume	
};
