/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "sound.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int fd;
static int format_16bit;
static int swap_endian;
static long size;

struct sound_driver sound_wav;

static void write_16l(int fd, unsigned short v)
{
	unsigned char x;

	x = v & 0xff;
	write(fd, &x, 1);

	x = v >> 8;
	write(fd, &x, 1);
}

static void write_32l(int fd, unsigned int v)
{
	unsigned char x;

	x = v & 0xff;
	write(fd, &x, 1);

	x = (v >> 8) & 0xff;
	write(fd, &x, 1);

	x = (v >> 16) & 0xff;
	write(fd, &x, 1);

	x = (v >> 24) & 0xff;
	write(fd, &x, 1);
}

static int init(struct options *options)
{
	char *buf;
	unsigned int len = 0;
	unsigned short chan;
	unsigned int sampling_rate, bytes_per_second;
	unsigned short bytes_per_frame, bits_per_sample;

	swap_endian = is_big_endian();

	if (options->out_file == NULL) {
		options->out_file = "out.wav";
	}

	if (strcmp(options->out_file, "-")) {
		fd = open(options->out_file, O_WRONLY | O_CREAT | O_TRUNC
							| O_BINARY, 0644);
		if (fd < 0)
			return -1;
	} else {
		fd = 1;
	}

	if (strcmp(options->out_file, "-")) {
		int len = strlen(sound_wav.description) +
				strlen(options->out_file) + 8;
		if ((buf = malloc(len)) == NULL)
			return -1;
		snprintf(buf, len, "%s: %s", sound_wav.description,
						options->out_file);
		sound_wav.description = buf;
	} else {
		sound_wav.description = strdup("WAV writer: stdout");
		len = -1;
	}

	write(fd, "RIFF", 4);
	write_32l(fd, len);
	write(fd, "WAVE", 4);

	chan = options->format & XMP_FORMAT_MONO ? 1 : 2;
	sampling_rate = options->rate;

	bits_per_sample = options->format & XMP_FORMAT_8BIT ? 8 : 16;
	if (bits_per_sample == 8) {
		options->format |= XMP_FORMAT_UNSIGNED;
		format_16bit = 0;
	} else {
		options->format &= ~XMP_FORMAT_UNSIGNED;
		format_16bit = 1;
	}

	bytes_per_frame = chan * bits_per_sample / 8;
	bytes_per_second = sampling_rate * bytes_per_frame;

	write(fd, "fmt ", 4);
	write_32l(fd, 16);
	write_16l(fd, 1);
	write_16l(fd, chan);
	write_32l(fd, sampling_rate);
	write_32l(fd, bytes_per_second);
	write_16l(fd, bytes_per_frame);
	write_16l(fd, bits_per_sample);

	write(fd, "data", 4);
	write_32l(fd, len);

	size = 0;

	return 0;
}

static void play(void *b, int len)
{
	if (swap_endian && format_16bit) {
		convert_endian(b, len);
	}
	write(fd, b, len);
	size += len;
}

static void deinit(void)
{
	lseek(fd, 40, SEEK_SET);
	write_32l(fd, size);

	lseek(fd, 4, SEEK_SET);
	write_32l(fd, size + 40);

	if (fd > 0) {
		close(fd);
	}

	free(sound_wav.description);
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


struct sound_driver sound_wav = {
	"wav",
	"WAV writer",
	NULL,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};

