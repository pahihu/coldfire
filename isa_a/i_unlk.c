/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Unlink (UNLK) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 0 | 1 | 0 | 1 | 1 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int UNLKTime=2;

TRACER_DEFAULT_CHANNEL(i_unlk);

INSTRUCTION_2ARGS(UNLK,
	unsigned Code1,13,
	unsigned Register,3);

static void execute(void)
{
	struct _Address ARegister,Stack;
	u32 Result;
	
	UNLK_Instr Instr;

	Memory_RetrWordFromPC(&Instr.Code);

	if(!EA_GetFromPC(&ARegister, 32, 1, Instr.Bits.Register)) return;
	if(!EA_GetFromPC(&Stack, 32, 1, 7)) return;
	
	/* Load the stack pointer from the A register */
	TRACE("Loading Stack Pointer from A%d:\n", Instr.Bits.Register);
	EA_GetValue(&Result, &ARegister);
	EA_PutValue(&Stack, Result);
	TRACE("New Stack Pointer is 0x%08lx\n", Result);

	/* Now pop a longword from the stack and set that to be the 
            A register */
	TRACE("Popping old Aregister value from the stack\n");
	Result = Stack_Pop(32);
	TRACE("   Value=0x%08lx, storing it...\n", Result);
	EA_PutValue(&ARegister, Result);

	TRACE("Done\n");
	
	cycle(UNLKTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	UNLK_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	sprintf(Instruction, "UNLK");
	Addressing_Print(32, 1, Instr.Bits.Register, Arg1);
	Arg2[0]=0;

	return 0;
}

s32 unlk_5206_register(void)
{
	instruction_register(0x4E58, 0xFFF8, &execute, &disassemble);
	return 1;
}
