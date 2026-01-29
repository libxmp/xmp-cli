/* Extended Module Player
 * Copyright (C) 1996-2026 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdlib.h>
#include <string.h>
#include "sound.h"

#define XMP_WAVE_FORMAT_PCM		0x0001
#define XMP_WAVE_FORMAT_EXTENSIBLE	0xfffe

static const char subformat[] = {
	"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71"
};

static FILE *fd;
static int bits_per_sample;
static int swap_endian;
static long size;
static long data_size_offset;

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
	unsigned short bytes_per_frame;

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

	chan = get_channels_from_format(options);
	sampling_rate = options->rate;

	bits_per_sample = get_bits_from_format(options);
	update_format_signed(options, bits_per_sample != 8);

	bytes_per_frame = chan * bits_per_sample / 8;
	bytes_per_second = sampling_rate * bytes_per_frame;

	fwrite("fmt ", 1, 4, fd);
	if (bits_per_sample > 16) {
		write_32l(fd, 40);
		write_16l(fd, XMP_WAVE_FORMAT_EXTENSIBLE);
		data_size_offset = 64;
	} else {
		write_32l(fd, 16);
		write_16l(fd, XMP_WAVE_FORMAT_PCM);
		data_size_offset = 40;
	}
	write_16l(fd, chan);
	write_32l(fd, sampling_rate);
	write_32l(fd, bytes_per_second);
	write_16l(fd, bytes_per_frame);
	write_16l(fd, bits_per_sample);

	if (bits_per_sample > 16) {
		write_16l(fd, 22);			/* size of extension */
		write_16l(fd, bits_per_sample);		/* valid bits per sample */
		write_32l(fd, 0);			/* chn position mask */
		write_16l(fd, XMP_WAVE_FORMAT_PCM);	/* subformat code */
		fwrite(subformat, 1, 14, fd);		/* subformat suffix */
	}

	fwrite("data", 1, 4, fd);
	write_32l(fd, 0);		/* will be written when finished */

	size = 0;

	return 0;
}

static void play(void *b, int len)
{
	if (bits_per_sample == 24) {
		len = downmix_32_to_24_packed((unsigned char *)b, len);
	}
	if (swap_endian) {
		convert_endian((unsigned char *)b, len, bits_per_sample);
	}
	fwrite(b, 1, len, fd);
	size += len;
}

static void deinit(void)
{
	/* Pad chunks to word boundaries, even at the end of the file. */
	int pad = 0;
	if (size & 1) {
		fputc(0, fd);
		pad = 1;
	}

	if (fseek(fd, data_size_offset, SEEK_SET) == 0) {
		write_32l(fd, size);
	}
	if (fseek(fd, 4, SEEK_SET) == 0) {
		write_32l(fd, size + pad + (data_size_offset + 4 - 8));
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

static const char *description(void)
{
	return "WAV writer";
}

const struct sound_driver sound_wav = {
	"wav",
	NULL,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
