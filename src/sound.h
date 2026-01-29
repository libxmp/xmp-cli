#ifndef XMP_SOUND_H
#define XMP_SOUND_H

#include <stdio.h>

#include "common.h"

struct sound_driver {
	const char *id;
	const char *const *help;
	const char * (*description)(void);
	int (*init)(struct options *);
	void (*deinit)(void);
	void (*play)(void *, int);
	void (*flush)(void);
	void (*pause)(void);
	void (*resume)(void);
};

extern const struct sound_driver sound_null;
extern const struct sound_driver sound_wav;
extern const struct sound_driver sound_aiff;
extern const struct sound_driver sound_file;
extern const struct sound_driver sound_qnx;
extern const struct sound_driver sound_alsa05;
extern const struct sound_driver sound_oss;
extern const struct sound_driver sound_alsa;
extern const struct sound_driver sound_os2dart;
extern const struct sound_driver sound_win32;
extern const struct sound_driver sound_pulseaudio;
extern const struct sound_driver sound_coreaudio;
extern const struct sound_driver sound_hpux;
extern const struct sound_driver sound_sndio;
extern const struct sound_driver sound_sgi;
extern const struct sound_driver sound_solaris;
extern const struct sound_driver sound_netbsd;
extern const struct sound_driver sound_bsd;
extern const struct sound_driver sound_beos;
extern const struct sound_driver sound_aix;
extern const struct sound_driver sound_ahi;
extern const struct sound_driver sound_sb;

extern const struct sound_driver *const sound_driver_list[];

#define parm_init(p) { char *token; for (; *(p); (p)++) { \
	char s[80]; strncpy(s, *(p), 79); s[79] = 0; \
	token = strtok(s, ":="); token = strtok(NULL, "");
#define parm_end() } }
#define parm_error() do { \
	fprintf(stderr, "xmp: incorrect parameters in -D %s\n", s); \
	exit(4); } while (0)
#define chkparm0(x,y) { \
	if (!strcmp(s, x)) { \
	    if (token != NULL) parm_error(); else { y; } } }
#define chkparm1(x,y) { \
	if (!strcmp(s, x)) { \
	    if (token == NULL) parm_error(); else { y; } } }
#define chkparm2(x,y,z,w) { if (!strcmp(s, x)) { \
	if (2 > sscanf(token, y, z, w)) parm_error(); } }

static inline int is_big_endian(void)
{
	unsigned short w = 0x00ff;
	return (*(char *)&w == 0x00);
}

static inline int get_bits_from_format(const struct options *options)
{
	if ((options->format & XMP_FORMAT_32BIT) && options->format_downmix != 24) {
		return 32;
	} else if (options->format & XMP_FORMAT_32BIT) {
		return 24;
	} else if (~options->format & XMP_FORMAT_8BIT) {
		return 16;
	} else {
		return 8;
	}
}

static inline int get_signed_from_format(const struct options *options)
{
	return !(options->format & XMP_FORMAT_UNSIGNED);
}

static inline int get_channels_from_format(const struct options *options)
{
	return options->format & XMP_FORMAT_MONO ? 1 : 2;
}

static inline void update_format_bits(struct options *options, int bits)
{
	options->format &= ~(XMP_FORMAT_32BIT | XMP_FORMAT_8BIT);
	options->format_downmix = 0;

	switch (bits) {
	case 8:
		options->format |= XMP_FORMAT_8BIT;
		break;
	case 24:
		options->format |= XMP_FORMAT_32BIT;
		options->format_downmix = 24;
		break;
	case 32:
		options->format |= XMP_FORMAT_32BIT;
		break;
	}
}

static inline void update_format_signed(struct options *options, int is_signed)
{
	if (!is_signed) {
		options->format |= XMP_FORMAT_UNSIGNED;
	} else {
		options->format &= ~XMP_FORMAT_UNSIGNED;
	}
}

static inline void update_format_channels(struct options *options, int channels)
{
	if (channels == 1) {
		options->format |= XMP_FORMAT_MONO;
	} else {
		options->format &= ~XMP_FORMAT_MONO;
	}
}

void init_sound_drivers(void);
const struct sound_driver *select_sound_driver(struct options *);
void downmix_32_to_24_aligned(unsigned char *, int);
int downmix_32_to_24_packed(unsigned char *, int);
void convert_endian_16bit(unsigned char *, int);
void convert_endian_24bit(unsigned char *, int);
void convert_endian_32bit(unsigned char *, int);
void convert_endian(unsigned char *, int, int);

#endif
