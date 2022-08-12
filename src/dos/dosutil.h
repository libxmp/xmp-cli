#ifndef __DOSUTIL_H__
#define __DOSUTIL_H__

extern int dpmi_allocate_dos_memory(int paragraphs, int *ret_selector_or_max);
extern int dpmi_free_dos_memory(int selector);
extern int dpmi_resize_dos_memory(int selector, int newpara, int *ret_max);

extern int dpmi_lock_linear_region(unsigned long address, unsigned long size);
extern int dpmi_unlock_linear_region(unsigned long address, unsigned long size);
extern int dpmi_lock_linear_region_base(void *address, unsigned long size);
extern int dpmi_unlock_linear_region_base(void *address, unsigned long size);

#ifdef __WATCOMC__
#include <conio.h>

#define inportb(x)    inp(x)
#define outportb(x,y) outp(x,y)

extern int enable();
extern int disable();
#pragma aux enable = "sti" "mov eax,1"
#pragma aux disable = "cli" "mov eax,1"

#else
#include <pc.h>
#endif

#endif
