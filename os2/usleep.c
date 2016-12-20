#define INCL_DOSPROCESS
#include <os2.h>
void usleep (unsigned long usec)
{
	DosSleep((usec >= 1000)? (usec / 1000) : 1);
}
