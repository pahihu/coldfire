/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Subtract instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 0 | 1 | Register  | 1 | 1 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int SUBATime[]={-1, 3, 3, 3, 3, 4, 3, -1 };

TRACER_DEFAULT_CHANNEL(i_suba);

INSTRUCTION_5ARGS(SUBA,
	unsigned Code2,4,
	unsigned Register,3,
	unsigned Code1,3,
	unsigned EAMode,3,
	unsigned EARegister,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	SUBA_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 1, Instr.Bits.Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	Result = DValue - SValue;

	TRACE("0x%08lx - 0x%08lx = 0x%08lx\n", DValue, SValue, Result);

        /* Condition codes are not affected */

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	TRACE("Done\n");

	cycle(SUBATime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);

	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	SUBA_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "SUBA.L");
	Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
	Addressing_Print(32, 1, Instr.Bits.Register, Arg2);
	return 0;
}

s32 suba_5206_register(void)
{
	instruction_register(0x91C0, 0xF1C0, &execute, &disassemble);
	return 1;
}
