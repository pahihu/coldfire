/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/*#define TRACER_OFF*/

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(exception);

static s16 exception_pending = 0;
static u32 (*iack_func[8])(u32 interrupt_level)
	= { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

void exception_check_and_handle(void)
{
	int x;
	if(!exception_pending) return;

	TRACE("exception_pending = 0x%04x\n", exception_pending);

	/* if the mask is 7, do nothing, if the mask is 0, check 
	 * all interrupts 7->1, interrupts level 0 doesn't make sense
	 *  FIXME: can the coldfire fire an interrupt with priority 0 ? */
	
	TRACE("currint interrupt mask is %d, checking 7->%d\n", 
		memory_core.sr.i, memory_core.sr.i+1);
	
	for(x=7; x>=memory_core.sr.i + 1; x--) {
		if(iack_func[x] != NULL) {
			u32 vector;
			TRACE("Found interrupt_level %d to do exception.\n",
					x);
			vector = (iack_func[x])(x);
			TRACE("iack_func returned vector %lu\n", vector);
			exception_push_stack_frame(vector);
			/* Set the new interrupt priority */
			memory_core.sr.i = x;
			
			/* Set the Master/Interrupt bit to 0 */
			memory_core.sr.m = 0;
			TRACE("   Interrupt Priority mask is now %d\n",
						memory_core.sr.i);
			/* Do the RAW exception */
			exception_do_raw_exception(vector);
			return;
		}
	}
}


void exception_post(u32 interrupt_level, 
		u32 (*func)(u32 interrupt_level) )
{
	TRACE("Exception posted at interrupt level %d\n", interrupt_level);
	exception_pending |= (0x1 << interrupt_level);
	iack_func[interrupt_level] = func;
}

void exception_withdraw(u32 interrupt_level)
{
	TRACE("Exception withdrawn at interrupt level %d\n", interrupt_level);
	if(iack_func[interrupt_level] == NULL) {
		ERR("Attempting to withdraw interrupt level %d which is not set.\n",
				interrupt_level);
	}
	exception_pending &= ~(0x1 << interrupt_level);
	iack_func[interrupt_level] = NULL;
}

/* This table is for whether the PC should be set to the next instruction
 * after the fault, or the current instruction, this is only the first 16
 * Vectors.. all others are '0'
 * 
 *  0 = Next address  1 = Fault address */
static char exception_pc_location[16] = {
	0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0 };

void exception_push_stack_frame(s16 vector)
{
	u32 PC_to_push = memory_core.pc;
	u32 stack_frame;

	/* Restore the PC to the fault address */
	TRACE("pushing exception stack frame for exception vector=%d\n", vector);
	if(vector < 16) {
		if(exception_pc_location[vector] == 1)
			PC_to_push = memory_core.pc_instruction_begin;
	}

	/* See if we can read from SP-4 and SP-8 */
	if(!memory_seek(memory_core.a[7]-4) || !memory_seek(memory_core.a[7]-8)) {
		/* Push won't fit */
		printf("pushing to SP=0x%08x for vector %d would cause an error\n", memory_core.a[7], vector);
		/* location of a known good stack pointer:
		 * - 5206, the SP is in the MBAR, 32 bit offset #1
		 * */
		Memory_Retr(&memory_core.a[7], 32, memory_core.vbr);
		printf("SP reset to 0x%08x\n", memory_core.a[7]);
		/* Force return to monitor for debugging.*/
		printf("Forcing Autovector Interrupt 7\n");
		exception_push_stack_frame(31);
		Monitor_HandleException(31);
		exception_restore_from_stack_frame();
	}



	
	/* Stack Frame:
	 * 31      27        25             17       15            0
	 * +--------+---------+-------------+---------+------------+
	 * | Format | FS[3:2] | Vector[7:0] | FS[1:0] |     SR     |
	 * +--------+---------+-------------+---------+------------+
	 * |                 PC                                    |
	 * +-------------------------------------------------------+
	 */

	/* Build the stack frame in a portable fashion */
	stack_frame = 	((0x4 | (memory_core.a[7] & 0x3)) << 28) |	
			(0x0 << 26) |
			((vector & 0xFF) << 18) |
			(0x0 << 16) |
			(GET_SR() & 0xFFFF);
	

	TRACE("Pushing PC [0x%x] and StackFrame [0x%x]\n", PC_to_push,
			 *(s32*)&stack_frame);

	/* Align the stack to the next s32word offset */
/*	FIXME: I'm not convinced that this is correct
	memory_core.a[7] &= 0xfffffffc;*/

	/* Push the PC to the stack */
	Stack_Push(32, PC_to_push);
	/* Push the rest of the stack frame */
	Stack_Push(32, *(s32 *)&stack_frame);
}

void exception_restore_from_stack_frame(void)
{
	s32 frame;
	/* Pop the SR and PC off the stack */
	frame=Stack_Pop(32);
	SET_SR(frame & 0x0000FFFF);
	memory_core.pc=Stack_Pop(32);

	/* Align the stack according to the format */
/*	FIXME: I'm not convinced that this is correct 
	memory_core.a[7] += (frame & 0x30000000 >> 28);*/

	TRACE("Set SR=0x%08x\n", frame & 0x0000FFFF);
	TRACE("Set PC=0x%08x\n", memory_core.pc);
}


s32 exception_do_raw_exception(s16 vector)
{
	u32 offset=0;
	static struct _memory_segment *seg;
	/* Find the jump offset in the vbr */
	TRACE("Doing Exception Vector=%d, vbr=0x%08x\n", vector, 
			memory_core.vbr);
	/* If this falis, we could go into an infinite loop, with an invalid
	 * memory access exception */
	if(!Memory_Retr(&offset, 32, memory_core.vbr + (vector*4))) return 0;
	
	TRACE("ISR is at 0x%08x\n", offset);

	/* Assert the S bit, and clear the T bit */
	memory_core.sr.s = SR_S;
	memory_core.sr.t = 0;

	/* If the offset is in the rom, (the base_register for the
	 *  segment the ISR is in is the address of the rombar)
	 *  then we'll ask the monitor to  handle the exception */
	seg = memory_find_segment_for(offset);

	/* Possible FIXME: if one doesn't use ROMBAR in the config
	 * file, and just specifies the proper address, this won't
	 * ever be true, becaus we'll create a new segment for the same
	 * memory region... */
	if( seg->base_register == &memory_core.rombar) {
		
		/* Handler in rom, somwhere.  Ask monitor to handle the 
		 * exception for us .
		 * Monitor provides an alternative for every exception */
		Monitor_HandleException(vector);
		/* Restore the process for monitor, because it doens't
		 * know how to do it */
		exception_restore_from_stack_frame();
		return 0;
	}
	memory_core.pc=offset;

	TRACE("Set PC to ISR offset [0x%x]\n", memory_core.pc);
	TRACE("Done\n");
	return 0;
}


s32 exception_do_exception(s16 vector)
{
	exception_push_stack_frame(vector);
	return exception_do_raw_exception(vector);
}

