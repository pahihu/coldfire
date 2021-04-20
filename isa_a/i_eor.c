/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/* FIXME: Unverified correct operation */
#include "coldfire.h"

/* EOR instruction */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 1 | 1 | Register  |  OPmode   |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int EORTime[8]={1, 3, 3, 3, 3, 4, 3, -1};

TRACER_DEFAULT_CHANNEL(i_eor);

INSTRUCTION_5ARGS(EOR,
	unsigned Code1,4,
	unsigned Register,3,
	unsigned OPMode,3,
	unsigned EAMode,3,
	unsigned EARegister,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	EOR_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(Instr.Bits.OPMode==2) { /* <EA>y ^ Dx */
		TRACE("<EA>y ^ Dx -> Dx\n");
		if(Instr.Bits.EAMode==1) {
			ERR("May not specify Ax for source");
			return;
		}
		TRACE("Source:\n");
		if(!EA_GetFromPC(&Source, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
		TRACE("Destination:\n");
		if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	} else if (Instr.Bits.OPMode==6) { /* Dy & <EA>x -> <EA>x */
/*		
		if(Instr.Bits.EAMode==0) {
			ERR("May not specify Dx for destination when source is Dx");
			return;
		} else if (Instr.Bits.EAMode==1) {
			ERR("May not specify Ax for destination when source is Dx");
			return;
		} else */ if (Instr.Bits.EAMode==7 && Instr.Bits.EARegister==4) {
			ERR("May not specify Immediate Addressing for destination");
			return;
		}
		TRACE("Source:\n");
		if(!EA_GetFromPC(&Source, 32, 0, Instr.Bits.Register)) return;
		TRACE("Destination:\n");
		if(!EA_GetFromPC(&Destination, 32, Instr.Bits.EAMode, Instr.Bits.EARegister)) return;
	} else {
		ERR("Unknown OPMode %d", Instr.Bits.OPMode);
		return;
	}

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

	cycle(EORTime[cycle_EA(Instr.Bits.EARegister,Instr.Bits.EAMode)]);

	TRACE("Done\n");
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	EOR_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "EOR.L");
	if(Instr.Bits.OPMode==2) { /* <EA>y ^ Dx */
		Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg1);
		Addressing_Print(32, 0, Instr.Bits.Register, Arg2);	
	} else {
		Addressing_Print(32, 0, Instr.Bits.Register, Arg1);
		Addressing_Print(32, Instr.Bits.EAMode, Instr.Bits.EARegister, Arg2);	
	}
	return 0;
}

s32 eor_5206_register(void)
{
	instruction_register(0xB180, 0xF1C0, &execute, &disassemble);
	return 1;
}
