/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* DC.W  */
/* Format ... well.. anything that any of the other instructions
   don't handle
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

TRACER_DEFAULT_CHANNEL(i_dc);

INSTRUCTION_1ARG(DC,
	unsigned Code1,16);

static void execute(void)
{
	u32 dummy;
	TRACE("Called\n");
	/* Read the instruction, we already know what it is */
	Memory_RetrWordFromPC(&dummy);
	TRACE("Value=0x%04x, doing exception vector 4\n", dummy);
	/* Do an exception */
	exception_do_exception(4);
	TRACE("Done\n");
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	u32 dummy;
	Memory_RetrWordFromPC(&dummy);
	sprintf(Instruction, "DC.W");
	sprintf(Arg1, "0x%04x", dummy);
	Arg2[0]=0;
	return 0;
}


s32 dc_5206_register(void)
{
	instruction_register(0x0000, 0x0000, &execute, &disassemble);
	return 0;
}
