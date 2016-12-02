#define INCL_DOSPROCESS
#include <os2.h>
void usleep (unsigned long usec)
{
	DosSleep(usec ? (usec/1000l) : 1l);
}
