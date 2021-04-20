/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Logical Shift Left/Right LSL, LSR instructions */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 1 | 1 | 0 | Count/Reg | dr| 1 | 0 |i/r| 0 | 1 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

int LSXTime=1;

TRACER_DEFAULT_CHANNEL(i_lsx);

INSTRUCTION_7ARGS(LSX,
	unsigned Code3,4,
	unsigned CountReg,3,
	unsigned DR,1,
	unsigned Code2,2,
	unsigned IR,1,
	unsigned Code1,2,
	unsigned Register,3);

static void execute(void)
{
	struct _Address Source,Destination;
	u32 Result, SValue, DValue;
	LSX_Instr Instr;
	s32 last_bit;

	Memory_RetrWordFromPC(&Instr.Code);

	TRACE("Source:\n");
	if(Instr.Bits.IR==0) {
		/* Shift from count in instruction word */
		SValue = Instr.Bits.CountReg;
		if(SValue == 0) SValue = 8;
		TRACE("Shift by count in instruction word = %d\n", SValue);
	} else {
		if(!EA_GetFromPC(&Source, 32, 0, Instr.Bits.CountReg)) return;
		/* Get source, modulo 64 */
		EA_GetValue(&SValue, &Source);
		SValue &= 0x0000003F;
		TRACE("Shift by count in D%d = \n", Instr.Bits.CountReg, SValue);
	}
	TRACE("Destination:\n");
	if(!EA_GetFromPC(&Destination, 32, 0, Instr.Bits.Register)) return;
	EA_GetValue(&DValue, &Destination);


	if(SValue == 0) {
		TRACE("No Shift");
		memory_core.sr.c = 0;
		Result = DValue;
	} else {
		last_bit = 0;
		if(Instr.Bits.DR==0) {
			/* Shift Right */
			TRACE("Shift Right\n");
			/* Catch if we are shifting the register clean, this
			 * catchs any funny modulo arithmetic the native
			 * hardware does with a shift */
			if(SValue <= 32) 
				last_bit = DValue & (0x1 << (SValue-1));
			
			/* On x86, the instruction takes modulo 32, so a 
			 * shift by 0x20 actually shifts 0, and 
			 * 0x21 shifts 1, etc. but we want to be able
			 * to shift everything out of the register */
			Result = (SValue >= 32) ? 0 : (DValue >> SValue);
			
		} else {
			/* Shift Left */
			TRACE("Shift Left\n");
			if(SValue <= 32) 
				last_bit = DValue & (0x80000000 >> (SValue-1));
			
			Result = (SValue >= 32) ? 0 : (DValue << SValue);
		}
		memory_core.sr.c = (last_bit) ? SR_C : 0;
		memory_core.sr.x = (last_bit) ? SR_X : 0;
	}
	TRACE("0x%08lx %s 0x%02lx = 0x%08lx\n", DValue, (Instr.Bits.DR==0) ? ">>" : "<<", SValue, Result);

	/* X - Set according to last bit shifted out of
		the operand; unaffected for shift count of 0
	   N - Set if result is -ve, cleared otherwise
	   Z - Set if result is zero, cleared otherwise
	   V - always cleared
	   C - set according to the last bit shifted out 
		of the operand; cleared for a shift count of 0
	*/
	memory_core.sr.n = ((s32)Result < 0) ? SR_N : 0;
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	memory_core.sr.v = 0;
	
	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);

	cycle(LSXTime);
	
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	LSX_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);

	if(Instr.Bits.DR==0) {
		/* Shift Right */
		sprintf(Instruction, "LSR.L");
	} else {
		/* Shift Left */
		sprintf(Instruction, "LSL.L");
	}
	if(Instr.Bits.IR==0) {
		/* Shift from count in instruction word */
		s32 SValue = Instr.Bits.CountReg;
		if(SValue == 0) SValue = 8;
		sprintf(Arg1, "#0x%02X", SValue);
	} else {
		sprintf(Arg1, "D%d", Instr.Bits.CountReg);
	}
	Addressing_Print(32, 0, Instr.Bits.Register, Arg2);
	return 0;
}

s32 lsx_5206_register(void)
{
	instruction_register(0xE088, 0xF0D8, &execute, &disassemble);
	instruction_register(0xE088, 0xF0D8, &execute, &disassemble);
	return 2;
}
