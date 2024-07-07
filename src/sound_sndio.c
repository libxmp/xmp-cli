/*
 * Copyright (c) 2009 Thomas Pfaff <tpfaff@tp76.info>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndio.h>
#include "sound.h"

static struct sio_hdl *hdl;

static int init(struct options *options)
{
	struct sio_par par, askpar;

	hdl = sio_open(NULL, SIO_PLAY, 0);
	if (hdl == NULL) {
		fprintf(stderr, "%s: failed to open audio device\n", __func__);
		return -1;
	}

	sio_initpar(&par);
	par.pchan = options->format & XMP_FORMAT_MONO ? 1 : 2;
	par.rate = options->rate;
	par.le = SIO_LE_NATIVE;
	par.appbufsz = par.rate / 4;

	if (options->format & XMP_FORMAT_8BIT) {
		par.bits = 8;
		par.sig = 0;
		options->format |= XMP_FORMAT_UNSIGNED;
	} else {
		par.bits = 16;
		par.sig = 1;
		options->format &= ~XMP_FORMAT_UNSIGNED;
	}

	askpar = par;
	if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par)) {
		fprintf(stderr, "%s: failed to set parameters\n", __func__);
		goto error;
	}

	if ((par.bits == 16 && par.le != askpar.le) ||
	    par.bits != askpar.bits ||
	    par.sig != askpar.sig ||
	    par.pchan != askpar.pchan ||
	    (par.rate * 1000 < askpar.rate *  995) ||
	    (par.rate * 1000 > askpar.rate * 1005)) {
		fprintf(stderr, "%s: parameters not supported\n", __func__);
		goto error;
	}

	if (!sio_start(hdl)) {
		fprintf(stderr, "%s: failed to start audio device\n", __func__);
		goto error;
	}
	return 0;

    error:
	sio_close(hdl);
	return -1;
}

static void deinit(void)
{
	sio_close(hdl);
	hdl = NULL;
}

static void play(void *buf, int len)
{
	if (buf != NULL) {
		sio_write(hdl, buf, len);
	}
}

static void flush(void)
{
}

static void onpause(void)
{
}

static void onresume(void)
{
}

static const char *description(void)
{
	return "OpenBSD sndio";
}

const struct sound_driver sound_sndio = {
	"sndio",
	NULL,
	description,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
