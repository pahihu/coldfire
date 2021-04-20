/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Word Unsigned multiply (MUL.W) instruction */
/* Word Unsigned multiply (MULS.W) instruction */

/* Format for MUL.W
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 1 | 0 | 0 | Register  | 0 | 1 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

Format for MULS.W
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 1 | 0 | 0 | Register  |U/S| 1 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int MULWTime[8]={9, 11, 11, 11, 11, 12, 11, 9};

TRACER_DEFAULT_CHANNEL(i_mul_w);

INSTRUCTION_6ARGS(MUL_W,
	unsigned Code2,4,
	unsigned Register,3,
	unsigned US,1,
	unsigned Code1,2,
	unsigned EAMode,3,
	unsigned EARegister,3);

static void execute(void) 
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	MUL_W_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(Instr.Bits.EAMode==1) {
		ERR("May Not specify Address Register (Ay) for MUL.W");
		return;
	}

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 16, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	EA_GetValue(&SValue, &Source);
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	EA_GetValue(&DValue, &Destination);

	if(Instr.Bits.US == 0) { /* Unsigned */
		Result = (u16)(SValue&0x0000FFFF) * (u16)(DValue&0x0000FFFF);
	} else {
		Result = (s16)(SValue&0x0000FFFF) * (s16)(DValue&0x0000FFFF);
	}

	TRACE("0x%04lx * 0x%04lx = 0x%04lx\n", (SValue&0x0000FFFF), (DValue&0x0000FFFF), Result);

	/* Set the status register */
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);
	TRACE("Done\n");
	
	cycle(MULWTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	MUL_W_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	if(Instr.Bits.US == 0) /* Unsigned */
		sprintf(Instruction, "MULU.W");
	else
		sprintf(Instruction, "MULS.W");
	Addressing_Print(16, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
	Addressing_Print(32, 0, Instr.Bits.Register, Arg2);
	return 0;
}

s32 mul_w_5206_register(void)
{
	instruction_register(0xC0C0, 0xF0C0, &execute, &disassemble);
	return 2;
}
