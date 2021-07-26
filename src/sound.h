#ifndef XMP_SOUND_H
#define XMP_SOUND_H

#include <xmp.h>
#include <stdio.h>
#include "common.h"
#include "list.h"

struct sound_driver {
	const char *id;
	const char *description;
	const char *const *help;
	int (*init)(struct options *);
	void (*deinit)(void);
	void (*play)(void *, int);
	void (*flush)(void);
	void (*pause)(void);
	void (*resume)(void);
	struct list_head list;
};

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
struct sound_driver *select_sound_driver(struct options *);
void convert_endian(unsigned char *, int);

#endif
