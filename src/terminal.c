#include <stdio.h>
#include <xmp.h>
#include "common.h"

#ifdef HAVE_TERMIOS_H
#include <termios.h>

static struct termios term;
#endif

int set_tty()
{
#ifdef HAVE_TERMIOS_H
	struct termios t;

	if (tcgetattr(0, &term) < 0)
		return -1;

	t = term;
	t.c_lflag &= ~(ECHO | ICANON | TOSTOP);
	t.c_cc[VMIN] = t.c_cc[VTIME] = 0;

	if (tcsetattr(0, TCSAFLUSH, &t) < 0)
		return -1;
#endif

	return 0;
}

int reset_tty()
{
#ifdef HAVE_TERMIOS_H
	if (tcsetattr(0, TCSAFLUSH, &term) < 0) {
		fprintf(stderr, "can't reset terminal!\n");
		return -1;
	}
#endif

	return 0;
}

