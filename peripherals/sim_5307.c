/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*#define TRACER_OFF*/
#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(sim);

typedef unsigned char __u8;
typedef u16 __u16;
typedef u32 __u32;

struct _sim_5307 {
	/* MBar only needs 22 bits, we have 24 here, so we could store mbar
	 * in the first 3 bytes of the sim..... ah well.... */
	__u8	rsr;		/* 000: reset status register */
	__u8	sypcr;		/* 001: system protection control register */
	__u8	swivr;		/* 002: software watchdog interrupt vector register */
	__u8	swsr;		/* 003: software watchdog service register */
	__u16	par;		/* 004: pin assignment register */
	__u8	irqpar;		/* 006: interrupt port assignment register */
	__u8	pad00;		/* 007: reserved */
	__u8	pllcr;		/* 008: pll control */
	__u8	pad01[0x03];	/* 009: reserved */
	__u8	mpark;		/* 00c: default bus master park register */
	__u8	pad02[0x03];	/* 00d: reserved */
	__u8	pad03[0x30];	/* 010: 0x10->0x3f reserved */
	__u32	ipr;		/* 040: interrupt pending register */
	__u32	imr;		/* 044: interrupt mask register */
	__u8	pad04[0x03];	/* 048: reserved */
	__u8	avr;		/* 04b: autovector register */
	__u8	icr[10];	/* 04c: Interrupt Control Registers
				 *	icr0: software watchdog timer
				 *	icr1: timer0
				 *	icr2: timer1
				 *	icr3: i2c
				 *	icr4: uart0
				 *	icr5: uart1
				 *	icr6: dma0
				 *	icr7: dma1
				 *	icr8: dma2
				 *	icr9: dma3 */
	__u8	pad05[0x02];	/* 056: reserved*/
};
		    


static struct _sim_register *sim_register_lookup_by_offset(s32 offset);
static struct _sim_register *sim_register_lookup_by_name(char *name);
static void sim_interrupt_assert(s16 number, s16 vector);
static void sim_interrupt_withdraw(s16 number);
struct _sim sim_data;

/* name, offset, width, read, write, resetvalue, description */
static struct _sim_register sim_reg[] = {
	{"RSR",   0x0000, 8, 1, 1, 0x00, "Reset Status Register"}, 
	{"SYPCR", 0x0001, 8, 1, 1, 0x00, "System Protection Control Register"}, 
	{"SWIVR", 0x0002, 8, 1, 1, 0x0f, "Software Watchdog Interrupt Vector Register"}, 
	{"SWSR",  0x0003, 8, 1, 1, 0x00, "Software Watchdog Service Register"}, 
	{"PAR",   0x0004,16, 1, 1, 0x0000, "Pin Assignment Register"}, 
	{"IRQPAR",0x0006, 8, 1, 1, 0x00, "Interrupt Port Assignment Register"}, 
	{"PLLCR", 0x0008, 8, 1, 1, 0x00, "PLL control Register"}, 
	{"MPARK", 0x000c, 8, 1, 1, 0x00, "Default Bus Master Park Register"}, 
	{"IPR",   0x0040,32, 1, 0, 0x00000000, "Interrupt Pending Register"}, 
	{"IMR",   0x0044,32, 1, 1, 0x0003fffe, "Interrupt Mask Register"}, 
	{"AVR",   0x004b, 8, 1, 1, 0x00, "Autovector Register"}, 
	{"ICR0",  0x004c, 8, 1, 1, 0x00, "Software Watchdog Timer ICR"}, 
	{"ICR1",  0x004d, 8, 1, 1, 0x00, "Timer0 ICR"}, 
	{"ICR2",  0x004e, 8, 1, 1, 0x00, "Timer1 ICR"}, 
	{"ICR3",  0x004f, 8, 1, 1, 0x00, "i2c ICR"}, 
	{"ICR4",  0x0050, 8, 1, 1, 0x00, "Uart0 ICR"}, 
	{"ICR5",  0x0051, 8, 1, 1, 0x00, "Uart1 ICR"}, 
	{"ICR6",  0x0052, 8, 1, 1, 0x00, "DMA0 ICR"}, 
	{"ICR7",  0x0053, 8, 1, 1, 0x00, "DMA1 ICR"}, 
	{"ICR8",  0x0054, 8, 1, 1, 0x00, "DMA2 ICR"}, 
	{"ICR9",  0x0055, 8, 1, 1, 0x00, "DMA3 ICR"}, 
  
	{ NULL, 0, 0, 0, 0 , 0, NULL }};

static int sim_reg_count = 0; /* Will be filled in inside Init() */
static struct _memory_segment *sim_memory_segment = NULL;

/* Interrupts go from 0 -> 9 on the 5307, with the 7 external
 *  autovector only interrupts defined in the autovector interrupt */

/* ICR:
 *	+----+----+----+----+----+----+----+----+
 * 	| av | xx | xx |  int level   | int pri |
 *	+----+----+----+----+----+----+----+----+ 
 * av -- autovectored, 1=yes 
 * int level -- interrupt level, 0 -> 7
 * int pri -- interrupt priority 11(high) -> 00 (low) */
#define ICR_LEVEL(icr) 	(((icr) & 0x1c) >> 2)
#define ICR_PRI(icr) 	((icr) & 0x02)
#define ICR_AVEC(icr)	((icr) & 0x80)


static int sim_register_offset_compare(const void *a, const void *b)
{
	/* ascending order */
	return ( ((struct _sim_register *)a)->offset - 
			((struct _sim_register *)b)->offset  );
}

static struct _sim_register *sim_register_lookup_by_offset(s32 offset)
{
	struct _sim_register key;

	/* Setup search key */
	key.offset = offset;

	return bsearch(&key, &sim_reg, sim_reg_count, 
		sizeof(struct _sim_register), &sim_register_offset_compare);
}

static struct _sim_register *sim_register_lookup_by_name(char *name)
{
	int x;
	for(x=0;x<sim_reg_count; x++) {
		if(strcasecmp(sim_reg[x].name, name) == 0)
			return &sim_reg[x];
	}
	return NULL;
}

static void sim_setup(struct _memory_segment *s);
static void sim_fini(struct _memory_segment *s);
static char sim_read(struct _memory_segment *s, u32 *result, s16 size, u32 offset);
static char sim_write(struct _memory_segment *s, s16 size, u32 offset, u32 value);
static void sim_reset(struct _memory_segment *s);


void sim_5307_init(void)
{
	memory_module_register("sim_5307", &sim_setup);
}

static void sim_setup(struct _memory_segment *s)
{
	sim_data.interrupt_assert = &sim_interrupt_assert;
	sim_data.interrupt_withdraw = &sim_interrupt_withdraw;
	sim_data.register_lookup_by_offset = &sim_register_lookup_by_offset;
	sim_data.register_lookup_by_name = &sim_register_lookup_by_name;
	sim_register(&sim_data);
	
	/* Count the registers */
	sim_reg_count = 0;
	while(1) {
		if(sim_reg[sim_reg_count].name == NULL) break;
		sim_reg_count++;
	}
	/* Sort 'em */
	qsort(&sim_reg, sim_reg_count, sizeof(struct _sim_register),
		&sim_register_offset_compare);


	/* Register the memory region */
	if(s->mask && s->mask != 0xFFFFFF00) {
		printf("warning: sim length must be 0xFF, not 0x%08x. reset.\n", 
				~s->mask);
	}
	s->mask = 0xFFFFFF00;
	s->fini = &sim_fini;
	s->read = &sim_read;
	s->write = &sim_write;
	s->reset = &sim_reset;
	s->update = NULL;
	s->data = malloc(sizeof(struct _sim_5307));
	memset(s->data, 0, sizeof(struct _sim_5307));

	if(sim_memory_segment) {
		printf("A SIM has already been defined!\n");
		printf("There is probably 2 SIMs defined in the board config.\n");
		printf("Aborting.\n");
		exit(1);
	}
	sim_memory_segment = s;
}

static void sim_fini(struct _memory_segment *s)
{
	free(s->data);
	free(s->name);
}

static void sim_reset(struct _memory_segment *s)
{
	int x;
	for(x=0; x<sim_reg_count; x++) {
		/* We could do a writable check here, 'cept then the
		 * register which are read only won't get initialized */
		if(!sim_reg[x].write) continue;
		
		Memory_Stor(sim_reg[x].width, 
				memory_core.mbar + sim_reg[x].offset,
				sim_reg[x].resetvalue);
	}
}

static char sim_read(struct _memory_segment *s, u32 *result, 
					s16 size, u32 offset)
{
	struct _sim_register *reg;
	struct _sim_5307 *sd = (struct _sim_5307 *)s->data;
	TRACE("size=%d, offset=0x%08lx\n", size, offset);

	reg = sim_register_lookup_by_offset(offset);
	if(!reg) {
		ERR("Unaligned SIM memory access, offset=0x%08lx\n", offset);
		return 0;
	}

	if(!reg->read) {
		/* Register is not read-able */
		/* FIXME: do something intelligent here :) */
	}

	TRACE("reg=%s, offset=0x%lx, width=%d\n", reg->name, reg->offset, reg->width);

	if(reg->width == 32)
		*result = *(u32 *)((u8 *)sd + offset);
	else if(reg->width == 16)
		*result = *(u16 *)((u8 *)sd + offset);
	else
		*result = *(u8 *)((u8 *)sd + offset);

	TRACE("Returning with result=0x%08lx\n", *result);
	return 1;
}


static char sim_write(struct _memory_segment *s, s16 size, 
			u32 offset, u32 value)
{
/*	register s32 *ptr = (u32 *)0xdeadbeef;*/
	struct _sim_register *reg;
	struct _sim_5307 *sd = (struct _sim_5307 *)s->data;
	TRACE("size=%d, offset=0x%08lx, value=0x%08lx\n", size, offset, value);

	reg = sim_register_lookup_by_offset(offset);
	if(!reg) {
		ERR("Unaligned SIM memory access, offset=0x%08lx\n", offset);
		return 0;
	}
	/* Find a register we can write in, if not this one, maybe the 
	 * next one as s32 as the offset is the same */
	if(!reg->write) {
		if( ((reg+1)->write) && ((reg+1)->offset == offset) ) reg++;
	}
	TRACE("reg=%s, offset=0x%lx, width=%d\n", reg->name, reg->offset, reg->width);

	if(!reg->write) {
		
		/* Register is not write-able */
		/* FIXME: do something intelligent here :) */
		ERR("Attempt to write to read-only register %s(%s)\n", 
			reg->name, reg->description);
	}
	if(size != reg->width) {
		ERR("Warning: %d bit write to %d bit register %s(%s)\n",
			size, reg->width, reg->name, reg->description);
	}
		

	if(reg->width == 32)
		*(u32 *)((u8 *)sd + offset) = value;
	else if(reg->width == 16)
		*(u16 *)((u8 *)sd + offset) = value;
	else
		*(u8 *)((u8 *)sd + offset) = value;

	return 1;
	
}

/* We use this to store the vector of an interrupt we are posting.
 * if the interrupt is not autovectored, we will jump to whatever one
 * of the 256 interrupts is specified in here.  In here [0] will be
 * unused.  Interrupts start at 1.  BUT in the ICR, that starts at [0],
 * so we;ll need to remember to sub 1 from any ICR access */
static unsigned char interrupt_acknowledge[32];
static int interrupts_at_level[8] = { 0,0,0,0,0,0,0,0 };

/* This is given to the core when there's an interrupt, this figures
 * out the vector of the interrupt for the core */
static u32 sim_iack_func(u32 interrupt_level)
{
	struct _sim_5307 *s = (struct _sim_5307 *)sim_memory_segment->data;
	u32 vector=0;
	int x;
	s16 mask = 0x1;

	TRACE("called for interrupt_level=%d\n", interrupt_level);
	/* Find the _pending_ interrupt with level == interrupt_level */
	for(x=1; x<18; x++) {
		int icr_avec, icr_level, icr_pri;
		
		mask <<= 1;
		if(! (s->ipr & mask)) {
			TRACE("sim input number %d is not pending.\n", x);
			continue;
		}

		/* icr_avec will be non-zero if it's autovectored, but
		 *  it will be different for each interrupt level */
		if(x >= 8) {
			icr_avec = ICR_AVEC(s->icr[x-8]);
			icr_level = ICR_LEVEL(s->icr[x-8]);
			icr_pri = ICR_PRI(s->icr[x-8]);
			TRACE(" %d: ICR = 0x%02x (IL=%d,pri=%d,avec=%d)\n", x, 
				s->icr[x-8], icr_level, icr_pri, icr_avec?1:0);
		} else {
			/* 0 - interrupt source returns vector 
			 * 1 - autovector */
			icr_avec = (s->avr & (0x1<<x) );
			icr_level=x;
			icr_pri=0;
			
			TRACE(" %d: external int (lev=%d,pri=%d,avec=%d)\n", x, 
				icr_level, icr_pri, icr_avec?1:0);
			/* FIXME: how the heck do we use the IRQPAR? */
		}
		
		if(icr_level != interrupt_level) continue;
		
		if(icr_avec) {
			TRACE("   This interrupt is autovectored, using autovector.\n");
			TRACE("   vector = 24 + interrupt level(%d) = %d\n",
					icr_level, 24 + icr_level);
			vector = 24 + icr_level; 
		} else {
			TRACE("   Polling the device to get the vector number...\n");
			vector = interrupt_acknowledge[x];
			TRACE("   vector = %d\n", vector);
		}
		return vector;
	}

	ERR("Inside iack_func, but no interrupt is waiting with level %d\n", interrupt_level);
	return 0;
}
	
/* number, 8==SW, 9==timer1, ... */
static void sim_interrupt_assert(s16 number, s16 vector)
{
	s16 mask = (0x1 << number);
	struct _sim_5307 *s = (struct _sim_5307 *)sim_memory_segment->data;
	int icr_level = ICR_LEVEL(s->icr[number-8]);
	
	TRACE("Posting interrupt Number=%d, Vector=%d\n", number, vector);
	/* Post an interrupt */
	if(!(s->imr & mask )) {
		/* this emulates the coldfire interupt ack bus cycle to 
		 * fetch the vector number, this is the vector that 
		 * the device reports to us */
		interrupt_acknowledge[number] = vector;
		
		/* Set us pending */
		if(s->ipr & mask) {
			TRACE("Already pending, not playing with registers further.\n");
			return;
		}
		
		s->ipr |= mask;
		TRACE("Done, IPR is now 0x%04x\n", s->ipr);


		/* Tell the coldfire that we would like an interrupt */
		exception_post(icr_level, &sim_iack_func);

		interrupts_at_level[icr_level]++;
		TRACE("interrupts_at_level[%d] = %d\n", 
				icr_level, interrupts_at_level[icr_level]);
		
		return;
	}
	TRACE("NOT Posted, The interrupt is unmasked in the IMR\n");
	return;
}

static void sim_interrupt_withdraw(s16 number)
{
	s16 mask = (0x1 << number);
	struct _sim_5307 *s = (struct _sim_5307 *)sim_memory_segment->data;
	int icr_level = ICR_LEVEL(s->icr[number-8]);

//	TRACE("Withdrawing interrupt Number=%d, Vector=%d\n", number);
	/* Post an interrupt */
	if(!(s->imr & mask )) {
		TRACE("Withdrawing interrupt Number=%d\n", number);
		interrupt_acknowledge[number] = 0;
		
		if(! (s->ipr & mask) ) {
			/* This interrupt isn't pending, there's no 
			 * need to withdraw it further */
			TRACE("Interrupt wasn't pending, no need to withdraw.\n");
			return;
		}
		/* Set us not pending */
		s->ipr &= ~mask;
		
		/* withdraw from the coldfire too, only if there are 
		 *  no interrupts left pending at that level, there could
		 *  be multiple interrupts at a single level */
		interrupts_at_level[icr_level]--;
		TRACE("interrupts_at_level[%d] = %d\n", 
				icr_level, interrupts_at_level[icr_level]);
		if(interrupts_at_level[icr_level] == 0) {
			TRACE("calling exception routines to remove interrupt\n");
			exception_withdraw(icr_level);
		}
		TRACE("Done.\n");
		return;
	}
//	TRACE("NOT Withdrawn, the interrupt is unmasked in the IMR\n");
	return;
}

