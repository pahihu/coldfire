/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* Branch Contitionally, Branch Always (BRA) and 
    Branch To Subroutine (BSR) instructions */

/* Format 
   
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 1 | 0 |   Condition   |  8-Bit displacement           |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
|                16-Bit displacement if 8bit is 00              |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
|                32-Bit displacement if 8bit is FF              |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 */
 

static u32 BRATime = 2;
static u32 BSRTime = 3;
static u32 BCCTime[] = { 3, 1, 2, 3 };

TRACER_DEFAULT_CHANNEL(i_bcc);

INSTRUCTION_3ARGS(BCC, 
	unsigned Code1,4,
	unsigned Condition,4,
	signed Displacement,8);

static void execute(void)
{

	u32 Displacement;
	/* The PC for the branch contains the address of the BCC
            instruction _plus two_ */
	u32 ReferencePC=memory_core.pc+2; 
	BCC_Instr Instr;

	Memory_RetrWordFromPC(&Instr.Code);
	Displacement=Instr.Bits.Displacement;
	if(Displacement==0) {
		Memory_RetrWordFromPC(&Displacement);
		Displacement = (s16)Displacement;
	}	
	if(Displacement == -1) {
		Memory_RetrLongWordFromPC(&Displacement);
	}
	
	TRACE("BCC: called, Displacement=%ld\n", Displacement);

	switch(Instr.Bits.Condition) {
	case 0: /* BRA */
		/* Do nothing, this is always true */
		TRACE("BCC: BRA %hx\n", Displacement);
		cycle(BRATime);
		goto i_bcc_do_branch;
	case 1: /* BSR */
		/* Save the PC in the A7 stack pointer */
		TRACE("BCC: BSR %hx\n", Displacement);
		Stack_Push(32, memory_core.pc);
		cycle(BSRTime);
		goto i_bcc_do_branch;
	case 2: /* BHI */
		/* Branch if not carry, or not zero */
		if(!memory_core.sr.c && !memory_core.sr.z) {
			TRACE("BHI: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BHI: NOT Branching\n");
		goto i_bcc_branch_not_taken;
	case 3: /* BLS */
		/* Branch if low or same  */
		if(memory_core.sr.c || memory_core.sr.z) {
			TRACE("BLS: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BLS: NOT Branching\n");
		goto i_bcc_branch_not_taken;
	case 4: /* BCC */
		/* Branch if carry cleared */
		if(!memory_core.sr.c) {
			TRACE("BCC: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BCC: NOT Branching\n");
		goto i_bcc_branch_not_taken;
	case 5: /* BCS */
		/* Branch if carry set */
		if(memory_core.sr.c) {
			TRACE("BCS: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BCS: NOT Branching\n");
		goto i_bcc_branch_not_taken;
	case 6: /* BNE */
		/* Branch if they are not equal, ie Dest-Source != 0 */
		if(!memory_core.sr.z) {
			TRACE("BNE: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BNE: NOT Branching\n");
		goto i_bcc_branch_not_taken;
	case 7: /* BEQ */
		/* Don't branch if they are not equal */
		if(memory_core.sr.z) {
			TRACE("BEQ: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BEQ: NOT branching\n");
		goto i_bcc_branch_not_taken;
	case 8: /* BVC */
		if(!memory_core.sr.v) {
			TRACE("BVC: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BVC: NOT branching\n");
		goto i_bcc_branch_not_taken;
	case 9: /* BVS */
		if(memory_core.sr.v) {
			TRACE("BVS: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BVS: NOT branching\n");
		goto i_bcc_branch_not_taken;
	
	case 10: /* BPL */
		if(!memory_core.sr.n) {
			TRACE("BPL: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		goto i_bcc_branch_not_taken;
	case 11: /* BMI */
		if(memory_core.sr.n) {
			TRACE("BMI: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		goto i_bcc_branch_not_taken;
	case 12: /* BGE */
		if((memory_core.sr.n && memory_core.sr.v) || (!memory_core.sr.n && !memory_core.sr.v)) {
			TRACE("BGE: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		goto i_bcc_branch_not_taken;
	case 13: /* BLT */
		if((memory_core.sr.n && !memory_core.sr.v) || (!memory_core.sr.n && memory_core.sr.v)) {
			TRACE("BLT: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BLT: NOT branching\n");
		goto i_bcc_branch_not_taken;
	case 14: /* BGT */
		if((memory_core.sr.n && memory_core.sr.v && !memory_core.sr.z) || (!memory_core.sr.n && !memory_core.sr.v && !memory_core.sr.z)) {
			TRACE("BGT: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BGT: NOT branching\n");
		goto i_bcc_branch_not_taken;
	case 15: /* BLE */
		if((memory_core.sr.z) || (memory_core.sr.n && !memory_core.sr.v) || (!memory_core.sr.n && memory_core.sr.v)) {
			TRACE("BLE: Branching %hx\n", Displacement);
			goto i_bcc_branch_taken;
		}
		TRACE("BLE: NOT branching\n");
		goto i_bcc_branch_not_taken;

	default:
		ERR("Unknown Condition Code 0x%02x\n", Instr.Bits.Condition);
		break;
	}
	ERR("This should NOT happen!\n");
	return;
	/* Set the new PC */
i_bcc_branch_taken:
	cycle(BCCTime[ (Displacement > 0) ? 2 : 0 ]);
i_bcc_do_branch:
	memory_core.pc=ReferencePC+Displacement;
	TRACE("BCC: New PC = %lx\n", memory_core.pc);
	return;

i_bcc_branch_not_taken:
	cycle(BCCTime[ (Displacement > 0) ? 3 : 1 ]);
	return;
}


static s32 disassemble(char *Instruction, char *Arg1, char *Arg2)
{
	u32 ReferencePC=memory_core.pc+2;
	u32 Displacement;
	BCC_Instr Instr;
	Memory_RetrWordFromPC(&Instr.Code);
	Displacement=Instr.Bits.Displacement;
	switch(Instr.Bits.Condition) {
	case 0: /* BRA */
		sprintf(Instruction, "BRA");
		break;
	case 1: /* BSR */
		sprintf(Instruction, "BSR");
		break;
	case 2: /* BHI */
		sprintf(Instruction, "BHI");
		break;
	case 3: /* BLS */
		sprintf(Instruction, "BLS");
		break;
	case 4: /* BCC */
		sprintf(Instruction, "BCC");
		break;
	case 5: /* BCS */
		sprintf(Instruction, "BCS");
		break;
	case 6: /* BNE */
		sprintf(Instruction, "BNE");
		break;
	case 7: /* BEQ */
		sprintf(Instruction, "BEQ");
		break;
	case 8: /* BVC */
		sprintf(Instruction, "BVC");
		break;
	case 9: /* BVS */
		sprintf(Instruction, "BVS");
		break;
	case 10: /* BPL */
		sprintf(Instruction, "BPL");
		break;
	case 11: /* BMI */
		sprintf(Instruction, "BMI");
		break;
	case 12: /* BGE */
		sprintf(Instruction, "BGE");
		break;
	case 13: /* BLT */
		sprintf(Instruction, "BLT");
		break;
	case 14: /* BGT */
		sprintf(Instruction, "BGT");
		break;
	case 15: /* BLE */
		sprintf(Instruction, "BLE");
		break;
	}

	if(Displacement==0) {
		Memory_RetrWordFromPC(&Displacement);
		Displacement = (s16)Displacement;
/*		sprintf(&Instruction[3], ".W");*/
	}
/*	else
		sprintf(&Instruction[3], ".B");*/
	sprintf(Arg1, "0x%08X", ReferencePC+Displacement);
	Arg2[0]=0;	
	return 0;
}

s32 bcc_isa_b_register(void)
{
	instruction_register(0x6000, 0xF000, &execute, &disassemble);
	return 3;
}
