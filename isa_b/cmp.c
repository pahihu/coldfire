/**********************************/
/*                                */
/*  Copyright 2005, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Compare (CMP) instruction, B and W.  The L format is in ISA_A */

/* Format 

CMP   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 1 | 1 | Register  | 0 | 0 | 0 |  EAMode   |EARegister | == CMP.B
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 1 | 1 | Register  | 0 | 0 | 1 |  EAMode   |EARegister | == CMP.W
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+


*/

static int CMPTime[8]={ 1, 3, 3, 3, 3, 4, 3, 1};

TRACER_DEFAULT_CHANNEL(i_cmp);

INSTRUCTION_5ARGS(CMP,
	unsigned Code2,4,
	unsigned Register,3,
	unsigned size,3,
	unsigned EAMode,3,
	unsigned EARegister,3);

static void execute_b(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	CMP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	
	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 8, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 8, 0, Instr.Bits.Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	TRACE("Comparing (byte) 0x%08x and 0x%08x\n", SValue, DValue);

	Result = (u8)DValue - (u8)SValue;

	SR_Set(I_CMP, SValue, DValue, Result);

	TRACE("Done\n");
	cycle(CMPTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	return;
}
static void execute_w(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	CMP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	
	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 16, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 16, 0, Instr.Bits.Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	TRACE("Comparing (word) 0x%08x and 0x%08x\n", SValue, DValue);

	Result = (u16)DValue - (u16)SValue;

	SR_Set(I_CMP, SValue, DValue, Result);

	TRACE("Done\n");
	cycle(CMPTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);
	return;
}

static const s16 cmp_size[4] = { 8, 16, 32, 0 };
static const char cmp_size_str[4] = { 'B', 'W', 'L', '?' };

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	CMP_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "CMP.%c", cmp_size_str[(u16)Instr.Bits.size]);
	Addressing_Print(cmp_size[(u16)Instr.Bits.size], Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
	Addressing_Print(cmp_size[(u16)Instr.Bits.size], 0, Instr.Bits.Register, Arg2);
	return 0;
}

s32 cmp_isa_b_register(void)
{
	/* Longword cmp is in isa_a */
	instruction_register(0xB000, 0xF1C0, &execute_b, &disassemble);
	instruction_register(0xB040, 0xF1C0, &execute_w, &disassemble);
	return 1;
}
