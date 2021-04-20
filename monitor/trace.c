/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*  & Matt Minnis                 */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);


/* For counting the number of instructions left to trace */
s32 Trace_InstructionCount=0;

int Monitor_TRACE(int argc, char **argv)
{
	u32 frame;

	if(argc > 1)
		sscanf(argv[1], "%x", &Trace_InstructionCount);
	else
		Trace_InstructionCount = 1;

	/* Get the to-be-restored stack pointer, and set the T bit */
	
	Memory_Retr(&frame, 32, memory_core.a[7]);
	frame |= 0x00008000;
	Memory_Stor(32,memory_core.a[7],frame);
	
	/* Trace on */
	memory_core.sr.t = SR_T;
	/* Continue execution */
	return 0;
}

void Monitor_TRACE_Entry(s16 Vector, char *enter_monitor, char *dump_info)
{
	u32 frame;
	*dump_info=1;
	Trace_InstructionCount--;
	if(Trace_InstructionCount == 0) {
		memory_core.sr.t = 0; /* Stop tracing when the count == 0 */

		Memory_Retr(&frame, 32, memory_core.a[7]);
		frame &= ~0x00008000;
		Memory_Stor(32,memory_core.a[7],frame);

		*enter_monitor = 1;
	} else {
		*enter_monitor = 0;
	}
}

