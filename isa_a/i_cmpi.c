/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Compare immediate (CMPI) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 0 | 0 | 0 | 1 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
|               Upper Word of Immediate Data                    |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
|               Lower Word of Immediate Data                    |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int CMPITime=1;

TRACER_DEFAULT_CHANNEL(i_cmpi);

INSTRUCTION_2ARGS(CMPI,
	unsigned Code1,13,
	unsigned Register,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	CMPI_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	
	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, 7, 4)) return;
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;

	EA_GetValue(&SValue, &Source);
	EA_GetValue(&DValue, &Destination);

	Result = DValue - SValue;

	TRACE("Comparing 0x%08lx with 0x%08lx\n", SValue, DValue);

	/* Set the status register */
	SR_Set(I_CMPI, SValue, DValue, Result);

	cycle(CMPITime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	CMPI_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "CMPI.L");
	Addressing_Print(32, 7, 4, Arg1);
	Addressing_Print(32, 0, Instr.Bits.Register, Arg2);
	return 0;
}

s32 cmpi_5206_register(void)
{
	instruction_register(0x0C80, 0xFFF8, &execute, &disassemble);
	return 1;
}
