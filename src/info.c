#include <stdio.h>
#include <string.h>
#include <xmp.h>
#include "common.h"

static int max_channels = -1;

void info_help(void)
{
	printf(
"COMMAND KEYS SUMMARY\n"
"     Space      Pause/unpause\n"
"    F, Right    Advance to next order\n"
"    B, Left     Return to previous order\n"
"    N, Up       Advance to next module\n"
"    P, Down     Return to previous module\n"
"    1 - 0       Mute/unmute channels\n"
"      !         Unmute all channels\n"
"      ?         Display available commands\n"
"      m         Display module information\n"
"      i         Display combined instrument/sample list\n"
"      I         Display instrument list\n"
"      S         Display sample list\n"
);
}

void info_mod(struct xmp_module_info *mi)
{
	int i;

	printf("Module name  : %s\n", mi->mod->name);
	printf("Module type  : %s\n", mi->mod->type);
	printf("Module length: %d patterns\n", mi->mod->len);
	printf("Patterns     : %d\n", mi->mod->pat);
	printf("Instruments  : %d\n", mi->mod->ins);
	printf("Samples      : %d\n", mi->mod->smp);
	printf("Channels     : %d [ ", mi->mod->chn);

	for (i = 0; i < mi->mod->chn; i++) {
		if (mi->mod->xxc[i].flg & XMP_CHANNEL_SYNTH) {
			printf("S ");
		} else {
			printf("%x ", mi->mod->xxc[i].pan >> 4);
		}
	}
	printf("]\n");

	printf("Duration     : %dmin%02ds", (mi->total_time + 500) / 60000,
					((mi->total_time + 500) / 1000) % 60);
	if (mi->num_sequences > 1) {
		printf(" (main sequence)\n");
		for (i = 1; i < mi->num_sequences; i++) {
			int dur = mi->sequence[i].duration;

			if (dur == 0) {
				continue;
			}

			printf("               %dmin%02ds "
				"(sequence at position %d)\n",
				(dur + 500) / 60000, ((dur + 500) / 1000) % 60,
				mi->sequence[i].entry_point);
		}
	} else {
		printf("\n");
	}
}

void info_frame_init(struct xmp_module_info *mi)
{
	max_channels = 0;
}

void info_frame(struct xmp_module_info *mi, struct control *ctl, int reprint)
{
	static int ord = -1, spd = -1, bpm = -1;
	int time;

	if (mi->virt_used > max_channels)
		max_channels = mi->virt_used;

	if (!reprint && mi->frame != 0)
		return;

	time = ctl->time / 100;

	if (reprint || mi->order != ord || mi->bpm != bpm || mi->speed != spd) {
	        printf("\rSpeed[%02X] BPM[%02X] Pos[%02X/%02X] "
			 "Pat[%02X/%02X] Row[  /  ] Chn[  /  ]      0:00:00.0",
					mi->speed, mi->bpm,
					mi->order, mi->mod->len - 1,
					mi->pattern, mi->mod->pat - 1);
		ord = mi->order;
		bpm = mi->bpm;
		spd = mi->speed;
	}

	printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
	       "%02X/%02X] Chn[%02X/%02X] %c  ",
		mi->row, mi->num_rows - 1, mi->virt_used, max_channels,
		ctl->loop ? 'L' : ' ');

	if (ctl->pause) {
		printf(" - PAUSED -");
	} else {
		printf("%3d:%02d:%02d.%d",
			(int)(time / (60 * 600)), (int)((time / 600) % 60),
			(int)((time / 10) % 60), (int)(time % 10));
	}

	fflush(stdout);
}

void info_ins_smp(struct xmp_module_info *mi)
{
	int i, j;
	struct xmp_module *mod = mi->mod;

	printf("Instruments and samples:\n");
	printf("   Instrument name                  Smp  Size  Loop  End    Vol Fine Xpo Pan\n");
	for (i = 0; i < mod->ins; i++) {
		struct xmp_instrument *ins = &mod->xxi[i];

		if (strlen(ins->name) == 0 && ins->nsm == 0) {
			continue;
		}

		printf("%02x %-32.32s ", i, ins->name);

		for (j = 0; j < ins->nsm; j++) {
			struct xmp_subinstrument *sub = &ins->sub[j];
			struct xmp_sample *smp = &mod->xxs[sub->sid];

			if (j > 0) {
				if (smp->len == 0) {
					continue;
				}
				printf("%36.36s", " ");
			}

			printf("[%02x] %05x%c%05x %05x%c V%02x %+04d %+03d P%02x\n",
				sub->sid,
				smp->len,
				smp->flg & XMP_SAMPLE_16BIT ? '+' : ' ',
				smp->lps,
				smp->lpe,
				smp->flg & XMP_SAMPLE_LOOP ?
					smp->flg & XMP_SAMPLE_LOOP_BIDIR ?
						'B' : 'L' : ' ',
				sub->vol,
				sub->fin,
				sub->xpo,
				sub->pan & 0xff);
		}

		if (j == 0) {
			printf("[  ] ----- ----- -----  --- ---- --- ---\n");
		}

	}
}

void info_instruments(struct xmp_module_info *mi)
{
	int i, j;
	struct xmp_module *mod = mi->mod;

	printf("Instruments:\n");
	printf("   Instrument name                  Vl Rls  Env Ns Sub  Gv Vl Fine Xpo Pan Sm\n");
	for (i = 0; i < mod->ins; i++) {
		struct xmp_instrument *ins = &mod->xxi[i];

		if (strlen(ins->name) == 0 && ins->nsm == 0) {
			continue;
		}

		printf("%02x %-32.32s %02x %04x %c%c%c %02x ", i, ins->name,
			ins->vol, ins->rls,
			ins->aei.flg & XMP_ENVELOPE_ON ? 'A' : '-',
			ins->fei.flg & XMP_ENVELOPE_ON ? 'F' : '-',
			ins->pei.flg & XMP_ENVELOPE_ON ? 'P' : '-',
			ins->nsm
		);

		for (j = 0; j < ins->nsm; j++) {
			struct xmp_subinstrument *sub = &ins->sub[j];
			struct xmp_sample *smp = &mod->xxs[sub->sid];

			if (j > 0) {
				if (smp->len == 0) {
					continue;
				}
				printf("%51.51s", " ");
			}

			printf("[%02x] %02x %02x %+04d %+03d P%02x %02x\n",
				j,
				sub->gvl,
				sub->vol,
				sub->fin,
				sub->xpo,
				sub->pan,
				sub->sid);
		}

		if (j == 0) {
			printf("[  ] -- -- ---- --- --- --\n");
		}

	}
}

void info_samples(struct xmp_module_info *mi)
{
	int i;
	struct xmp_module *mod = mi->mod;

	printf("Samples:\n");
	printf("   Sample name                      Length Start  End    Flags\n");
	for (i = 0; i < mod->ins; i++) {
		struct xmp_sample *smp = &mod->xxs[i];

		if (strlen(smp->name) == 0 && smp->len == 0) {
			continue;
		}

		printf("%02x %-32.32s %06x %06x %06x %s %s %s\n",
			i, smp->name,
			smp->len,
			smp->lps,
			smp->lpe,
			smp->flg & XMP_SAMPLE_16BIT ? "16" : "--",
			smp->flg & XMP_SAMPLE_LOOP  ? "L"  : "-",
			smp->flg & XMP_SAMPLE_LOOP_BIDIR ? "B" : "-");
	}
}

