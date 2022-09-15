/* Extended Module Player
 * Copyright (C) 1996-2022 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"

char *xmp_strdup(const char *in)
{
	size_t len = strlen(in) + 1;
	char *out = (char *) malloc(len);
	if (out) {
		memcpy(out, in, len);
	}
	return out;
}


/* locale-insensitive tolower and strcasecmp: */

static inline int xmp_tolower(int c)
{
	return (c >= 'A' && c <= 'Z') ? (c | ('a' - 'A')) : c;
}

int xmp_strcasecmp(const char *s1, const char *s2)
{
	const char *p1 = s1;
	const char *p2 = s2;
	char c1, c2;

	if (p1 == p2)
		return 0;

	do
	{
		c1 = xmp_tolower (*p1++);
		c2 = xmp_tolower (*p2++);
		if (c1 == '\0')
			break;
	} while (c1 == c2);

	return (int)(c1 - c2);
}


int report(const char *fmt, ...)
{
	va_list a;
	int n;

	va_start(a, fmt);
	n = vfprintf(stderr, fmt, a);
	va_end(a);

	return n;
}
