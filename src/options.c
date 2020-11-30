/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)
#include <getopt.h>
#else
#include "getopt_long.h"
#endif
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

enum {
	OPT_FX9BUG = 0x105,
	OPT_PROBEONLY,
	OPT_LOADONLY,
	OPT_NOCMD,
	OPT_VBLANK,
	OPT_FIXLOOP,
	OPT_NORC,
	OPT_LOOPALL,
	OPT_NUMVOICES,
};

struct player_mode pmode[] = {
	{ "auto",         "Autodetect",                XMP_MODE_AUTO },
	{ "mod",          "Generic MOD player",        XMP_MODE_MOD },
	{ "noisetracker", "Noisetracker",              XMP_MODE_NOISETRACKER },
	{ "protracker",   "Protracker 2",              XMP_MODE_PROTRACKER },
	{ "s3m",          "Generic S3M player",        XMP_MODE_S3M },
	{ "st3",          "Scream Tracker 3",          XMP_MODE_ST3 },
	{ "st3gus",       "Scream Tracker 3 with GUS", XMP_MODE_ST3GUS },
	{ "xm",	          "Generic XM player",         XMP_MODE_XM },
	{ "ft2",          "Fasttracker II",            XMP_MODE_FT2 },
	{ "it",           "Impulse Tracker",           XMP_MODE_IT },
	{ "itsmp",        "Impulse Tracker sample mode", XMP_MODE_ITSMP },
	{ NULL, NULL, 0 }
};

static void usage(char *s, struct options *options)
{
	struct list_head *head;
	struct sound_driver *sd;
	struct player_mode *pm;
	const char *const *hlp;

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

	printf("\nAvailable player modes:\n");
	for (pm = pmode; pm->name != NULL; pm++) {
		printf("   %-22.22s %s\n", pm->name, pm->desc);
	}

	printf("\nPlayer control options:\n"
"   -D parameter[=val]     Pass configuration parameter to the output driver\n"
"   -d --driver name       Force output to the specified device\n"
"   -e --player-mode mode  Force tracker emulation (default auto)\n"
"   --fix-sample-loops     Use sample loop start /2 in MOD/UNIC/NP3\n"
"   -l --loop              Enable module looping\n"
"   --loop-all             Loop over entire module list\n"
"   -M --mute ch-list      Mute the specified channels\n"
"   --nocmd                Disable interactive commands\n"
"   --norc                 Don't read configuration files\n"
"   -R --random            Random order playing\n"
"   -S --solo ch-list      Set channels to solo mode\n"
"   -s --start num         Start from the specified order\n"
"   -t --time num          Maximum playing time in seconds\n"
"   --vblank               Force vblank timing in Amiga modules\n"
"   -Z --all-sequences     Play all sequences (subsongs) in module\n"
"   -z --sequence num      Play the specified sequence (0=main)\n" 
"\nMixer options:\n"
"   -A --amiga             Use Paula simulation mixer in Amiga formats\n"
"   -a --amplify {0|1|2|3} Amplification factor: 0=Normal, 1=x2, 2=x4, 3=x8\n"
"   -b --bits {8|16}       Software mixer resolution (8 or 16 bits)\n"
"   -c --stdout            Mix the module to stdout\n"
"   -f --frequency rate    Sampling rate in hertz (default 44100)\n"
"   -i --interpolation {nearest|linear|spline}\n"
"                          Select interpolation type (default spline)\n"
"   --mixer-voices num     Maximum number of mixer voices (default %d)\n"
"   -m --mono              Mono output\n"
"   -n --null              Use null output driver (same as --driver=null)\n"
"   -F --nofilter          Disable IT lowpass filters\n"
"   -o --output-file name  Mix the module to file ('-' for stdout)\n"
"   -P --pan pan           Percentual pan separation\n"
"   -p --default-pan       Percentual default pan setting (default %d%%)\n"
"   -r --reverse           Reverse left/right stereo channels\n"
"   -u --unsigned          Set the mixer to use unsigned samples\n"
"\nEnvironment options:\n"
"   -I --instrument-path   Set pathname to external samples\n"
"\nInformation options:\n"
"   -C --show-comment      Display the module comment text, if any\n"
"   -h --help              Display a summary of the command line options\n"
"   -L --list-formats      List supported module formats\n"
"   --probe-only           Probe audio device and exit\n"
"   --load-only            Load module and exit\n"
"   -q --quiet             Quiet mode (verbosity level = 0)\n"
"   -V --version           Print version information\n"
"   -v --verbose           Verbose mode (incremental)\n",
		options->numvoices, options->defpan);
}

static const struct option lopt[] = {
	{ "amiga",		0, 0, 'A' },
	{ "amplify",		1, 0, 'a' },
	{ "bits",		1, 0, 'b' },
	{ "driver",		1, 0, 'd' },
	{ "default-pan",	1, 0, 'p' },
	{ "fix-sample-loops",	0, 0, OPT_FIXLOOP },
	{ "frequency",		1, 0, 'f' },
	{ "help",		0, 0, 'h' },
	{ "instrument-path",	1, 0, 'I' },
	{ "interpolation",	1, 0, 'i' },
	{ "list-formats",	0, 0, 'L' },
	{ "loop",		0, 0, 'l' },
	{ "loop-all",		0, 0, OPT_LOOPALL },
	{ "mixer-voices",	1, 0, OPT_NUMVOICES },
	{ "mono",		0, 0, 'm' },
	{ "mute",		1, 0, 'M' },
	{ "null",		0, 0, 'N' },
	{ "nocmd",		0, 0, OPT_NOCMD },
	{ "norc",		0, 0, OPT_NORC },
	{ "nofilter",		0, 0, 'F' },
	/* { "offset-bug-emulation",0, 0, OPT_FX9BUG }, */
	{ "output-file",	1, 0, 'o' },
	{ "pan",		1, 0, 'P' },
	{ "player-mode",	1, 0, 'e' },
	{ "probe-only",		0, 0, OPT_PROBEONLY },
	{ "load-only",		0, 0, OPT_LOADONLY },
	{ "quiet",		0, 0, 'q' },
	{ "random",		0, 0, 'R' },
	{ "reverse",		0, 0, 'r' },
	{ "show-comment",	0, 0, 'C' },
	{ "solo",		1, 0, 'S' },
	{ "start",		1, 0, 's' },
	{ "stdout",		0, 0, 'c' },
	{ "tempo",		1, 0, 'T' },
	{ "time",		1, 0, 't' },
	{ "unsigned",		0, 0, 'u' },
	{ "vblank",		0, 0, OPT_VBLANK },
	{ "version",		0, 0, 'V' },
	{ "verbose",		0, 0, 'v' },
	{ "all-sequences",	1, 0, 'Z' },
	{ "sequence",		1, 0, 'z' },
	{ NULL,			0, 0, 0   }
};

void get_options(int argc, char **argv, struct options *options)
{
	struct player_mode *pm;
	int optidx = 0;
	int o;

#define OPTIONS "Aa:b:CcD:d:e:Ff:hI:i:LlM:mNo:P:p:qRrS:s:T:t:uVvZz:"
	while ((o = getopt_long(argc, argv, OPTIONS, lopt, &optidx)) != -1) {
		switch (o) {
		case 'A':
			options->amiga_mixer = 1;
			break;
		case 'a':
			options->amplify = atoi(optarg);
			break;
		case 'b':
			if (atoi(optarg) == 8) {
				options->format |= XMP_FORMAT_8BIT;
			}
			break;
		case 'C':
			options->show_comment = 1;
			break;
		case 'c':
			options->driver_id = "file";
			options->out_file = "-";
			break;
		case 'D':
			if (options->dparm < MAX_DRV_PARM)
				options->driver_parm[options->dparm++] = optarg;
			break;
		case 'd':
			options->driver_id = optarg;
			break;
		case 'e':
			for (pm = pmode; pm->name != NULL; pm++) {
				if (!strcmp(optarg, pm->name)) {
					options->player_mode = pm->mode;
					break;
				}
				options->player_mode = -1;
			}

			break;
		case 'F':
			options->dsp &= ~XMP_DSP_LOWPASS;
			break;
		case 'f':
			options->rate = strtoul(optarg, NULL, 0);
			break;
		case OPT_FIXLOOP:
			options->fixloop = 1;
			break;
		case 'I':
			options->ins_path = optarg;
			break;
		case 'i':
			if (!strcmp(optarg, "nearest")) {
				options->interp = XMP_INTERP_NEAREST;
			} else if (!strcmp(optarg, "linear")) {
				options->interp = XMP_INTERP_LINEAR;
			} else if (!strcmp(optarg, "spline")) {
				options->interp = XMP_INTERP_SPLINE;
			} else {
				options->interp = -1;
			}
			break;
		case OPT_LOADONLY:
			options->info = 1;
			options->silent = 1;
			break;
		case 'L': {
			const char* const * list;
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
		case OPT_LOOPALL:
			options->loop = 2;
			break;
		case OPT_NUMVOICES:
			options->numvoices = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			options->format |= XMP_FORMAT_MONO;
			break;
		case 'N':
			options->silent = 1;
			break;
		case OPT_NOCMD:
			options->nocmd = 1;
			break;
		case OPT_NORC:
			options->norc = 1;
			break;
		case 'o':
			options->out_file = optarg;
			if (strlen(optarg) >= 4 &&
			    !strcasecmp(optarg + strlen(optarg) - 4, ".wav")) {
				options->driver_id = "wav";
			} else if (strlen(optarg) >= 5 &&
			    !strcasecmp(optarg + strlen(optarg) - 5, ".aiff")) {
				options->driver_id = "aiff";
			} else {
				options->driver_id = "file";
			}
			break;
		/* case OPT_FX9BUG:
			options->fx9bug = 1;
			break; */
		case 'P':
			options->mix = strtoul(optarg, NULL, 0);
			if (options->mix < 0)
				options->mix = 0;
			if (options->mix > 100)
				options->mix = 100;
			break;
		case 'p':
			options->defpan = strtoul(optarg, NULL, 0);
			if (options->defpan < 0)
				options->defpan = 0;
			if (options->defpan > 100)
				options->defpan = 100;
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
		case 'r':
			options->reverse = 1;
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
		case OPT_VBLANK:
			options->vblank = 1;
			break;
		case 'V':
			puts("Extended Module Player " VERSION);
			exit(0);
		case 'v':
			options->verbose++;
			break;
		case 'Z':
			options->explore = 1;
			break;
		case 'z':
			options->sequence = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			usage(argv[0], options);
			/* fall through */
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
