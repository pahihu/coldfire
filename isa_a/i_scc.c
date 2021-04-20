/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Set Conditionally */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 1 |   Condition   | 1 | 1 | 0 | 0 | 0 |  Register |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

*/

const char *code_mnemonic[16] = { "T", "F", "HI", "LS", "CC", "CS", "NE", "EQ",
			"VC", "VS", "PL", "MI", "GE", "LT", "GT", "LE"};

TRACER_DEFAULT_CHANNEL(i_scc);

INSTRUCTION_4ARGS(SCC,
	unsigned Code2,4,
	unsigned Condition,4,
	unsigned Code1,5,
	unsigned Register,3);

int SCCTime=1;

static void execute(void)
{
	struct _Address Destination;
	SCC_Instr Instr;
	unsigned char Result=0;

	Memory_RetrWordFromPC(&Instr.Code);

	if(!EA_GetFromPC(&Destination, 8, 0, Instr.Bits.Register)) return;

	cycle(SCCTime);

	switch(Instr.Bits.Condition) {
	case 0: /* True */
		Result=1;
		break;
	case 1: /* False */
		break;
	case 2: /* SHI */
		/* The docs say (!C or !Z), however processor seems to do 
		 * this: */
		Result = memory_core.sr.c || !memory_core.sr.z;
		break;
	case 3: /* SLS */
		Result = memory_core.sr.c || memory_core.sr.z;
		break;
	case 4: /* SCC */
		Result = !memory_core.sr.c;
		break;
	case 5: /* SCS */
		Result = memory_core.sr.c;
		break;
	case 6: /* SNE */
		/* Set if they are not equal, ie Dest-Source != 0 */
		Result = (!memory_core.sr.z);
		break;
	case 7: /* BEQ */
		/* Don't set if they are not equal */
		Result = (memory_core.sr.z);
		break;
	case 8: /* SVC */
		Result = (!memory_core.sr.v);
		break;
	case 9: /* SVS */
		Result = memory_core.sr.v;
		break;
	case 10: /* SPL */
		Result = !memory_core.sr.n;
		break;
	case 11: /* SMI */
		Result = memory_core.sr.n;
		break;
	case 12: /* SGE */
		Result = ((memory_core.sr.n && memory_core.sr.v) || 
			(!memory_core.sr.n && !memory_core.sr.v));
		break;
	case 13: /* SLT */
		Result = ((memory_core.sr.n && !memory_core.sr.v) || 
			(!memory_core.sr.n && memory_core.sr.v));
		break;
	case 14: /* SGT */
		Result = ((memory_core.sr.n && memory_core.sr.v && !memory_core.sr.z) || 
			(!memory_core.sr.n && !memory_core.sr.v && !memory_core.sr.z));
		break;
	case 15: /* SLE */
		Result = (memory_core.sr.z || (memory_core.sr.n && !memory_core.sr.v) ||
			(!memory_core.sr.n && memory_core.sr.v) );
		break;
	default:
		ERR("Unknown Condition Code 0x%02x\n", Instr.Bits.Condition);
		return;
	}
	TRACE("S%s: Result=%d\n", code_mnemonic[(int)Instr.Bits.Condition],
					Result);
	if(Result) Result = 0x000000FF;

	TRACE("Storing Result:\n");
	EA_PutValue(&Destination, Result);
	
	
	return;
}


static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	
	SCC_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	sprintf(Instruction, "S%s", code_mnemonic[(int)Instr.Bits.Condition]);
	sprintf(Arg1, "D%d", Instr.Bits.Register);
	Arg2[0]=0;	
	return 0;
}

s32 scc_5206_register(void)
{
	instruction_register(0x50C0, 0xF0F8, &execute, &disassemble);
	return 1;
}
