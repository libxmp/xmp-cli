/* Interface for DOS IRQ routines, from libMikMod.
 * Copyright (C) 1999 by Andrew Zabolotny <bit@eltech.ru>
 *
 * Extended Module Player
 * Copyright (C) 1996-2021 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU General Public License. See the COPYING
 * file for more information.
 */

#ifndef __DOSIRQ_H__
#define __DOSIRQ_H__

#include "dosutil.h"

#define PIC1_BASE	0x20		/* PIC1 base */
#define PIC2_BASE	0xA0		/* PIC2 base */

#ifdef __GNUC__
#define NO_REORDER __attribute__((no_reorder))
#else
#define NO_REORDER
#endif

#ifdef __WATCOMC__
#define INTERRUPT_ATTRIBUTES __interrupt __far
#else
#define INTERRUPT_ATTRIBUTES
#endif
typedef void (INTERRUPT_ATTRIBUTES *irq_handler) ();

struct irq_handle {
	irq_handler c_handler;		/* The real interrupt handler */
	unsigned long handler_size;	/* The size of interrupt handler */
#ifdef __DJGPP__
	unsigned long handler;		/* Interrupt wrapper address */
	unsigned long prev_selector;	/* Selector of previous handler */
	unsigned long prev_offset;	/* Offset of previous handler */
#else
	irq_handler   prev_vect;	/* The previous interrupt handler */
#endif
	unsigned char irq_num;		/* IRQ number */
	unsigned char int_num;		/* Interrupt number */
	unsigned char pic_base;		/* PIC base (0x20 or 0xA0) */
	unsigned char pic_mask;		/* Old PIC mask state */
};

/* Return the enabled state for specific IRQ */
static inline unsigned char irq_state(struct irq_handle * irq)
{
	return ((~inportb(irq->pic_base + 1)) & (0x01 << (irq->irq_num & 7)));
}

/* Acknowledge the end of interrupt */
static inline void _irq_ack(int irqno)
{
	outportb(irqno > 7 ? PIC2_BASE : PIC1_BASE, 0x60 | (irqno & 7));
	/* For second controller we also should acknowledge first controller */
	if (irqno > 7)
		outportb(PIC1_BASE, 0x20);	/* 0x20, 0x62? */
}

/* Acknowledge the end of interrupt */
static inline void irq_ack(struct irq_handle * irq)
{
	outportb(irq->pic_base, 0x60 | (irq->irq_num & 7));
	/* For second controller we also should acknowledge first controller */
	if (irq->pic_base != PIC1_BASE)
		outportb(PIC1_BASE, 0x20);	/* 0x20, 0x62? */
}

/* Mask (disable) the particular IRQ given his ordinal */
static inline void _irq_disable(int irqno)
{
	unsigned int port_no = (irqno < 8 ? PIC1_BASE : PIC2_BASE) + 1;
	outportb(port_no, inportb(port_no) | (1 << (irqno & 7)));
}

/* Unmask (enable) the particular IRQ given its ordinal */
static inline void _irq_enable(int irqno)
{
	unsigned int port_no = (irqno < 8 ? PIC1_BASE : PIC2_BASE) + 1;
	outportb(port_no, inportb(port_no) & ~(1 << (irqno & 7)));
}

/* Mask (disable) the particular IRQ given its irq_handle structure */
static inline void irq_disable(struct irq_handle * irq)
{
	outportb(irq->pic_base + 1,
                 inportb(irq->pic_base + 1) | (1 << (irq->irq_num & 7)));
}

/* Unmask (enable) the particular IRQ given its irq_handle structure */
static inline void irq_enable(struct irq_handle * irq)
{
	outportb(irq->pic_base + 1,
                 inportb(irq->pic_base + 1) & ~(1 << (irq->irq_num & 7)));
}

/* Check if a specific IRQ is pending: return 0 is no */
static inline int irq_check(struct irq_handle * irq)
{
	outportb(irq->pic_base, 0x0B);	/* Read IRR vector */
	return (inportb(irq->pic_base) & (1 << (irq->irq_num & 7)));
}

/* Hook a specific IRQ; NOTE: IRQ is disabled upon return, irq_enable() it */
extern struct irq_handle *irq_hook(int irqno, irq_handler handler,
                                   void (*end)());
/* Unhook a previously hooked IRQ */
extern void irq_unhook(struct irq_handle * irq);
/* Start IRQ detection process (IRQ list is given with irq mask) */
/* irq_confirm should return "1" if the IRQ really comes from the device */
extern void irq_detect_start(unsigned int irqs,
                             int (*irq_confirm) (int irqno));
/* Finish IRQ detection process */
extern void irq_detect_end();
/* Get the count of specific irqno that happened */
extern int irq_detect_get(int irqno, unsigned int *irqmask);
/* Clear IRQ counters */
extern void irq_detect_clear();

/* The size of interrupt stack */
extern unsigned int __irq_stack_size;
/* The number of nested interrupts that can be handled */
extern unsigned int __irq_stack_count;

#endif /* __DOSIRQ_H__ */
