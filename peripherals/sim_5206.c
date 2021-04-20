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

#include "sim_5206.h"

TRACER_DEFAULT_CHANNEL(sim);


static struct _sim_register *sim_register_lookup_by_offset(s32 offset);
static struct _sim_register *sim_register_lookup_by_name(char *name);
static void sim_interrupt_assert(s16 number, s16 vector);
static void sim_interrupt_withdraw(s16 number);
struct _sim sim_data;

/* name, offset, width, read, write, resetvalue, description */
static struct _sim_register sim_reg[] = {
	{"SIMR",  0x0003, 8, 1, 1, 0xC0, "SIM Configuration Register"}, 
 
	{"ICR1",  0x0014, 8, 1, 1, 0x04, "Interrupt Control Register Ext1"}, 
	{"ICR2",  0x0015, 8, 1, 1, 0x08, "Interrupt Control Register Ext2"}, 
	{"ICR3",  0x0016, 8, 1, 1, 0x0C, "Interrupt Control Register Ext3"}, 
	{"ICR4",  0x0017, 8, 1, 1, 0x10, "Interrupt Control Register Ext4"}, 
	{"ICR5",  0x0018, 8, 1, 1, 0x14, "Interrupt Control Register Ext5"}, 
	{"ICR6",  0x0019, 8, 1, 1, 0x18, "Interrupt Control Register Ext6"}, 
	{"ICR7",  0x001A, 8, 1, 1, 0x1C, "Interrupt Control Register Ext7"}, 
	{"ICR8",  0x001B, 8, 1, 1, 0x1C, "Interrupt Control Register Ext8"}, 
	{"ICR9",  0x001C, 8, 1, 1, 0x80, "Interrupt Control Register Ext9"}, 
	{"ICR10", 0x001D, 8, 1, 1, 0x80, "Interrupt Control Register Ext10"}, 
	{"ICR11", 0x001E, 8, 1, 1, 0x00, "Interrupt Control Register Ext11"}, 
	{"ICR12", 0x001F, 8, 1, 1, 0x00, "Interrupt Control Register Ext12"}, 
	{"ICR13", 0x0020, 8, 1, 1, 0x00, "Interrupt Control Register Ext13"}, 
	/* Motorola's docs have these next two marked as 32 wide, 
	 *  my docs say they're 16 */
	{"IMR",   0x0036, 16, 1, 1, 0x3FFE, "Interrupt Mask Register"}, 
	{"IPR",   0x003A, 16, 1, 0, 0x0000, "Interrupt Pending Register"}, 
	/* Reset Status Register reset == 0x80 or 0x20 
	 *  I guess we pick one at ramdom ?? */
	{"RSR",   0x0040, 8, 1, 1, 0x80, "Reset Status Register"}, 
	{"SYPCR", 0x0041, 8, 1, 1, 0x00, "System Protection Control Register"}, 
	{"SWIVR", 0x0042, 8, 0, 1, 0x0F, "Software Watchdog Interrupt Vector Register"}, 
	{"SWSR",  0x0043, 8, 0, 1, 0xde /* Unitialized */, "Software Watchdog Service Register"}, 
 
/* FIXME: I'm down to here with filling in reset values, noticing 
 *  differences, and changing the register names to be consistant with the
 *  docs motorola give out */


	/* General Purpose I/O */
 
	{"PAR",   0x00CB, 8, 1, 1, 0x00000000, "Pin Assignment Register"}, 
	{"PPDDR", 0x01C5, 8, 1, 1, 0x00000000, "Port A Data Direction Register"}, 
	{"PPDAT", 0x01C9, 8, 1, 1, 0x00000000, "Port A Data Register"}, 
 
	/* Chip Select Registers  */
 
	{"CSAR0", 0x0064, 16, 1, 1, 0x00000000, "Chip-Select 0 Base Address Register"}, 
	{"CSMR0", 0x0068, 32, 1, 1, 0x00000000, "Chip-Select 0 Address Mask Register"}, 
	{"CSCR0", 0x006E, 16, 1, 1, 0x00000000, "Chip-Select 0 Control Register"}, 
	{"CSAR1", 0x0070, 16, 1, 1, 0x00000000, "Chip-Select 1 Base Address Register"}, 
	{"CSMR1", 0x0074, 32, 1, 1, 0x00000000, "Chip-Select 1 Address Mask Register"}, 
	{"CSCR1", 0x007A, 16, 1, 1, 0x00000000, "Chip-Select 1 Control Register"}, 
	{"CSAR2", 0x007C, 16, 1, 1, 0x00000000, "Chip-Select 2 Base Address Register"}, 
	{"CSMR2", 0x0080, 32, 1, 1, 0x00000000, "Chip-Select 2 Address Mask Register"}, 
	{"CSCR2", 0x0086, 16, 1, 1, 0x00000000, "Chip-Select 2 Control Register"}, 
	{"CSAR3", 0x0088, 16, 1, 1, 0x00000000, "Chip-Select 3 Base Address Register"}, 
	{"CSMR3", 0x008C, 32, 1, 1, 0x00000000, "Chip-Select 3 Address Mask Register"}, 
	{"CSCR3", 0x0092, 16, 1, 1, 0x00000000, "Chip-Select 3 Control Register"}, 
	{"CSAR4", 0x0094, 16, 1, 1, 0x00000000, "Chip-Select 4 Base Address Register"}, 
	{"CSMR4", 0x0098, 32, 1, 1, 0x00000000, "Chip-Select 4 Address Mask Register"}, 
	{"CSCR4", 0x009E, 16, 1, 1, 0x00000000, "Chip-Select 4 Control Register"}, 
	{"CSAR5", 0x00A0, 16, 1, 1, 0x00000000, "Chip-Select 5 Base Address Register"}, 
	{"CSMR5", 0x00A4, 32, 1, 1, 0x00000000, "Chip-Select 5 Address Mask Register"}, 
	{"CSCR5", 0x00AA, 16, 1, 1, 0x00000000, "Chip-Select 5 Control Register"}, 
	{"CSAR6", 0x00AC, 16, 1, 1, 0x00000000, "Chip-Select 6 Base Address Register"}, 
	{"CSMR6", 0x00B0, 32, 1, 1, 0x00000000, "Chip-Select 6 Address Mask Register"}, 
	{"CSCR6", 0x00B6, 16, 1, 1, 0x00000000, "Chip-Select 6 Control Register"}, 
	{"CSAR7", 0x00B8, 16, 1, 1, 0x00000000, "Chip-Select 7 Base Address Register"}, 
	{"CSMR7", 0x00BC, 32, 1, 1, 0x00000000, "Chip-Select 7 Address Mask Register"}, 
	{"CSCR7", 0x00C2, 16, 1, 1, 0x00000000, "Chip-Select 7 Control Register"}, 
	{"DMCR", 0x00C6, 16, 1, 1, 0x00000000, "Default Memory Control Register"}, 
 
	/* DRAM Controller Registers */
 
	{"DCRR",  0x0046, 16, 1, 1, 0x00000000, "DRAM Controller Refresh Register"}, 
	{"DCTR",  0x004A, 16, 1, 1, 0x00000000, "DRAM Controller Timing Register"}, 
	{"DCAR0", 0x004C, 16, 1, 1, 0x00000000, "DRAM Controller Bank 0 Address Register"}, 
	{"DCMR0", 0x0050, 32, 1, 1, 0x00000000, "DRAM Controller Bank 0 Mask Register"}, 
	{"DCCR0", 0x0057, 8, 1, 1, 0x00000000, "DRAM Controller Bank 0 Control Register"}, 
	{"DCAR1", 0x0058, 16, 1, 1, 0x00000000, "DRAM Controller Bank 1 Address Register"}, 
	{"DCMR1", 0x005C, 32, 1, 1, 0x00000000, "DRAM Controller Bank 1 Mask Register"}, 
	{"DCCR1", 0x0063, 8, 1, 1, 0x00000000, "DRAM Controller Bank 1 Control Register"}, 
 
	/* Timer Registers */
 
	/* Timer 1 */
 
	{"TMR1",  0x0100, 16, 1, 1, 0x0000, "TIMER1 Mode Register"}, 
	{"TRR1",  0x0104, 16, 1, 1, 0xFFFF, "TIMER1 Reference Register"}, 
	{"TCR1",  0x0108, 16, 1, 0, 0x0000, "TIMER1 Capture Register"}, 
	{"TCN1",  0x010C, 16, 1, 1, 0x0000, "TIMER1 Counter"}, 
	{"TER1",  0x0111,  8, 1, 1, 0x00,   "TIMER1 Event Register"}, 
 
	/* Timer 2 */
 
	{"TMR2",  0x0120, 16, 1, 1, 0x0000, "TIMER2 Mode Register"}, 
	{"TRR2",  0x0124, 16, 1, 1, 0xFFFF, "TIMER2 Reference Register"}, 
	{"TCR2",  0x0128, 16, 1, 0, 0x0000, "TIMER2 Capture Register"}, 
	{"TCN2",  0x012C, 16, 1, 1, 0x0000, "TIMER2 Counter"}, 
	{"TER2",  0x0131,  8, 1, 1, 0x00,   "TIMER2 Event Register"}, 
 
	/* Serial Module Registers */
 
	/* UART1 */
 
	{"UMR1",  0x0140, 8, 1, 1, 0x00000000, "UART1 Mode Register"}, 
	{"USR1",  0x0144, 8, 1, 0, 0x00000000, "UART1 Status Register"}, 
	{"UCSR1", 0x0144, 8, 0, 1, 0x00000000, "UART1 Clock Select Register"}, 
	{"UCR1",  0x0148, 8, 0, 1, 0x00000000, "UART1 Command Register"}, 
	{"URBUF1",0x014C, 8, 1, 0, 0x00000000, "UART1 Receiver Buffer"}, 
	{"UTBUF1",0x014C, 8, 0, 1, 0x00000000, "UART1 Transmitter Buffer"}, 
	{"UISR1", 0x0150, 8, 1, 0, 0x00000000, "UART1 Input Port Change Register"}, 
	{"UACR1", 0x0150, 8, 0, 1, 0x00000000, "UART1 Auxiliary Control Register"}, 
	{"UIR1",  0x0154, 8, 1, 0, 0x00000000, "UART1 Interrupt Status Register"}, 
	{"UIMR1", 0x0154, 8, 0, 1, 0x00000000, "UART1 Interrupt Mask Register"}, 
	{"UBG11", 0x0158, 8, 0, 1, 0x00000000, "UART1 Baud Rate Generator PreScale MSB"}, 
	{"UBG21", 0x015C, 8, 0, 1, 0x00000000, "UART1 Baud Rate Generator PreScale LSB"}, 
	{"UIVR1", 0x0170, 8, 1, 1, 0x00000000, "UART1 Interrupt Vector Register"}, 
	{"UIP1",  0x0174, 8, 1, 0, 0x00000000, "UART1 Input Port Register"}, 
	{"UOP11", 0x0178, 8, 0, 1, 0x00000000, "UART1 Output Port Bit Set Command"}, 
	{"UOP01", 0x017C, 8, 0, 1, 0x00000000, "UART1 Output Port Bit Reset Command"}, 
 
	/* UART2 */
 
	{"UMR2",  0x0180, 8, 1, 1, 0x00000000, "UART2 Mode Register"}, 
	{"USR2",  0x0184, 8, 1, 0, 0x00000000, "UART2 Status Register"}, 
	{"UCSR2", 0x0184, 8, 0, 1, 0x00000000, "UART2 Clock Select Register"}, 
	{"UCR2",  0x0188, 8, 0, 1, 0x00000000, "UART2 Command Register"}, 
	{"URBUF2",0x018C, 8, 1, 0, 0x00000000, "UART2 Receiver Buffer"}, 
	{"UTBUF2",0x018C, 8, 0, 1, 0x00000000, "UART2 Transmitter Buffer"}, 
	{"UISR2", 0x0190, 8, 1, 0, 0x00000000, "UART2 Input Port Change Register"}, 
	{"UACR2", 0x0190, 8, 0, 1, 0x00000000, "UART2 Auxiliary Control Register"}, 
	{"UIR2",  0x0194, 8, 1, 0, 0x00000000, "UART2 Interrupt Status Register"}, 
	{"UIMR2", 0x0194, 8, 0, 1, 0x00000000, "UART2 Interrupt Mask Register"}, 
	{"UBG12", 0x0198, 8, 0, 1, 0x00000000, "UART1 Baud Rate Generator PreScale MSB"}, 
	{"UBG22", 0x019C, 8, 0, 1, 0x00000000, "UART2 Baud Rate Generator PreScale LSB"}, 
	{"UIVR2", 0x01B0, 8, 1, 1, 0x00000000, "UART2 Interrupt Vector Register"}, 
	{"UIP2",  0x01B4, 8, 1, 0, 0x00000000, "UART2 Input Port Register"}, 
	{"UOP12", 0x01B8, 8, 0, 1, 0x00000000, "UART2 Output Port Bit Set Command"}, 
	{"UOP02", 0x01BC, 8, 0, 1, 0x00000000, "UART2 Output Port Bit Reset Command"}, 
 
	/* M-BUS Registers */
 
	{"MADR",  0x01E0, 8, 1, 1, 0x00000000, "M-BUS Address Register"}, 
	{"MFDR",  0x01E4, 8, 1, 1, 0x00000000, "M-BUS Frequency Divider Register"}, 
	{"MBCR",  0x01E8, 8, 1, 1, 0x00000000, "M-BUS Control Register"}, 
	{"MBSR",  0x01EC, 8, 1, 1, 0x00000000, "M-BUS Status Register"}, 
	{"MBDR",  0x01F0, 8, 1, 1, 0x00000000, "M-BUS Data I/O Register"}, 
  
	{ NULL, 0, 0, 0, 0 , 0, NULL }};

static int sim_reg_count = 0; /* Will be filled in inside Init() */
static struct _memory_segment *sim_memory_segment = NULL;

/* Interrupts go from 1 -> 13 on the 5206, and 
 * 	1 -> 15 on the 5206e (unimplemented) */

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


void sim_5206_init(void)
{
	memory_module_register("sim_5206", &sim_setup);
}

static void sim_setup(struct _memory_segment *s)
{
	TRACE("set for sim_5206\n");
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
	if(s->mask && s->mask != 0xFFFFFC00) {
		printf("warning: sim length must be 0x3FF, not 0x%08x. reset.\n", 
				~s->mask);
	}
	s->mask = 0xFFFFFC00;
	s->fini = &sim_fini;
	s->read = &sim_read;
	s->write = &sim_write;
	s->reset = &sim_reset;
	s->update = NULL;
	s->data = malloc(sizeof(struct _sim_5206));
	memset(s->data, 0, sizeof(struct _sim_5206));
	TRACE("sim alloced with %d bytes at %p\n", sizeof(struct _sim_5206),
			s->data);

	if(sim_memory_segment) {
		printf("A SIM has already been defined!\n");
		printf("There is probably 2 SIMs defined in the board config.\n");
		printf("Aborting.\n");
		exit(1);
	}
	TRACE("setting sim_memory_segment to %p, s->data=%p\n", s, s->data);
	sim_memory_segment = s;
}

static void sim_fini(struct _memory_segment *s)
{
	TRACE("starting.\n");
	free(s->data);
	free(s->name);
	TRACE("done.\n");
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
	struct _sim_5206 *sd = (struct _sim_5206 *)s->data;
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
	struct _sim_5206 *sd = (struct _sim_5206 *)s->data;
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
static unsigned char interrupt_acknowledge[16];
static int interrupts_at_level[8] = { 0,0,0,0,0,0,0,0 };

/* This is given to the core when there's an interrupt, this figures
 * out the vector of the interrupt for the core */
static u32 sim_iack_func(u32 interrupt_level)
{
	struct _sim_5206 *s = (struct _sim_5206 *)sim_memory_segment->data;
	u32 vector=0;
	int x;
	s16 mask = 0x1;

	TRACE("called for interrupt_level=%d\n", interrupt_level);
	/* Find the _pending_ interrupt with level == interrupt_level */
	for(x=1; x<14; x++) {
		int icr_avec, icr_level, icr_pri;
		
		mask <<= 1;
		if(! (s->IPR & mask)) {
			TRACE("sim input number %d is not pending.\n", x);
			continue;
		}

		icr_avec = ICR_AVEC(s->ICR[x-1]);
		icr_level = ICR_LEVEL(s->ICR[x-1]);
		icr_pri = ICR_PRI(s->ICR[x-1]);
		TRACE(" %d: ICR = 0x%02x (IL=%d,pri=%d,avec=%d)\n", x, 
				s->ICR[x-1], icr_level, icr_pri, icr_avec?1:0);
		
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
		break;
	}
	if(x==14) {
		ERR("Inside iack_func, but no interrupt is waiting with level %d\n", interrupt_level);
		return 0;
	}

	return vector;
}
	
static void sim_interrupt_assert(s16 number, s16 vector)
{
	s16 mask = (0x1 << number);
	struct _sim_5206 *s = (struct _sim_5206 *)sim_memory_segment->data;
	int icr_level = ICR_LEVEL(s->ICR[number-1]);
	
	TRACE("Posting interrupt Number=%d, Vector=%d\n", number, vector);
	/* Post an interrupt */
	if(!(s->IMR & mask )) {
		/* this emulates the coldfire interupt ack bus cycle to 
		 * fetch the vector number, this is the vector that 
		 * the device reports to us */
		interrupt_acknowledge[number] = vector;
		
		/* Set us pending */
		if(s->IPR & mask) {
			TRACE("Already pending, not playing with registers further.\n");
			return;
		}
		
		s->IPR |= mask;
		TRACE("Done, IPR is now 0x%04x\n", s->IPR);


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
	struct _sim_5206 *s = (struct _sim_5206 *)sim_memory_segment->data;
	int icr_level = ICR_LEVEL(s->ICR[number-1]);

//	TRACE("Withdrawing interrupt Number=%d, Vector=%d\n", number);
	/* Post an interrupt */
	if(!(s->IMR & mask )) {
		TRACE("Withdrawing interrupt Number=%d\n", number);
		interrupt_acknowledge[number] = 0;
		
		if(! (s->IPR & mask) ) {
			/* This interrupt isn't pending, there's no 
			 * need to withdraw it further */
			TRACE("Interrupt wasn't pending, no need to withdraw.\n");
			return;
		}
		/* Set us not pending */
		s->IPR &= ~mask;
		
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

