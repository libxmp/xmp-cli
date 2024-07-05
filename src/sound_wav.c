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
static int format_16bit;
static int swap_endian;
static long size;

static void write_16l(FILE *f, unsigned short v)
{
	unsigned char x;

	x = v & 0xff;
	fwrite(&x, 1, 1, f);

	x = v >> 8;
	fwrite(&x, 1, 1, f);
}

static void write_32l(FILE *f, unsigned int v)
{
	unsigned char x;

	x = v & 0xff;
	fwrite(&x, 1, 1, f);

	x = (v >> 8) & 0xff;
	fwrite(&x, 1, 1, f);

	x = (v >> 16) & 0xff;
	fwrite(&x, 1, 1, f);

	x = (v >> 24) & 0xff;
	fwrite(&x, 1, 1, f);
}

static int init(struct options *options)
{
	unsigned short chan;
	unsigned int sampling_rate, bytes_per_second;
	unsigned short bytes_per_frame, bits_per_sample;

	swap_endian = is_big_endian();

	if (options->out_file == NULL) {
		options->out_file = "out.wav";
	}

	if (strcmp(options->out_file, "-")) {
		fd = fopen(options->out_file, "wb");
		if (fd == NULL)
			return -1;
	} else {
		fd = stdout;
	}

	fwrite("RIFF", 1, 4, fd);
	write_32l(fd, 0);		/* will be written when finished */
	fwrite("WAVE", 1, 4, fd);

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

	fwrite("fmt ", 1, 4, fd);
	write_32l(fd, 16);
	write_16l(fd, 1);
	write_16l(fd, chan);
	write_32l(fd, sampling_rate);
	write_32l(fd, bytes_per_second);
	write_16l(fd, bytes_per_frame);
	write_16l(fd, bits_per_sample);

	fwrite("data", 1, 4, fd);
	write_32l(fd, 0);		/* will be written when finished */

	size = 0;

	return 0;
}

static void play(void *b, int len)
{
	if (swap_endian && format_16bit) {
		convert_endian((unsigned char *)b, len);
	}
	fwrite(b, 1, len, fd);
	size += len;
}

static void deinit(void)
{
	if (fseek(fd, 40, SEEK_SET) == 0) {
		write_32l(fd, size);
	}
	if (fseek(fd, 4, SEEK_SET) == 0) {
		write_32l(fd, size + 40);
	}

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

const struct sound_driver sound_wav = {
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
