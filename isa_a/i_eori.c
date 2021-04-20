/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* EORI Immediate (EORI) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 0 | 0 | 0 | 1 | 0 | 1 | 0 | 1 | 0 | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int EORITime=1;

TRACER_DEFAULT_CHANNEL(i_eori);

INSTRUCTION_2ARGS(EORI,
	unsigned Code1,13,
	unsigned Register,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	EORI_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, 7, 4)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	Result = SValue ^ DValue;

	TRACE("0x%08lx ^ 0x%08lx = 0x%08lx\n", SValue, DValue, Result);

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	/* Set the status register
	 *  X - not affected
	 *  N - set if MSB or result is 1
	 *  Z - set if result is zero
	 *  V,C always cleared
	 */
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;
	TRACE("Done");
				 
	cycle(EORITime);
					 
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	EORI_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "EORI.L");
	Addressing_Print(32, 7, 4, Arg1);
	Addressing_Print(32, 0, Instr.Bits.Register, Arg2);
	return 0;
}

s32 eori_5206_register(void)
{
	instruction_register(0x0A80, 0xFFF8, &execute, &disassemble);
	return 1;
}
