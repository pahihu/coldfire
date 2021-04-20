/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*  & Matt Minnis                 */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

/* From: Start of address to disassemble
	 buffer: buffer to place disassembled line 
	 RETURNS: Length of disassembled instruction, in bytes */
int Monitor_InstructionDI(s32 FromPC, char *buffer)
{
	u32 Value;
	s32 x;
	s32 OldPC;
	char InstrStr[16];
	char Arg1[32];
	char Arg2[32];
	struct _Instruction *InstructionPtr;

	buffer[0]=0;

	/* Save the PC, and set the new value */
	OldPC=memory_core.pc;
	memory_core.pc=FromPC;

	/* Retrieve the instruction */
	Memory_RetrWord(&Value, memory_core.pc);
	buffer+=sprintf(buffer,"%08X: ", memory_core.pc);

	InstructionPtr=Instruction_FindInstruction(Value);

	if(InstructionPtr==NULL) {
		memory_core.pc+=2;
	} else {
		/* disassemble the instruction */
		(*InstructionPtr->DIFunctionPtr)(&InstrStr[0], &Arg1[0], &Arg2[0]);
		/* Here, the PC will have changed, so we can tell if we used instructions or not */
		for(x=FromPC;x<FromPC+8;x+=2) {
			if(x < memory_core.pc) {
				Memory_RetrWord(&Value, x);
				buffer+=sprintf(buffer,"%04hX ", (u16)Value);
			} else
				buffer+=sprintf(buffer,"     ");
		}

		buffer+=sprintf(buffer," %-9s ", &InstrStr[0]);
		if(Arg1[0]) buffer+=sprintf(buffer,"%s", &Arg1[0]);
		if(Arg2[0]) buffer+=sprintf(buffer,",%s", &Arg2[0]);
	}
	/* Find the length of the instruction */
	x = memory_core.pc - FromPC;
	/* Restore the PC */
	memory_core.pc=OldPC;
	return x;
}

s32 di_saved_PC;
int Monitor_DI(int argc, char **argv)
{
	u32 addr;
	char buffer[128];
	int x;

	addr=di_saved_PC;
	if(argc > 1) sscanf(argv[1], "%x", &addr);

	for(x=0;x < monitor_config.disassemble_lines; x++) {
		addr += Monitor_InstructionDI(addr, buffer);
		printf("%s\n", buffer);
	}
	di_saved_PC = addr;
	return 1;
}

