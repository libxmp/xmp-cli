#include <unistd.h>
#include <xmp.h>
#include "common.h"

#ifdef __CYGWIN__
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
static int stdin_ready_for_reading()
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

static int read_key()
{
	char key;
	int ret;

	ret = 0;

#if defined HAVE_TERMIOS_H && !defined WIN32
#ifdef __CYGWIN__
	if (stdin_ready_for_reading())
#endif
		ret = read(0, &key, 1);
#elif defined WIN32
	if (kbhit()) {
		cmd = getch();
		ret = 1;
	}
#elif defined __AMIGA__
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

/* Interactive commands */

/* VT100 ESC sequences:
 * ESC [ A - up arrow
 * ESC [ B - down arrow
 * ESC [ C - right arrow
 * ESC [ D - left arrow
 */
void read_command(xmp_context handle, struct control *ctl)
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
	case 'q':		/* quit */
	cmd_quit:
		xmp_stop_module(handle);
		ctl->pause = 0;
		ctl->skip = -2;
		break;
	case 'f':		/* jump to next order */
	cmd_next_pos:
		xmp_next_position(handle);
		ctl->pause = 0;
		break;
	case 'b':		/* jump to previous order */
	cmd_prev_pos:
		xmp_prev_position(handle);
		ctl->pause = 0;
		break;
	case 'n':		/* skip to next module */
	cmd_next_mod:
		xmp_stop_module(handle);
		ctl->pause = 0;
		ctl->skip = 1;
		break;
	case 'p':		/* skip to previous module */
	cmd_prev_mod:
		xmp_stop_module(handle);
		ctl->pause = 0;
		ctl->skip = -1;
		break;
	case 'l':
		ctl->loop ^= 1;
		break;
	case ' ':		/* paused module */
		ctl->pause ^= 1;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		xmp_channel_mute(handle, cmd - '1', -1);
		break;
	case '0':
		xmp_channel_mute(handle, 9, -1);
		break;
	case '!': {
		int i;
		for (i = 0; i < 10; i++) {
			xmp_channel_mute(handle, i, 0);
		}
		break; }
	case '?':
	case 'i':
	case 'I':
	case 'S':
	case 'm':
		ctl->display = cmd;
		break;
	}
}
