/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stdio.h>
#include <xmp.h>
#include "common.h"

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#include <unistd.h>

static struct termios term;
#endif

int set_tty(void)
{
#ifdef HAVE_TERMIOS_H
	struct termios t;

	if (!isatty(STDIN_FILENO))
		return 0;
	if (tcgetattr(STDIN_FILENO, &term) < 0)
		return -1;

	t = term;
	t.c_lflag &= ~(ECHO | ICANON | TOSTOP);
	t.c_cc[VMIN] = t.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) < 0)
		return -1;
#endif

	return 0;
}

int reset_tty(void)
{
#ifdef HAVE_TERMIOS_H
	if (!isatty(STDIN_FILENO))
		return 0;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) < 0) {
		fprintf(stderr, "can't reset terminal!\n");
		return -1;
	}
#endif

	return 0;
}

