/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* SubX instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 0 | 1 |Register Dx| 1 | 1 | 0 | 0 | 0 | 0 |Register Dy|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int SUBXTime=1;

TRACER_DEFAULT_CHANNEL(i_subx);

INSTRUCTION_4ARGS(SUBX,
	unsigned Code2,4,
	unsigned RegisterDx,3,
	unsigned Code1,6,
	unsigned RegisterDy,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	SUBX_Instr Instr;

	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, 0, Instr.Bits.RegisterDy)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.RegisterDx)) return;
	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	Result = DValue - SValue - (s32)(memory_core.sr.x ? 1 : 0);

	TRACE("0x%08lx - 0x%08lx - %d = 0x%08lx\n", DValue, SValue, 
				(s32)(memory_core.sr.x ? 1 : 0), Result);

	SR_Set(I_SUBX, SValue, DValue, Result);

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	cycle(SUBXTime);

	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	SUBX_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	sprintf(Instruction, "SUBX.L");
	Addressing_Print(32, 0, Instr.Bits.RegisterDy, Arg1);
	Addressing_Print(32, 0, Instr.Bits.RegisterDx, Arg2);
	return 0;
}

s32 subx_5206_register(void)
{
	instruction_register(0x9180, 0xF1F8, &execute, &disassemble);
	return 1;
}
