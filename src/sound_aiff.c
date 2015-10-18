/* Amiga AIFF driver for Extended Module Player
 * Copyright (C) 2014 Lorence Lombardo
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */  
    
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "sound.h"

typedef struct {
	/* Exponent, bit #15 is sign bit for mantissa */
	unsigned short exponent;

	/* 64 bit mantissa */
	unsigned long mantissa[2];
} extended;


static int fd;
static int channels;
static int bits;
static int swap_endian;
static long size;


static void ulong2extended(unsigned long in, extended *ex) 
{
	int exponent = 31 + 16383;

	while (!(in & 0x80000000)) {
		exponent--;
		in <<= 1;
	}

	ex->exponent = exponent;
	ex->mantissa[0] = in;
	ex->mantissa[1] = 0;
}

static inline void write8(int fd, unsigned char c)
{
	write(fd, &c, 1);
}

static void write32b(int fd, unsigned long w)
{
	write8(fd, (w & 0xff000000) >> 24);
	write8(fd, (w & 0x00ff0000) >> 16);
	write8(fd, (w & 0x0000ff00) >> 8);
	write8(fd,  w & 0x000000ff);
}

static int init(struct options *options) 
{
	char hed[54] = {
		'F', 'O', 'R', 'M', 0, 0, 0, 0,
		'A', 'I', 'F', 'F',

		/* COMM chunk */ 
		'C', 'O', 'M', 'M', 0, 0, 0, 18,
		0, 0,				/* channels */ 
		0, 0, 0, 0,			/* frames */ 
		0, 0,				/* bits */ 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* rate (extended format) */ 
		    
		/* SSND chunk */ 
		'S', 'S', 'N', 'D', 0, 0, 0, 0,
		0, 0, 0, 0,			/* offset */ 
		0, 0, 0, 0			/* block size */  
	};
	extended ex;

	swap_endian = !is_big_endian();
	channels = options->format & XMP_FORMAT_MONO ? 1 : 2;
	bits = options->format & XMP_FORMAT_8BIT ? 8 : 16;
	size = 0;

	ulong2extended(options->rate, &ex);
	hed[21] = channels;
	hed[27] = bits;
	hed[28] = (ex.exponent & 0xff00) >> 8;
	hed[29] =  ex.exponent & 0x00ff;
	hed[30] = (ex.mantissa[0] & 0xff000000) >> 24;
	hed[31] = (ex.mantissa[0] & 0x00ff0000) >> 16;
	hed[32] = (ex.mantissa[0] & 0x0000ff00) >> 8;
	hed[33] =  ex.mantissa[0] & 0x000000ff;

	if (options->out_file == NULL) {
		options->out_file = "out.aiff";
	}

	if (strcmp(options->out_file, "-")) {
		fd = open(options->out_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (fd < 0)
			return -1;
	} else {
		fd = 1;
	}

	write(fd, hed, 54);
	
	return 0;
}

static void play(void *b, int len) 
{
	if (swap_endian && bits == 16) {
		convert_endian(b, len);
	}
	write(fd, b, len);
	size += len;
}

static void deinit(void) 
{
	if (size > 54) {
		if (lseek(fd, 4, SEEK_SET) == 4) {	/* FORM chunk size */
			write32b(fd, size - 8);
		}

		if (lseek(fd, 22, SEEK_SET) == 22) {	/* COMM frames */
			unsigned long tmp = (size - 54) / (bits / 8) / channels;
			write32b(fd, tmp);
		}

		if (lseek(fd, 42, SEEK_SET) == 42) {	/* SSND chunk size */
			write32b(fd, size - 48);	/* minus header + 8 */
		}
	}

	if (fd > 0) {
		close(fd);
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

struct sound_driver sound_aiff = {
	"aiff",
	"AIFF writer",
	NULL,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume 
};
