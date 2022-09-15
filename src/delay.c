/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#include "common.h"

#if defined(_WIN32)
#include <windows.h>

void delay_ms(unsigned int msec) {
	Sleep(msec);
}

#elif defined(__OS2__)||defined(__EMX__)
#define INCL_DOSPROCESS
#include <os2.h>

void delay_ms(unsigned int msec) {
	DosSleep(msec);
}

#elif defined(_DOS)
#include <dos.h>

void delay_ms(unsigned int msec) {
	delay(msec); /* doesn't seem to use int 15h. */
}

#elif defined(XMP_AMIGA)
#ifdef __amigaos4__
#define __USE_INLINE__
#else
#endif
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct MsgPort *timerport;

#ifdef __amigaos4__
struct TimeRequest *timerio;
struct TimerIFace *ITimer;
#else
struct timerequest *timerio;
#endif

#ifdef __amigaos4__
struct Device *TimerBase = NULL;
#elif defined(__MORPHOS__) || defined(__VBCC__)
struct Library *TimerBase = NULL;
#else
struct Device *TimerBase = NULL;
#endif

static void amiga_atexit (void) {
	#ifdef __amigaos4__
	if (ITimer) {
		DropInterface((struct Interface *)ITimer);
	}
	#endif
	if (TimerBase) {
		WaitIO((struct IORequest *) timerio);
		CloseDevice((struct IORequest *) timerio);
		DeleteIORequest((struct IORequest *) timerio);
		DeleteMsgPort(timerport);
		TimerBase = NULL;
	}
}

void amiga_inittimer (void)
{
	timerport = CreateMsgPort();
	if (timerport != NULL) {
		#if defined(__amigaos4__)
		timerio = (struct TimeRequest *) CreateIORequest(timerport, sizeof(struct TimeRequest));
		#else
		timerio = (struct timerequest *) CreateIORequest(timerport, sizeof(struct timerequest));
		#endif
		if (timerio != NULL) {
			if (OpenDevice((STRPTR) TIMERNAME, UNIT_MICROHZ, (struct IORequest *) timerio, 0) == 0) {
				#if defined(__amigaos4__)
				TimerBase = timerio->Request.io_Device;
				ITimer = (struct TimerIFace *) GetInterface(TimerBase, "main", 1, NULL);
				#elif defined(__MORPHOS__) || defined(__VBCC__)
				TimerBase = (struct Library *)timerio->tr_node.io_Device;
				#else
				TimerBase = timerio->tr_node.io_Device;
				#endif
			} else {
				DeleteIORequest((struct IORequest *)timerio);
				DeleteMsgPort(timerport);
			}
		} else {
			DeleteMsgPort(timerport);
		}
	}
	if (!TimerBase) {
		fprintf(stderr, "Can't open timer.device\n");
		exit (-1);
	}

	/* 1us wait, for timer cleanup success */
	#if defined(__amigaos4__)
	timerio->Request.io_Command = TR_ADDREQUEST;
	timerio->Time.Seconds = 0;
	timerio->Time.Microseconds = 1;
	#else
	timerio->tr_node.io_Command = TR_ADDREQUEST;
	timerio->tr_time.tv_secs = 0;
	timerio->tr_time.tv_micro = 1;
	#endif
	SendIO((struct IORequest *) timerio);
	WaitIO((struct IORequest *) timerio);

	atexit (amiga_atexit);
}

void amiga_getsystime(void *tv)
{
	#ifdef __amigaos4__
	GetSysTime((struct TimeVal *)tv);
	#else
	GetSysTime((struct timeval *)tv);
	#endif
}

void delay_ms(unsigned int msec) {
	#if defined(__amigaos4__)
	timerio->Request.io_Command = TR_ADDREQUEST;
	timerio->Time.Seconds = msec / 1000000;
	timerio->Time.Microseconds = msec % 1000000;
	#else
	timerio->tr_node.io_Command = TR_ADDREQUEST;
	timerio->tr_time.tv_secs = msec / 1000000;
	timerio->tr_time.tv_micro = msec % 1000000;
	#endif
	SendIO((struct IORequest *) timerio);
	WaitIO((struct IORequest *) timerio);
}

#elif defined(HAVE_USLEEP)
#include <unistd.h>

void delay_ms(unsigned int msec) {
	usleep(msec * 1000);
}

#elif defined(HAVE_SELECT)

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#else
#  ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#  endif
#  ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#  endif
#  ifdef HAVE_UNISTD_H
#  include <unistd.h>
#  endif
#endif
#include <stddef.h>

void delay_ms(unsigned int msec) {
	struct timeval tv;
	long usec;

	usec = msec * 1000;
	tv.tv_sec = usec / 1000000;
	tv.tv_usec = usec % 1000000;
	select(0, NULL, NULL, NULL, &tv);
}

#else
#error Missing implementation of delay_ms()
#endif
