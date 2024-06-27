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

static FILE *fd;
static long size;
static int swap_endian;

static int init(struct options *options)
{
	char **parm = options->driver_parm;

	swap_endian = 0;

	parm_init(parm);
	chkparm1("endian",
		swap_endian = (is_big_endian() ^ strcmp(token, "big")));
	parm_end();

	if (options->out_file == NULL) {
		options->out_file = "out.raw";
	}

	if (strcmp(options->out_file, "-")) {
		fd = fopen(options->out_file, "wb");
		if (fd == NULL)
			return -1;
	} else {
		fd = stdout;
	}

	return 0;
}

static void play(void *b, int len)
{
	if (swap_endian) {
		convert_endian((unsigned char *)b, len);
	}
	fwrite(b, 1, len, fd);
	size += len;
}

static void deinit(void)
{
	if (fd && fd != stdout) {
		fclose(fd);
	}
	fd = NULL;
}

static void flush(void)
{
	if (fd) {
		fflush(fd);
	}
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char * const help[] = {
	"endian=big", "Force big-endian 16-bit samples",
	"endian=little", "Force little-endian 16-bit samples",
	NULL
};

const struct sound_driver sound_file = {
	"file",
	"Raw file writer",
	help,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
