/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Sign Extend (EXT,EXTB) instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 0 | 0 |  OPmode   | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int EXTTime=1;

TRACER_DEFAULT_CHANNEL(i_ext);


INSTRUCTION_4ARGS(EXT,
	unsigned Code2,7,
	unsigned OPMode,3,
	unsigned Code1,3,
	unsigned Register,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 SValue, Result;
	EXT_Instr Instr;

	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(!EA_GetFromPC(&Source, 32, 0, Instr.Bits.Register)) return;
	EA_GetValue(&SValue, &Source);
	
	if(Instr.Bits.OPMode==2) { /* Byte -> Word */
		TRACE("Destination: (Byte -> Word)\n");
		if(!EA_GetFromPC(&Destination, 16, 0, Instr.Bits.Register)) return;
		Result=(char)SValue;
	} else if(Instr.Bits.OPMode==3) { /* Word -> Long */
		TRACE("Destination: (Word -> Long)\n");
		if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
		Result=(s16)SValue;
		EA_PutValue(&Destination, (s16)SValue);
	} else if(Instr.Bits.OPMode==7) { /* Byte -> Long */
		TRACE("Destination: (Byte -> Long)\n");
		if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
		Result=(char)SValue;
	} else {
		ERR("Unknown opmode %d\n", Instr.Bits.OPMode);
		return;
	}

	EA_PutValue(&Destination, Result);
	
	/* Set the status register;
	 *  X - Not affected
	 *  N - Set if result is -ve, cleared otherwise
	 *  Z - Set if result is zero, cleared otherwise
	 *  V - always cleared
	 *  C - always cleared
	 */
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;
	TRACE("Done\n");

	cycle(EXTTime);

	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	EXT_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(Instr.Bits.OPMode==2) { /* Byte -> Word */
		sprintf(Instruction, "EXT.W");
	} else if(Instr.Bits.OPMode==3) { /* Word -> Long */
		sprintf(Instruction, "EXT.L");
	} else if(Instr.Bits.OPMode==7) { /* Byte -> Long */
		sprintf(Instruction, "EXTB.L");
	} else {
		ERR("Unknown opmode\n");
	}

	Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
	Arg2[0]=0;
	return 0;
}

s32 ext_5206_register(void)
{
	instruction_register(0x4800, 0xFE38, &execute, &disassemble);
	instruction_register(0x4800, 0xFE38, &execute, &disassemble);
	return 2;
}
