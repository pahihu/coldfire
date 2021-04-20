/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* NOT instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 0   1   1   0   1   0   0   0   0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int NOTTime=1;

TRACER_DEFAULT_CHANNEL(i_not);

INSTRUCTION_2ARGS(NOT,
	unsigned Code1,13,
	unsigned Register,3);

static void execute(void)
{
	struct _Address Destination;
	u32 Result, DValue;
	NOT_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	EA_GetValue(&DValue, &Destination);

	Result = ~DValue;
	TRACE("~0x%08lx = 0x%08lx\n", DValue, Result);

	/* Set the status register */
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	TRACE("Done\n");
	
	cycle(NOTTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	NOT_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "NOT.L");
	Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
	Arg2[0]=0;
 	return 0;

}

s32 not_5206_register(void)
{
	instruction_register(0x4680, 0xFFF8, &execute, &disassemble);
	return 1;
}
