/* Extended Module Player
 * Copyright (C) 1996-2012 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See doc/COPYING
 * for more information.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <xmp.h>

#include "common.h"
#include "sound.h"
#include "list.h"

extern char *optarg;
static char *token;

#ifdef HAVE_SYS_RTPRIO_H
extern int rt;
#endif

extern struct list_head sound_driver_list;


#define OPT_FX9BUG	0x105
#define OPT_PROBEONLY	0x106
#define OPT_STDOUT	0x109
#define OPT_STEREO	0x10a
#define OPT_NOCMD	0x10b
#define OPT_REALTIME	0x10c
#define OPT_FIXLOOP	0x10d
#define OPT_CRUNCH	0x10e
#define OPT_VBLANK	0x10f
#define OPT_SHOWTIME	0x110
#define OPT_DUMP	0x111

static void usage(char *s)
{
	struct list_head *head;
	struct sound_driver *sd;
	char **hlp;

	printf("Usage: %s [options] [modules]\n", s);

	printf("\nAvailable drivers:\n");

	list_for_each(head, &sound_driver_list) {
		sd = list_entry(head, struct sound_driver, list);
		printf("    %s (%s)\n", sd->id, sd->description);
	}

	list_for_each(head, &sound_driver_list) {
		sd = list_entry(head, struct sound_driver, list);
		if (sd->help)
			printf("\n%s options:\n", sd->description);
		for (hlp = sd->help; hlp && *hlp; hlp += 2)
			printf("   -D%-20.20s %s\n", hlp[0], hlp[1]);
	}

	printf("\nPlayer control options:\n"
"   -D parameter[=val]     Pass configuration parameter to the output driver\n"
"   -d --driver name       Force output to the specified device\n"
"   -l --loop              Enable module looping\n"
"   -M --mute ch-list      Mute the specified channels\n"
"   --nocmd                Disable interactive commands\n"
"   -R --random            Random order playing\n"
"   -S --solo ch-list      Set channels to solo mode\n"
"   -s --start num         Start from the specified order\n"
"   -t --time num          Maximum playing time in seconds\n"
"\nMixer options:\n"
"   -a --amplify {0|1|2|3} Amplification factor: 0=Normal, 1=x2, 2=x4, 3=x8\n"
"   -b --bits {8|16}       Software mixer resolution (8 or 16 bits)\n"
"   -c --stdout            Mix the module to stdout\n"
"   -f --frequency rate    Sampling rate in hertz (default 44100)\n"
"   -m --mono              Mono output\n"
"   -N --null              Use null output driver (same as --driver=null)\n"
"   -n --nearest           Use nearest neighbor interpolation (no filter)\n"
"   -F --nofilter          Disable IT lowpass filters\n"
"   -o --output-file name  Mix the module to file ('-' for stdout)\n"
"   -P --pan pan           Percentual pan separation\n"
"   -u --unsigned          Set the mixer to use unsigned samples\n"
"\nEnvironment options:\n"
"   -I --instrument-path   Set pathname to external samples\n"
"\nInformation options:\n"
"   -h --help              Display a summary of the command line options\n"
"   -i --info              Display module information and exit\n"
"   -L --list-formats      List supported module formats\n"
"   --probe-only           Probe audio device and exit\n"
"   -q --quiet             Quiet mode (verbosity level = 0)\n"
"   -V --version           Print version information\n"
"   -v --verbose           Verbose mode (incremental)\n");
}

static struct option lopt[] = {
	{ "amplify",		1, 0, 'a' },
	{ "bits",		1, 0, 'b' },
	{ "driver",		1, 0, 'd' },
	{ "frequency",		1, 0, 'f' },
	{ "help",		0, 0, 'h' },
	{ "instrument-path",	1, 0, 'I' },
	{ "info",		0, 0, 'i' },
	{ "list-formats",	0, 0, 'L' },
	{ "loop",		0, 0, 'l' },
	{ "mono",		0, 0, 'm' },
	{ "mute",		1, 0, 'M' },
	{ "null",		0, 0, 'N' },
	{ "nearest",		0, 0, 'n' },
	{ "nocmd",		0, 0, OPT_NOCMD },
	{ "nofilter",		0, 0, 'F' },
	{ "output-file",	1, 0, 'o' },
	{ "pan",		1, 0, 'P' },
	{ "probe-only",		0, 0, OPT_PROBEONLY },
	{ "quiet",		0, 0, 'q' },
	{ "random",		0, 0, 'R' },
	{ "solo",		1, 0, 'S' },
	{ "start",		1, 0, 's' },
	{ "stdout",		0, 0, 'c' },
	{ "tempo",		1, 0, 'T' },
	{ "time",		1, 0, 't' },
	{ "unsigned",		0, 0, 'u' },
	{ "version",		0, 0, 'V' },
	{ "verbose",		0, 0, 'v' },
	{ NULL,			0, 0, 0   }
};

void get_options(int argc, char **argv, struct options *options)
{
	int optidx = 0;
	int dparm = 0;
	int o;

#define OPTIONS "a:b:cD:d:Ff:hI:iLlM:mNno:P:qRS:s:T:t:uVv"
	while ((o = getopt_long(argc, argv, OPTIONS, lopt, &optidx)) != -1) {
		switch (o) {
		case 'a':
			options->amplify = atoi(optarg);
			break;
		case 'b':
			if (atoi(optarg) == 8) {
				options->format |= XMP_FORMAT_8BIT;
			}
			break;
		case 'c':
			options->driver_id = "file";
			options->out_file = "-";
			break;
		case 'D':
			options->driver_parm[dparm++] = optarg;
			break;
		case 'd':
			options->driver_id = optarg;
			break;
		case 'F':
			options->dsp &= ~XMP_DSP_LOWPASS;;
			break;
		case 'f':
			options->rate = strtoul(optarg, NULL, 0);
			break;
		case 'I':
			options->ins_path = optarg;
			break;
		case 'i':
			options->info = 1;
			options->silent = 1;
			break;
		case 'L': {
			char **list;
			int i;
			list = xmp_get_format_list();
			for (i = 0; list[i] != NULL; i++) {
				printf("%d:%s\n", i + 1, list[i]);
			}
			exit(EXIT_SUCCESS);
			break; }
		case 'l':
			options->loop = 1;
			break;
		case 'm':
			options->format |= XMP_FORMAT_MONO;
			break;
		case 'N':
			options->silent = 1;
			break;
		case 'n':
			options->interp = XMP_INTERP_NEAREST;
			break;
		case OPT_NOCMD:
			options->nocmd = 1;
			break;
		case 'o':
			options->out_file = optarg;
			if (strlen(optarg) >= 4 &&
			    !strcasecmp(optarg + strlen(optarg) - 4, ".wav")) {
				options->driver_id = "wav";
			} else {
				options->driver_id = "file";
			}
			break;
		case 'P':
			options->mix = strtoul(optarg, NULL, 0);
			if (options->mix < 0)
				options->mix = 0;
			if (options->mix > 100)
				options->mix = 100;
			break;
		case OPT_PROBEONLY:
			options->probeonly = 1;
			break;
		case 'q':
			options->verbose = 0;
			break;
		case 'R':
			options->random = 1;
			break;
		case 'M':
		case 'S':
			if (o == 'S') {
				memset(options->mute, 1, XMP_MAX_CHANNELS);
			}
			token = strtok(optarg, ",");
			while (token) {
				int a, b;
				char buf[40];
				memset(buf, 0, 40);
				if (strchr(token, '-')) {
					b = strcspn(token, "-");
					strncpy(buf, token, b);
					a = atoi(buf);
					strncpy(buf, token + b + 1,
						strlen(token) - b - 1);
					b = atoi(buf);
				} else {
					a = b = atoi(token);
				}
				for (; b >= a; b--) {
					if (b < XMP_MAX_CHANNELS)
						options->mute[b] = (o == 'M');
				}
				token = strtok(NULL, ",");
			}
			break;
		case 's':
			options->start = strtoul(optarg, NULL, 0);
			break;
		case 't':
			options->max_time = strtoul(optarg, NULL, 0) * 1000;
			break;
		case 'u':
			options->format |= XMP_FORMAT_UNSIGNED;
			break;
		case 'V':
			puts("Extended Module Player " VERSION);
			exit(0);
		case 'v':
			options->verbose++;
			break;
		case 'h':
			usage(argv[0]);
		default:
			exit(-1);
		}
	}

	/* Set limits */
	if (options->rate < 1000)
		options->rate = 1000;	/* Min. rate 1 kHz */
	if (options->rate > 48000)
		options->rate = 48000;	/* Max. rate 48 kHz */
}
