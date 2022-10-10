/* Extended Module Player
 * Copyright (C) 1996-2021 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

/* SoundBlaster/Pro/16/AWE32 output driver for DOS from libMikMod,
 * authored by Andrew Zabolotny <bit@eltech.ru>, with further SB16
 * fixes by O. Sezer <sezero@users.sourceforge.net>.
 * Timer callback functionality replaced by a push mechanism.
 */

#include <string.h>
#include "dos/dossb.h"
#include "sound.h"

/* The last buffer byte filled with sound */
static unsigned int buff_tail = 0;

static int write_sb_output(char *data, unsigned int siz) {
	unsigned int dma_size, dma_pos;
	unsigned int cnt;

	sb_query_dma(&dma_size, &dma_pos);
	/* There isn't much sense in filling less than 256 bytes */
	dma_pos &= ~255;

	/* If nothing to mix, quit */
	if (buff_tail == dma_pos)
		return 0;

	/* If DMA pointer still didn't wrapped around ... */
	if (dma_pos > buff_tail) {
		if ((cnt = dma_pos - buff_tail) > siz)
			cnt = siz;
		memcpy(sb.dma_buff->linear + buff_tail, data, cnt);
		buff_tail += cnt;
		/* If we arrived right to the DMA buffer end, jump to the beginning */
		if (buff_tail >= dma_size)
			buff_tail = 0;
	} else {
		/* If wrapped around, fill first to the end of buffer */
		if ((cnt = dma_size - buff_tail) > siz)
			cnt = siz;
		memcpy(sb.dma_buff->linear + buff_tail, data, cnt);
		buff_tail += cnt;
		siz -= cnt;
		if (!siz) return cnt;

		/* Now fill from buffer beginning to current DMA pointer */
		if (dma_pos > siz) dma_pos = siz;
		data += cnt;
		cnt += dma_pos;

		memcpy(sb.dma_buff->linear, data, dma_pos);
		buff_tail = dma_pos;
	}
	return cnt;
}

static int init(struct options *options)
{
	const char *card = NULL;
	const char *mode = NULL;
	int bits = 0;

	if (!sb_open()) {
		fprintf(stderr, "Sound Blaster initialization failed.\n");
		return -1;
	}

	if (options->rate < 4000) options->rate = 4000;
	if (sb.caps & SBMODE_16BITS) {
		card = "16";
		mode = "stereo";
		bits = 16;
		options->format &= ~XMP_FORMAT_8BIT;
	} else {
		options->format |= XMP_FORMAT_8BIT|XMP_FORMAT_UNSIGNED;
	}
	if (sb.caps & SBMODE_STEREO) {
		if (!card) {
			card = "Pro";
			mode = "stereo";
			bits = 8;
		}
		if (options->rate > sb.maxfreq_stereo)
			options->rate = sb.maxfreq_stereo;
		options->format &= ~XMP_FORMAT_MONO;
	} else {
		mode = "mono";
		card = (sb.dspver < SBVER_20)? "1" : "2";
		bits = 8;
		if (options->rate > sb.maxfreq_mono)
			options->rate = sb.maxfreq_mono;
		options->format |= XMP_FORMAT_MONO;
	}

	/* Enable speaker output */
	sb_output(TRUE);

	/* Set our routine to be called during SB IRQs */
	buff_tail = 0;
	sb.timer_callback = NULL;/* see above  */

	/* Start cyclic DMA transfer */
	if (!sb_start_dma(((sb.caps & SBMODE_16BITS) ? SBMODE_16BITS | SBMODE_SIGNED : 0) |
						(sb.caps & SBMODE_STEREO), options->rate)) {
		sb_output(FALSE);
		sb_close();
		fprintf(stderr, "Sound Blaster: DMA start failed.\n");
		return -1;
	}

	printf("Sound Blaster %s or compatible (%d bit, %s, %u Hz)\n", card, bits, mode, options->rate);

	return 0;
}

static void deinit(void)
{
	sb.timer_callback = NULL;
	sb_output(FALSE);
	sb_stop_dma();
	sb_close();
}

static void play(void *data, int siz) {
	int i;
	for (;;) {
		i = write_sb_output((char *)data, siz);
		if ((siz -= i) <= 0) return;
		data = (char *)data + i;
		/*delay_ms(1);*/
	}
}

static void flush(void)
{
}

static void onpause(void) {
	const int silence = (sb.caps & SBMODE_16BITS) ? 0 : 0x80;
	memset(sb.dma_buff->linear, silence, sb.dma_buff->size);
}

static void onresume(void)
{
}

struct sound_driver sound_sb = {
	"sb",
	"Sound Blaster for DOS",
	NULL,
	init,
	deinit,
	play,
	flush,
	onpause,
	onresume
};
