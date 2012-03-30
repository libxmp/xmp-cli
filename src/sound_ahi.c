/* Amiga AHI driver for Extended Module Player
 * Copyright (C) 2007 Lorence Lombardo
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "sound.h"

static int fd;

static int init(struct options *options)
{
	char **parm = options->driver_parm;
	char outfile[256];
	int nch = options->format & XMP_FORMAT_MONO ? 1 : 2;
	int res = options->format & XMP_FORMAT_8BIT ? 8 : 16;
	int bsize = options->rate * nch * res / 4;
	
	parm_init(parm);
	chkparm1("buffer", bsize = strtoul(token, NULL, 0));
	parm_end();

	sprintf(outfile, "AUDIO:B/%d/F/%d/C/%d/BUFFER/%d",
				res, options->rate, nch, bsize);

	fd = open(outfile, O_WRONLY);
	if (fd < 0)
		return -1;

	return 0;
}

static void play(void *b, int i)
{
	int j;

	while (i) {
		if ((j = write(fd, b, i)) > 0) {
			i -= j;
			b = (char *)b + j;
		} else
			break;
	}
}

static void deinit()
{
	close(fd);
}

static void flush()
{
}

static void onpause()
{
}

static void onresume()
{
}


static char *help[] = {
	"buffer=val", "Audio buffer size",
	NULL
};

struct sound_driver sound_ahi = {
	"ahi",
	"Amiga AHI audio",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

