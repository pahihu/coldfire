/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Long Unsigned multiply (MULU.L) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 0 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | Register  | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int MULLTime[8]={18, 20, 20, 20, 20, -1, -1, -1};

TRACER_DEFAULT_CHANNEL(i_mulu_l);

INSTRUCTION_3ARGS(MULU_L,
	unsigned Code1,10,
	unsigned EAMode,3,
	unsigned EARegister,3);

static void execute(void) 
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	char Register;
	u32 Instr2;
	MULU_L_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	Memory_RetrWordFromPC(&Instr2);
	Register=(Instr2 & 0x7000) >> 12;


	if(Instr.Bits.EAMode==1) {
		ERR("May Not specify Address Register (Ay) for MULU.L");
		return;
	} else if(Instr.Bits.EAMode==7) {
		ERR("May Not specify Direct Addressing for MULU.L");
		return;
	}
	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	/* FIXME: I'm not sure if this discards the upper 32 bits (as 
		required in the spec) or if it does something FuNkY */
	
	if(Instr2 & 0x0800) {
		/* Signed multiply */
		Result = (s32)(SValue) * (s32)(DValue);
	} else {
		/* Unsigned multiply */
		Result = (u32)(SValue) * (u32)(DValue);
	}

	TRACE("0x%08lx * 0x%08lx = 0x%08lx\n", SValue, DValue, Result);

	/* Set the status register */
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	TRACE("Done\n");
	
	cycle(MULLTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	u32 Instr2;
	char Register;
	MULU_L_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	Memory_RetrWordFromPC(&Instr2);
	Register=(Instr2 & 0x7000) >> 12;

	sprintf(Instruction, "MUL%c.L", (Instr2 & 0x0800) ? 'S' : 'U');
	Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
	Addressing_Print(32, 0, Register, Arg2);
	return 0;
}

s32 mulu_l_5206_register(void)
{
	instruction_register(0x4C00, 0xFFC0, &execute, &disassemble);
	return 2;
}
