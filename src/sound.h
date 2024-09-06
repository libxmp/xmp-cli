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

static inline int is_big_endian(void) {
	unsigned short w = 0x00ff;
	return (*(char *)&w == 0x00);
}

void init_sound_drivers(void);
const struct sound_driver *select_sound_driver(struct options *);
void convert_endian(unsigned char *, int);

#endif
