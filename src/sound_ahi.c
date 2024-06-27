/* Amiga AHI driver for Extended Module Player
 * Based on a MikMod driver written by Szilard Biro, which was loosely
 * based on an old AmigaOS4 version by Fredrik Wikstrom.
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#ifdef __amigaos4__
#define SHAREDMEMFLAG MEMF_SHARED
#define __USE_INLINE__
#else
#define SHAREDMEMFLAG MEMF_PUBLIC
#endif

#include <stdlib.h>
#include <string.h>
#include "sound.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/ahi.h>

#define BUFFERSIZE (4 << 10)

static struct MsgPort *AHImp = NULL;
static struct AHIRequest *AHIReq[2] = { NULL, NULL };
static int active = 0;
static signed char *AHIBuf[2] = { NULL, NULL };

static void closeLibs(void) {
    if (AHIReq[1]) {
        AHIReq[0]->ahir_Link = NULL; /* in case we are linked to req[0] */
        if (!CheckIO((struct IORequest *) AHIReq[1])) {
            AbortIO((struct IORequest *) AHIReq[1]);
            WaitIO((struct IORequest *) AHIReq[1]);
        }
        FreeVec(AHIReq[1]);
        AHIReq[1] = NULL;
    }
    if (AHIReq[0]) {
        if (!CheckIO((struct IORequest *) AHIReq[0])) {
            AbortIO((struct IORequest *) AHIReq[0]);
            WaitIO((struct IORequest *) AHIReq[0]);
        }
        if (AHIReq[0]->ahir_Std.io_Device) {
            CloseDevice((struct IORequest *) AHIReq[0]);
            AHIReq[0]->ahir_Std.io_Device = NULL;
        }
        DeleteIORequest((struct IORequest *) AHIReq[0]);
        AHIReq[0] = NULL;
    }
    if (AHImp) {
        DeleteMsgPort(AHImp);
        AHImp = NULL;
    }
    if (AHIBuf[0]) {
        FreeVec(AHIBuf[0]);
        AHIBuf[0] = NULL;
    }
    if (AHIBuf[1]) {
        FreeVec(AHIBuf[1]);
        AHIBuf[1] = NULL;
    }
}

static int init(struct options *options) {
    AHImp = CreateMsgPort();
    if (AHImp) {
        AHIReq[0] = (struct AHIRequest *)CreateIORequest(AHImp, sizeof(struct AHIRequest));
        if (AHIReq[0]) {
            AHIReq[0]->ahir_Version = 4;
            AHIReq[1] = AllocVec(sizeof(struct AHIRequest), SHAREDMEMFLAG);
            if (AHIReq[1]) {
                if (!OpenDevice(AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)AHIReq[0], 0)) {
                    AHIReq[0]->ahir_Std.io_Command = CMD_WRITE;
                    AHIReq[0]->ahir_Std.io_Data = NULL;
                    AHIReq[0]->ahir_Std.io_Offset = 0;
                    AHIReq[0]->ahir_Frequency = options->rate;
                    AHIReq[0]->ahir_Type = (options->format & XMP_FORMAT_8BIT)?
                                            ((options->format & XMP_FORMAT_MONO)? AHIST_M8S  : AHIST_S8S ) :
                                            ((options->format & XMP_FORMAT_MONO)? AHIST_M16S : AHIST_S16S);
                    AHIReq[0]->ahir_Volume = 0x10000;
                    AHIReq[0]->ahir_Position = 0x8000;

                    CopyMem(AHIReq[0], AHIReq[1], sizeof(struct AHIRequest));

                    AHIBuf[0] = AllocVec(BUFFERSIZE, SHAREDMEMFLAG | MEMF_CLEAR);
                    if (AHIBuf[0]) {
                        AHIBuf[1] = AllocVec(BUFFERSIZE, SHAREDMEMFLAG | MEMF_CLEAR);
                        if (AHIBuf[1]) {
                            /* driver is initialized before calling libxmp, so this is OK : */
                            options->format &= ~XMP_FORMAT_UNSIGNED;/* no unsigned with AHI */
                            return 0;
                        }
                    }
                }
            }
        }
    }

    closeLibs();
    return -1;
}

static void deinit(void) {
    closeLibs();
}

static void play(void *b, int len) {
    signed char *in = (signed char *)b;
    int chunk;
    while (len > 0) {
        if (AHIReq[active]->ahir_Std.io_Data) {
            WaitIO((struct IORequest *) AHIReq[active]);
        }
        chunk = (len < BUFFERSIZE)? len : BUFFERSIZE;
        memcpy(AHIBuf[active], in, chunk);
        len -= chunk;
        in += chunk;

        AHIReq[active]->ahir_Std.io_Data = AHIBuf[active];
        AHIReq[active]->ahir_Std.io_Length = chunk;
        AHIReq[active]->ahir_Link = !CheckIO((struct IORequest *) AHIReq[active ^ 1]) ? AHIReq[active ^ 1] : NULL;
        SendIO((struct IORequest *)AHIReq[active]);
        active ^= 1;
    }
}

static void flush(void) {
}

static void onpause(void) {
}

static void onresume(void) {
}

const struct sound_driver sound_ahi = {
    "ahi",
    "Amiga AHI audio",
    NULL,
    init,
    deinit,
    play,
    flush,
    onpause,
    onresume
};

