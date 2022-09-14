/* Extended Module Player
 * Copyright (C) 1996-2022 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <xmp.h>
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

