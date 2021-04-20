/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Push Effective Address (pea) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 0 | 1 |  EAMode   | EARegister|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int PEATime[8]={-1, 2, -1, -1, 2, 3, 2, -1};

TRACER_DEFAULT_CHANNEL(i_pea);

INSTRUCTION_3ARGS(PEA,
	unsigned Code1,10,
	unsigned EAMode,3,
	unsigned EARegister,3);

static void execute(void)
{
	struct _Address Source;
	u32 SValue;
	PEA_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	/* Retrive the effective address, not the value that the EA points to */
	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	EA_GetEA(&SValue, &Source);
	TRACE("Pusing 0x%08lx to the stack\n", SValue);
	Stack_Push(32,SValue);

	TRACE("Done\n");

	cycle(PEATime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	PEA_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "PEA");
	Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
	Arg2[0]=0;
	return 0;
}

s32 pea_5206_register(void)
{
	instruction_register(0x4840, 0xFFC0, &execute, &disassemble);
	return 1;
}
