/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <xmp.h>
#include "common.h"

#ifdef __AMIGA__
#include <sys/unistd.h>
#endif

static char driver[32];
static char instrument_path[256];

static void delete_spaces(char *s)
{
	while (*s) {
		if (*s == ' ' || *s == '\t') {
			memmove(s, s + 1, strlen(s));
		} else {
			s++;
		}
	}
}

static int get_yesno(char *s)
{
	return !(strncmp(s, "y", 1) && strncmp(s, "o", 1));
}

int read_config(struct options *o)
{
	FILE *rc;
	char myrc[PATH_MAX];
	char *hash, *var, *val, line[256];
	char cparm[512];

#if defined(__OS2__) || defined(__EMX__)
	const char *home = getenv("HOME");
	if (!home) home = "C:";

	snprintf(myrc, PATH_MAX, "%s\\xmp.conf", home);

	if ((rc = fopen(myrc, "r")) == NULL) {
		if ((rc = fopen("xmp.conf", "r")) == NULL) {
			return -1;
		}
	}
#elif defined __AMIGA__
	strncpy(myrc, "PROGDIR:xmp.conf", PATH_MAX);

	if ((rc = fopen(myrc, "r")) == NULL)
		return -1;
#elif defined WIN32
	const char *home = getenv("USERPROFILE");
	if (!home) home = "C:";

	snprintf(myrc, PATH_MAX, "%s/xmp.conf", home);

	if ((rc = fopen(myrc, "r")) == NULL)
		return -1;
#else
	char *home = getenv("HOME");

	snprintf(myrc, PATH_MAX, "%s/.xmp/xmp.conf", home);

	if ((rc = fopen(myrc, "r")) == NULL) {
		strncpy(myrc, SYSCONFDIR "/xmp.conf", PATH_MAX);
		if ((rc = fopen(myrc, "r")) == NULL) {
			return -1;
		}
	}
#endif

	while (!feof(rc)) {
		memset(line, 0, 256);
		if (fscanf(rc, "%255[^\n]", line) < 0 || fgetc(rc) < 0) {
			break;
		}

		/* Delete comments */
		if ((hash = strchr(line, '#')))
			*hash = 0;

		delete_spaces(line);

		if (!(var = strtok(line, "=\n"))) {
			continue;
		}

		val = strtok(NULL, " \t\n");

#define getval_yn(x,w,y) { \
	if (!strcmp(var,x)) { if (get_yesno (val)) w |= (y); \
	    else w &= ~(y); continue; } }

#define getval_tristate(x,w) { \
	if (!strcmp(var,x)) { if (get_yesno (val)) w = 1; \
	    else w = -1; continue; } }

#define getval_no(x,y) { \
	if (!strcmp(var,x)) { y = atoi (val); continue; } }

		getval_yn("8bit", o->format, XMP_FORMAT_8BIT);
		getval_yn("mono", o->format, XMP_FORMAT_MONO);
		getval_yn("filter", o->dsp, XMP_DSP_LOWPASS);
		getval_yn("loop", o->loop, 1);
		getval_yn("reverse", o->reverse, 1);
		getval_no("amplify", o->amplify);
		getval_no("mix", o->mix);
		getval_no("mixer_voices", o->numvoices);
		getval_no("default_pan", o->defpan);
		/*getval_no("chorus", o->chorus);
		getval_no("reverb", o->reverb);*/
		getval_no("srate", o->rate);
		/*getval_no("time", o->time);
		getval_no("verbosity", o->verbosity);*/
		getval_yn("amiga", o->amiga, 1);

		if (!strcmp(var, "driver")) {
			strncpy(driver, val, 31);
			o->driver_id = driver;
			continue;
		}

		if (!strcmp(var, "interpolation")) {
			if (!strcmp(val, "nearest")) {
				o->interp = XMP_INTERP_NEAREST;
			} else if (!strcmp(val, "linear")) {
				o->interp = XMP_INTERP_LINEAR;
			} else if (!strcmp(val, "spline")) {
				o->interp = XMP_INTERP_SPLINE;
			} else {
				fprintf(stderr, "%s: unknown interpolation "
						"type \"%s\"\n", myrc, val);
			}
		}

		if (!strcmp(var, "bits")) {
			if (atoi(val) == 8)
				o->format = XMP_FORMAT_8BIT;;
			continue;
		}

		if (!strcmp(var, "instrument_path")) {
			strncpy(instrument_path, val, 255);
			instrument_path[255] = 0;
			o->ins_path = instrument_path;
			continue;
		}

		/* If the line doesn't match any of the previous parameters,
		 * send it to the device driver
		 */
		if (o->dparm < MAX_DRV_PARM) {
			snprintf(cparm, 512, "%s=%s", var, val);
			o->driver_parm[o->dparm++] = strdup(cparm);
		}
	}

	fclose(rc);

	return 0;
}


static int compare_md5(unsigned char *d, char *digest)
{
	int i;

	for (i = 0; i < 16 && *digest; i++, digest += 2) {
		char hex[3];
		hex[0] = digest[0];
		hex[1] = digest[1];
		hex[2] = 0;

		if (d[i] != strtoul(hex, NULL, 16))
			return -1;
	}

	return 0;
}

static void parse_modconf(struct options *o, char *confname, unsigned char *md5)
{
	FILE *rc;
	char *hash, *var, *val, line[256];
	struct player_mode *pm;
	int active = 0;
	int mono = 0;

	if ((rc = fopen(confname, "r")) == NULL)
		return;

	while (!feof(rc)) {
		memset(line, 0, 256);
		if (fscanf(rc, "%255[^\n]", line) < 0 || fgetc(rc) < 0) {
			break;
		}

		/* Delete comments */
		if ((hash = strchr(line, '#')))
			*hash = 0;

		if (line[0] == '[') {
			if (compare_md5(md5, line + 1) == 0) {
				active = 1;
			} else {
				active = 0;
			}
			continue;
		}

		if (!active) {
			continue;
		}

		delete_spaces(line);

		if (!(var = strtok(line, "=\n"))) {
			continue;
		}

		val = strtok(NULL, " \t\n");

		getval_yn("filter", o->dsp, XMP_DSP_LOWPASS);
		getval_yn("loop", o->loop, 1);
		getval_yn("mono", mono, 1);
		getval_yn("reverse", o->reverse, 1);
		getval_no("amplify", o->amplify);
		getval_no("mix", o->mix);
		getval_no("mixer_voices", o->numvoices);

		getval_tristate("fixloop", o->fixloop);
		getval_tristate("fx9bug", o->fx9bug);
		getval_tristate("vblank", o->vblank);
		getval_tristate("amiga", o->amiga);

		if (!strcmp(var, "interpolation")) {
			if (!strcmp(val, "nearest")) {
				o->interp = XMP_INTERP_NEAREST;
			} else if (!strcmp(val, "linear")) {
				o->interp = XMP_INTERP_LINEAR;
			} else if (!strcmp(val, "spline")) {
				o->interp = XMP_INTERP_SPLINE;
			} else {
				fprintf(stderr, "%s: unknown interpolation "
					"type \"%s\"\n", confname, val);
			}
		}

		if (!strcmp(var, "mode")) {
			for (pm = pmode; pm->name != NULL; pm++) {
				if (!strcmp(val, pm->name)) {
					o->player_mode = pm->mode;
					break;
				}
				fprintf(stderr, "%s: unknown player mode "
					"\"%s\"\n", confname, val);
			}
		}

		fprintf(stderr, "%s: unknown option \"%s\"\n", confname, var);
	}

	if (mono) {
		o->mix = 0;
	}

	fclose(rc);
}


void read_modconf(struct options *o, unsigned char *md5)
{
#if defined(__OS2__) || defined(__EMX__)
	char myrc[PATH_MAX];
	const char *home = getenv("HOME");
	if (!home) home = "C:";

	snprintf(myrc, PATH_MAX, "%s\\modules.conf", home);
	parse_modconf(o, "xmp-modules.conf", md5);
	parse_modconf(o, myrc, md5);
#elif defined __AMIGA__
	parse_modconf(o, "PROGDIR:modules.conf", md5);
#elif defined WIN32
	char myrc[PATH_MAX];
	const char *home = getenv("USERPROFILE");
	if (!home) home = "C:";

	snprintf(myrc, PATH_MAX, "%s/modules.conf", home);
	parse_modconf(o, myrc, md5);
#else
	char myrc[PATH_MAX];
	char *home = getenv("HOME");

	snprintf(myrc, PATH_MAX, "%s/.xmp/modules.conf", home);
	parse_modconf(o, SYSCONFDIR "/modules.conf", md5);
	parse_modconf(o, myrc, md5);
#endif
}
