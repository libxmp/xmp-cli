/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
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

#if defined __EMX__
	char *home = getenv("HOME");

	snprintf(myrc, PATH_MAX, "%s\\.xmp\\xmp.conf", home);

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
	char *home = getenv("SystemRoot");

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
		fscanf(rc, "%255[^\n]", line);
		fgetc(rc);

		/* Delete comments */
		if ((hash = strchr(line, '#')))
			*hash = 0;

		delete_spaces(line);

		if (!(var = strtok(line, "=\n")))
			continue;

		val = strtok(NULL, " \t\n");

#define getval_yn(w,x,y) { \
	if (!strcmp(var,x)) { if (get_yesno (val)) w |= (y); \
	    else w &= ~(y); continue; } }

#define getval_no(x,y) { \
	if (!strcmp(var,x)) { y = atoi (val); continue; } }

		getval_yn(o->format, "8bit", XMP_FORMAT_8BIT);
		getval_yn(o->format, "mono", XMP_FORMAT_MONO);
		getval_yn(o->dsp, "filter", XMP_DSP_LOWPASS);
		getval_no("amplify", o->amplify);
		getval_no("mix", o->mix);
		/*getval_no("chorus", o->chorus);
		getval_no("reverb", o->reverb);*/
		getval_no("srate", o->rate);
		/*getval_no("time", o->time);
		getval_no("verbosity", o->verbosity);*/

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

		if (!strcmp(var, "loop")) {
			o->loop = get_yesno(val);
			continue;
		}

		if (!strcmp(var, "bits")) {
			if (atoi(val) == 8)
				o->format = XMP_FORMAT_8BIT;;
			continue;
		}

		if (!strcmp(var, "instrument_path")) {
			strncpy(instrument_path, val, 256);
			o->ins_path = instrument_path;
			continue;
		}
		/* If the line does not match any of the previous parameter,
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
