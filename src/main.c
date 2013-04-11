/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <xmp.h>
#include "errno.h"
#include "sound.h"
#include "common.h"

#ifdef WIN32
#include <windows.h>
#endif

extern int optind;

static struct sound_driver *sound;
static unsigned int foreground_in, foreground_out;
static int refresh_status;


int report(char *fmt, ...)
{
	va_list a;
	int n;

	va_start(a, fmt);
	n = vfprintf(stderr, fmt, a);
	va_end(a);

	return n;
}

#ifdef HAVE_SIGNAL_H
static void cleanup(int sig)
{
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
	signal(SIGFPE, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);

	sound->deinit();
	reset_tty();

	signal(sig, SIG_DFL);
#ifdef HAVE_KILL
	kill(getpid(), sig);
#endif
}
#endif

#ifdef SIGTSTP
static void sigtstp_handler(int n)
{
	report("\n");
	signal(SIGTSTP, SIG_DFL);
#ifdef HAVE_KILL
	kill(getpid(), SIGTSTP);
#endif
}

static void sigcont_handler(int sig)
{
#ifdef HAVE_TERMIOS_H
	unsigned int old_in = foreground_in;

	foreground_in  = tcgetpgrp(STDIN_FILENO)  == getpgrp();
	foreground_out = tcgetpgrp(STDERR_FILENO) == getpgrp();

	if (old_in != foreground_in)
		/* Only call if it was not already prepared */
		set_tty();
#endif

	if (sig != 0)
		refresh_status = 1;

	signal(SIGCONT, sigcont_handler);
	signal(SIGTSTP, sigtstp_handler);
}
#endif

static void show_info(int what, struct xmp_module_info *mi)
{
	report("\r%78.78s\n", " ");
	switch (what) {
	case '?':
		info_help();
		break;
	case 'i':
		info_ins_smp(mi);
		break;
	case 'I':
		info_instruments(mi);
		break;
	case 'S':
		info_samples(mi);
		break;
	case 'm':
		info_mod(mi);
		break;
	}
}

static void shuffle(int argc, char **argv)
{
	int i, j;
	char *x;

	for (i = 1; i < argc; i++) {
		j = 1 + rand() % (argc - 1);
		x = argv[i];
		argv[i] = argv[j];
		argv[j] = x;
	}
}

static void check_pause(xmp_context xc, struct control *ctl,
	struct xmp_module_info *mi, struct xmp_frame_info *fi, int verbose)
{
	if (ctl->pause) {
		sound->pause();
		if (verbose) {
			info_frame(mi, fi, ctl, 1);
		}
		while (ctl->pause) {
			usleep(100000);
			read_command(xc, ctl);
			if (ctl->display) {
				show_info(ctl->display, mi);
				if (verbose) {
					info_frame(mi, fi, ctl, 1);
				}
				ctl->display = 0;
			}
		}
		sound->resume();
	}
}

int main(int argc, char **argv)
{
	xmp_context xc;
	struct xmp_module_info mi;
	struct xmp_frame_info fi;
	struct options opt, save_opt;
	struct control control;
	int i;
	int first;
	int skipprev;
	FILE *f = NULL;
	int val, lf_flag;
#ifndef WIN32
	struct timeval tv;
	struct timezone tz;
	int flags;

	gettimeofday(&tv, &tz);
	srand(tv.tv_usec);
#else
	srand(GetTickCount());
#endif

	init_sound_drivers();

	memset(&opt, 0, sizeof (struct options));
	memset(&control, 0, sizeof (struct control));

	/* set defaults */
	opt.verbose = 1;
	opt.rate = 44100;
	opt.mix = -1;
	opt.driver_id = NULL;
	opt.interp = XMP_INTERP_SPLINE;
	opt.dsp = XMP_DSP_LOWPASS;

	/* read configuration file */
	if (!opt.norc) {
		read_config(&opt);
	}

	get_options(argc, argv, &opt);

	if (!opt.probeonly && optind >= argc) {
		fprintf(stderr, "%s: no modules to play\n"
			"Use `%s --help' for more information.\n",
			argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	if (opt.interp < 0) {
		fprintf(stderr, "%s: unknown interpolation type\n"
			"Use `%s --help' for more information.\n",
			argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	if (opt.silent) {
		opt.driver_id = "null";
	}

	sound = select_sound_driver(&opt);

	if (sound == NULL) {
		fprintf(stderr, "%s: can't initialize sound", argv[0]);
		if (opt.driver_id != NULL) {
			fprintf(stderr, " (driver = %s)", opt.driver_id);
		}
		fprintf(stderr, "\n");

		if (f != NULL) {
			fclose(f);
		}
		exit(EXIT_FAILURE);
	}

	if (opt.verbose > 0) {
		report("Extended Module Player " VERSION "\n"
			"Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr\n");

		report("Using %s\n", sound->description);

		report("Mixer set to %d Hz, %dbit, %s%s%s\n", opt.rate,
		    opt.format & XMP_FORMAT_8BIT ? 8 : 16,
		    opt.interp == XMP_INTERP_LINEAR ? "linear interpolated " :
		    opt.interp == XMP_INTERP_SPLINE ? "cubic spline interpolated " : "",
		    opt.format & XMP_FORMAT_MONO ? "mono" : "stereo",
		    opt.dsp & XMP_DSP_LOWPASS ? "" : " (no filter)");
	}

	if (opt.probeonly) {
		exit(EXIT_SUCCESS);
	}

	if (opt.random) {
		shuffle(argc - optind + 1, &argv[optind - 1]);
	}

#ifdef HAVE_SIGNAL_H
	signal(SIGTERM, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGFPE, cleanup);
	signal(SIGSEGV, cleanup);
#ifdef SIGQUIT
	signal(SIGQUIT, cleanup);
#endif
#ifdef SIGTSTP
	signal(SIGCONT, sigcont_handler);
	signal(SIGTSTP, sigtstp_handler);
#endif
#endif

	sigcont_handler(0);
	xc = xmp_create_context();

	skipprev = 0;

	if (opt.ins_path) {
		xmp_set_instrument_path(xc, opt.ins_path);
	}

	lf_flag = 0;
	memcpy(&save_opt, &opt, sizeof (struct options));

	for (first = optind; optind < argc; optind++) {
		memcpy(&opt, &save_opt, sizeof (struct options));

		if (opt.verbose > 0) {
			if (lf_flag)
				report("\n");
			lf_flag = 1;
			report("Loading %s... (%d of %d)\n",
				argv[optind], optind - first + 1, argc - first);
		}

		val = xmp_load_module(xc, argv[optind]);

		if (val < 0) {
			char *msg;

			val = -val;
			switch (val) {
			case XMP_ERROR_FORMAT:
				msg = "Unrecognized file format";
				break;
			case XMP_ERROR_DEPACK:
				msg = "Error depacking file";
				break;
			case XMP_ERROR_LOAD:
				msg = "Error loading module";
				break;
			case XMP_ERROR_SYSTEM:
				msg = strerror(errno);
				break;
			default:
				msg = "Unknown error";
			}

			fprintf(stderr, "%s: %s: %s\n", argv[0],
						argv[optind], msg);
			if (skipprev) {
		        	optind -= 2;
				if (optind < first) {
					optind += 2;
				}
			}
			continue;
		}

		xmp_get_module_info(xc, &mi);
		if (!opt.norc) {
			read_modconf(&opt, mi.md5);
		}

		skipprev = 0;
		control.time = 0.0;
		control.loop = opt.loop;
		
		if (opt.sequence) {
			if (opt.sequence < mi.num_sequences) {
				if (mi.seq_data[opt.sequence].duration > 0) {
					opt.start = mi.seq_data[opt.sequence].entry_point;
				}
			}
			opt.sequence = 0;
		}

		if (xmp_start_player(xc, opt.rate, opt.format) == 0) {
			xmp_set_player(xc, XMP_PLAYER_INTERP, opt.interp);
			xmp_set_player(xc, XMP_PLAYER_DSP, opt.dsp);

			if (opt.mix >= 0) {
				xmp_set_player(xc, XMP_PLAYER_MIX, opt.mix);
			}

			if (opt.reverse) {
				int mix;
				mix = xmp_get_player(xc, XMP_PLAYER_MIX);
				xmp_set_player(xc, XMP_PLAYER_MIX, -mix);
			}

			xmp_set_position(xc, opt.start);

			/* Mute channels */

			for (i = 0; i < XMP_MAX_CHANNELS; i++) {
				xmp_channel_mute(xc, i, opt.mute[i]);
			}

			/* Set player flags */

#define set_flag(x,y,z) do { \
  if ((y) > 0) (x) |= (z); \
  else if ((y) < 0) (x) &= ~ (z); \
} while (0)

			flags = 0;
			set_flag(flags, opt.vblank, XMP_FLAGS_VBLANK);
			set_flag(flags, opt.fx9bug, XMP_FLAGS_FX9BUG);
			set_flag(flags, opt.fixloop, XMP_FLAGS_FIXLOOP);

			xmp_set_player(xc, XMP_PLAYER_FLAGS, flags);
#if XMP_VERCODE < 0x040003
			if (flags & XMP_FLAGS_VBLANK) {
				xmp_scan_module(xc);
			}
#endif

			/* Show module data */

			if (opt.verbose > 0) {
				info_mod(&mi);
			}

			if (opt.verbose > 1) {
				info_instruments(&mi);
			}
	
			/* Play module */

			refresh_status = 1;
			info_frame_init();

			fi.loop_count = 0;
			while (!opt.info && xmp_play_frame(xc) == 0) {
				int old_loop = fi.loop_count;
				
				xmp_get_frame_info(xc, &fi);
				if (!control.loop && old_loop != fi.loop_count)
					break;

				sigcont_handler(0);
				if (foreground_out && opt.verbose > 0) {
					info_frame(&mi, &fi, &control, refresh_status);
					refresh_status = 0;
				}

				control.time += 1.0 * fi.frame_time / 1000;

				sound->play(fi.buffer, fi.buffer_size);

				if (foreground_in && !opt.nocmd) {
					read_command(xc, &control);

					if (control.display) {
						show_info(control.display, &mi);
						control.display = 0;
						refresh_status = 1;
					}
				}

				if (opt.max_time > 0 &&
				    control.time > opt.max_time) {
					break;
				}

				check_pause(xc, &control, &mi, &fi,
							opt.verbose);

				opt.start = 0;
			}

			xmp_end_player(xc);
		}

		xmp_release_module(xc);

		if (!opt.info) {
			report("\n");
		}

		if (control.skip == -1) {
			optind -= optind > first ? 2 : 1;
			skipprev = 1;
		} else if (control.skip == -2) {
			goto end;
		}
		control.skip = 0;
	}

	sound->flush();

    end:
	xmp_free_context(xc);

	if (foreground_in)
		reset_tty();

	sound->deinit();

	exit(EXIT_SUCCESS);
}
