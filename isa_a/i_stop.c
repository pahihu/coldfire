/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/* FIXME: Unverified correct operation */
#include "coldfire.h"

/* Stop (STOP) instruction */
/* MHM July 13, 2000 */
/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 0 | 1 | 0 |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int STOPTime=3;

TRACER_DEFAULT_CHANNEL(i_stop);

INSTRUCTION_1ARG(STOP,
	unsigned Code1,16);

static void execute(void)
{
	struct _Address Source;
	u32 Result, SValue;
	STOP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 16, 7, 4)) return;
	EA_GetValue(&SValue, &Source);

	Result = SValue;

	TRACE("#0x%08lx\n", Result);

	TRACE("Storing Result:\n");

	/* Set the status register */
	SET_SR(Result);

	TRACE("Done\n");
	
	cycle(STOPTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	STOP_Instr Instr;
	u32 SValue;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "STOP");
	Memory_RetrWordFromPC(&SValue);
	sprintf(Arg1, "#$%08x", SValue);
	Arg2[0]=0;
	/* Addressing_Print(32, 0, Instr.Bits.Register, Arg2); */
	return 0;
}

s32 stop_5206_register(void)
{
	instruction_register(0x4E72, 0xFFFF, &execute, &disassemble);
	return 1;
}
