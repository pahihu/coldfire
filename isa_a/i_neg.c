/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Negate instruction (NEG) */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 0 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int NEGTime=1;

TRACER_DEFAULT_CHANNEL(i_neg);

INSTRUCTION_2ARGS(NEG,
	unsigned Code1,13,
	unsigned Register,3);

static void execute(void)
{
	struct _Address Destination;
	u32 Result, DValue;
	NEG_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	EA_GetValue(&DValue, &Destination);

	Result = 0 - DValue;

	TRACE("~0x%08lx = 0x%08lx\n", DValue, Result);

	/* Set the status register */
	SR_Set(I_NEG, 0, DValue, Result);

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	TRACE("Done\n");

	cycle(NEGTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	NEG_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "NEG.L");
	Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
	Arg2[0]=0;
 	return 0;
}

s32 neg_5206_register(void)
{
	instruction_register(0x4480, 0xFFF8, &execute, &disassemble);
	return 1;
}
