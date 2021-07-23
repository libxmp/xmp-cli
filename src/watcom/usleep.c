#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>
void usleep (unsigned long usec) {
	DosSleep (usec / 1000);
}
#endif

#ifdef _DOS
#include <dos.h>
void usleep (unsigned long usec) {
	delay (usec / 1000); /* doesn't seem to use int 15h. */
}
#endif
