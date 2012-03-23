#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <xmp.h>
#include "sound.h"
#include "common.h"

#ifdef WIN32
#include <windows.h>
#endif

extern int optind;
extern struct sound_driver SOUND_DRIVER;
extern struct sound_driver sound_null;

struct sound_driver *sound = &SOUND_DRIVER;

static int background = 0;
static int refresh_status;

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
	fprintf(stderr, "\n");
	signal(SIGTSTP, SIG_DFL);
#ifdef HAVE_KILL
	kill(getpid(), SIGTSTP);
#endif
}

static void sigcont_handler(int sig)
{
#ifdef HAVE_TERMIOS_H
	background = (tcgetpgrp(0) == getppid());

	if (!background) {
		set_tty();
	}
#endif

	refresh_status = 1;

	signal(SIGCONT, sigcont_handler);
	signal(SIGTSTP, sigtstp_handler);
}
#endif

static void show_info(int what, struct xmp_module_info *mi)
{
	printf("\r%78.78s\n", " ");
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

static void check_pause(xmp_context handle, struct control *ctl,
                        struct xmp_module_info *mi)
{
	if (ctl->pause) {
		sound->pause();
		info_frame(mi, ctl, 1);
		while (ctl->pause) {
			usleep(100000);
			read_command(handle, ctl);
			if (ctl->display) {
				show_info(ctl->display, mi);
				info_frame(mi, ctl, 1);
				ctl->display = 0;
			}
		}
		sound->resume();
	}
}

int main(int argc, char **argv)
{
	xmp_context handle;
	struct xmp_module_info mi;
	struct options options;
	struct control control;
	int i;
	int first;
	int skipprev;
	FILE *f = NULL;
#ifndef WIN32
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	srand(tv.tv_usec);
#else
	srand(GetTickCount());
#endif

	memset(&options, 0, sizeof (struct options));
	memset(&control, 0, sizeof (struct control));
	options.verbose = 1;
	options.freq = 44100;

	get_options(argc, argv, &options);

	if (optind >= argc) {
		fprintf (stderr, "%s: no modules to play\n"
			"Use `%s --help' for more information.\n",
			argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}

	if (options.random) {
		shuffle(argc - optind + 1, &argv[optind - 1]);
	}

	if (options.silent) {
		sound = &sound_null;
	}

	if (options.out_file) {
		f = fopen(options.out_file, "wb");
		if (f == NULL) {
			perror(options.out_file);
			exit(EXIT_FAILURE);
		}
	}

	if (sound->init(&options.freq, &options.format) < 0) {
		fprintf(stderr, "%s: can't initialize sound\n", argv[0]);
		if (f != NULL) {
			fclose(f);
		}
		exit(EXIT_FAILURE);
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

#ifdef HAVE_TERMIOS_H
	background = (tcgetpgrp (0) == getppid ());

	if (!background) {
		set_tty();
	}
#endif

	handle = xmp_create_context();

	skipprev = 0;

	for (first = optind; optind < argc; optind++) {
		printf("\nLoading %s... (%d of %d)\n",
			argv[optind], optind - first + 1, argc - first);

		if (xmp_load_module(handle, argv[optind]) < 0) {
			fprintf(stderr, "%s: error loading %s\n", argv[0],
				argv[optind]);
			if (skipprev) {
		        	optind -= 2;
				if (optind < first) {
					optind += 2;
				}
			}
			continue;
		}
		skipprev = 0;
		control.time = 0.0;
		control.loop = options.loop;
		
		if (xmp_player_start(handle, options.freq, options.format) == 0) {
			xmp_set_position(handle, options.start);

			/* Mute channels */

			for (i = 0; i < XMP_MAX_CHANNELS; i++) {
				xmp_channel_mute(handle, i, options.mute[i]);
			}

			/* Show module data */

			xmp_player_get_info(handle, &mi);

			if (options.verbose > 0) {
				info_mod(&mi);
			}
			if (options.verbose == 2) {
				info_instruments(&mi);
			}
	
			/* Play module */

			refresh_status = 1;
			info_frame_init(&mi);

			while (!options.info && xmp_player_frame(handle) == 0) {
				int old_loop = mi.loop_count;
				
				xmp_player_get_info(handle, &mi);
				if (!control.loop && old_loop != mi.loop_count)
					break;

				if (!background) {
					info_frame(&mi, &control, refresh_status);
					refresh_status = 0;
				}

				control.time += 1.0 * mi.frame_time / 1000;

				sound->play(mi.buffer, mi.buffer_size);

				if (options.out_file) {
					fwrite(mi.buffer, mi.buffer_size, 1, f);
				}

				if (!background) {
					read_command(handle, &control);

					if (control.display) {
						show_info(control.display, &mi);
						control.display = 0;
						refresh_status = 1;
					}
				}

				check_pause(handle, &control, &mi);

				options.start = 0;
			}

			xmp_player_end(handle);
		}

		xmp_release_module(handle);

		if (!options.info) {
			printf("\n");
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
	xmp_free_context(handle);

	if (!background) {
		reset_tty();
	}

	sound->deinit();

	if (options.out_file) {
		fclose(f);
	}

	exit(EXIT_SUCCESS);
}
