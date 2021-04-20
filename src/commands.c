/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <unistd.h>
#if defined(_WIN32) || defined(__OS2__)
#include <conio.h>
#endif
#if defined(AMIGA) || defined(__AMIGA__) || defined(__AROS__)
#ifdef __amigaos4__
#define __USE_INLINE__
#endif
#include <proto/exec.h>
#include <proto/dos.h>
#endif
#include <xmp.h>
#include "common.h"
#include "keys.h"

#ifdef __CYGWIN__
#include <sys/select.h>

/*
 * from	daniel åkerud <daniel.akerud@gmail.com>
 * date	Tue, Jul 28, 2009 at 9:59 AM
 *
 * Under Cygwin, the read() in process_echoback blocks because VTIME = 0
 * is not handled correctly. To fix this you can either:
 *
 * 1. Enable "tty emulation" in Cygwin using an environment variable.
 * http://www.mail-archive.com/cygwin@cygwin.com/msg99417.html
 * For me this is _very_ slow and I can see the characters as they are
 * typed out when running xmp. I have not investigated why this is
 * happening, but there is of course a reason why this mode is not
 * enabled by default.
 * 
 * 2. Do a select() before read()ing if the platform is Cygwin.
 * This makes Cygwin builds work out of the box with no fiddling around,
 * but does impose a neglectible cpu overhead (for Cygwin builds only).
 */
static int stdin_ready_for_reading(void)
{
	fd_set fds;
	struct timeval tv;
	int ret;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

	if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds))
		return 1;

	return 0;
}
#endif

static int read_key(void)
{
	char key;
	int ret = 0;

#if defined(HAVE_TERMIOS_H) && !defined(_WIN32)
#ifdef __CYGWIN__
	if (stdin_ready_for_reading())
#endif
		ret = read(0, &key, 1);
#elif defined(_WIN32) || defined(__OS2__)
	if (kbhit()) {
		key = getch();
		ret = 1;
	}
#elif defined(AMIGA) || defined(__AMIGA__) || defined(__AROS__)
	/* Amiga CLI */
	{
		BPTR in = Input();
		if (WaitForChar(in, 1)) {
			Read(in, &key, 1);
			ret = 1;
		}
	}
#else
	ret = 0;
#endif

	if (ret <= 0) {
		return -1;
	}

	return key;
}

static void change_sequence(xmp_context handle, struct xmp_module_info *mi, struct control *ctl, int i)
{
	int seq = ctl->sequence;

	seq += i;

	if (seq >= mi->num_sequences) {
		seq = 0;
	} else if (seq < 0) {
		seq = mi->num_sequences - 1;
	}

	if (seq == ctl->sequence) {
		info_message("Sequence not changed: only one sequence available");
	} else {
		ctl->sequence = seq;
		info_message("Change to sequence %d", seq);
		xmp_set_position(handle, mi->seq_data[seq].entry_point);
	}
}

/* Interactive commands */

/* VT100 ESC sequences:
 * ESC [ A - up arrow
 * ESC [ B - down arrow
 * ESC [ C - right arrow
 * ESC [ D - left arrow
 */
void read_command(xmp_context handle, struct xmp_module_info *mi, struct control *ctl)
{
	int cmd;

	cmd = read_key();	
	if (cmd <= 0)
		return;

	switch (cmd) {
	case 0x1b:		/* escape */
		cmd = read_key();
		if (cmd != '[')
			goto cmd_quit;
		cmd = read_key();
		switch (cmd) {
		case 'A':
			goto cmd_next_mod;
		case 'B':
			goto cmd_prev_mod;
		case 'C':
			goto cmd_next_pos;
		case 'D':
			goto cmd_prev_pos;
		}

		break;

	#ifdef XMP_MOD_NEXT_2
	case XMP_MOD_NEXT_2:
		goto cmd_next_mod;
		break;
	#endif
	#ifdef XMP_MOD_BACK_2
	case XMP_MOD_BACK_2:
		goto cmd_prev_mod;
		break;
	#endif
	#ifdef XMP_PAT_NEXT_2
	case XMP_PAT_NEXT_2:
		goto cmd_next_pos;
		break;
	#endif
	#ifdef XMP_PAT_BACK_2
	case XMP_PAT_BACK_2:
		goto cmd_prev_pos;
		break;
	#endif

	case XMP_QUIT:		/* quit */
	#ifdef XMP_QUIT_2
	case XMP_QUIT_2:
	#endif
	cmd_quit:
		xmp_stop_module(handle);
		ctl->pause = 0;
		ctl->skip = -2;
		break;
	case XMP_PAT_NEXT:		/* jump to next order */
	cmd_next_pos:
		xmp_next_position(handle);
		ctl->pause = 0;
		break;
	case XMP_PAT_BACK:		/* jump to previous order */
	cmd_prev_pos:
		xmp_prev_position(handle);
		ctl->pause = 0;
		break;
	case XMP_MOD_NEXT:		/* skip to next module */
	cmd_next_mod:
		xmp_stop_module(handle);
		ctl->pause = 0;
		ctl->skip = 1;
		break;
	case XMP_MOD_BACK:		/* skip to previous module */
	cmd_prev_mod:
		xmp_stop_module(handle);
		ctl->pause = 0;
		ctl->skip = -1;
		break;
	case XMP_LOOP:
		/* this explains why the loop toggle was
		 * broken, it was just poorly coded
		 * ctl->loop++;
		 * ctl->loop %= 3;
		*/
		if(ctl->loop == 0) ctl-> loop = 1;
		else if(ctl->loop == 1) ctl-> loop = 0;
		break;
	case XMP_MIXER:
		ctl->cur_info = XMP_MIXER;
		break;
	case XMP_AMIGA: {
		int f;

		ctl->amiga_mixer = !ctl->amiga_mixer;

		/* set player flags */
		f = xmp_get_player(handle, XMP_PLAYER_FLAGS);
		if (ctl->amiga_mixer) {
			xmp_set_player(handle, XMP_PLAYER_FLAGS,
						f | XMP_FLAGS_A500);
		} else {
			xmp_set_player(handle, XMP_PLAYER_FLAGS,
						f &= ~XMP_FLAGS_A500);
		}

		/* set current module flags */
		f = xmp_get_player(handle, XMP_PLAYER_CFLAGS);
		if (ctl->amiga_mixer) {
			xmp_set_player(handle, XMP_PLAYER_CFLAGS,
						f | XMP_FLAGS_A500);
		} else {
			xmp_set_player(handle, XMP_PLAYER_CFLAGS,
						f &= ~XMP_FLAGS_A500);
		}
		break; }
	case XMP_CURR_SEQ:
		ctl->cur_info = XMP_CURR_SEQ;
		break;
	case XMP_EXPLORER:
		ctl->explore ^= 1;
		break;
	case XMP_PAUSE:		/* paused module */
	#ifdef XMP_PAUSE_2
	case XMP_PAUSE_2:
	#endif
		ctl->pause ^= 1;
		break;
#define mute(A) xmp_channel_mute(handle, A, 2); 
	case XMP_MUTE_1:
		mute(0);
		break;
	case XMP_MUTE_2:
		mute(1);
		break;
	case XMP_MUTE_3:
		mute(2);
		break;
	case XMP_MUTE_4:
		mute(3);
		break;
	case XMP_MUTE_5:
		mute(4);
		break;
	case XMP_MUTE_6:
		mute(5);
		break;
	case XMP_MUTE_7:
		mute(6);
		break;
	case XMP_MUTE_8:
		mute(7);
		break;
	case XMP_MUTE_9:
		mute(8);
		break;
	case XMP_MUTE_10:
		mute(9);
		break;
	case XMP_MUTE_ALL: {
		int i;
		for (i = 0; i < 10; i++) {
			xmp_channel_mute(handle, i, 0);
		}
		break; }
	case XMP_HELP_2:
	case XMP_COMMENT:
	case XMP_FULL_INFO:
	case XMP_INST_INFO:
	case XMP_SAMPLE_INFO:
	#ifdef XMP_CLEAR
	case XMP_CLEAR:
	#endif
	case XMP_MODULE_INFO:
		ctl->display = cmd;
		break;
	case XMP_HELP:
		ctl->display = '?';
		break;
	case XMP_SEQ_NEXT:
		change_sequence(handle, mi, ctl, 1);
		break;
	case XMP_SEQ_BACK:
		change_sequence(handle, mi, ctl, -1);
		break;
	}
}
