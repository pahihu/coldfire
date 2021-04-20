/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include "coldfire.h"

/* 4 formats for divide 
 
DIVS Word Format:
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 0 | 0 | Register  | 1 | 1 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

DIVU Word Format:
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 1 | 0 | 0 | 0 | Register  | 0 | 1 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

DIVS Long Format:
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 |Register Dx| 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |Register Dx|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

DIVU Long Format:
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 |Register Dx| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |Register Dx|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

2 remainder formats:
REMS Long format 
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 |Register Dx| 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |Register Dw|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
Note that this is the same as the DIVS format, when Dq==Dr, then
it's DIVS, else, it's REMS

REMU Long format 
 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 | 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 1 |  EA Mode  |EA Register|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| 0 |Register Dx| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |Register Dw|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
Again, when Dq==Dr this is DIVU, else it's REMU

REM <ea>y,Dw:Dx   32bit Dx / 32bit <ea>y => 32bit Dw
*/

int DIVSTime[8]={18, 20, 20, 20, 20, -1, -1, -1};

TRACER_DEFAULT_CHANNEL(i_div);


#define DIV_W_REGISTER(word) 	(((word)&0x0e00) >> 9)
#define DIV_W_DIVS(word)	(((word)&0x0100))
#define DIV_L_DX(word)		(((word)&0x7000) >> 12)
#define DIV_L_DW(word)		(((word)&0x0007))
#define DIV_L_DIVS(word)	(((word)&0x0800))

#define DIV_EAMODE(word)	(((word)&0x0038) >> 3)
#define DIV_EAREG(word)		(((word)&0x0007))

static void execute(void) 
{
	struct _Address source, destination, remainder;
	u32 code[2];
	u32 s, d, r;

//	printf("DIV executed!\n");


//	printf("PC=%ld\n", memory_core.pc);
	
	Memory_RetrWordFromPC(&code[0]);

	if(code[0] & 0x8000) {
		/* word format */
//		char overflow=0;
		if(!EA_GetFromPC(&source, 16, DIV_EAMODE(code[0]), 
					DIV_EAREG(code[0]) )) return ;
		if(!EA_GetFromPC(&destination, 32, 0, DIV_W_REGISTER(code[0]))) 
					return ;
		EA_GetValue(&s, &source);
		EA_GetValue(&d, &destination);
		if(DIV_W_DIVS(code[0])) {
			/* Signed operation */
			TRACE("signed word divide %ld / %ld\n", 
					(signed)d, (signed)s);
			r = (signed)d % (signed)s;
			d = (signed)d / (signed)s;
			if((signed)d < 0 && ((d&0xffff0000) == 0xffff0000) ) {
				d &= 0x0000ffff;
			}
			memory_core.sr.n = ((signed)d < 0) ? SR_N : 0;
		} else {
			TRACE("signed word divide %lu / %lu\n", d,s);
			r = d % s;
			d = d / s;
			memory_core.sr.n = 0;
		}
		TRACE(" (q=0x%08x, r=0x%08x)\n", d,r);
		
		if(d & 0xffff0000) {
			/* Overflow, result doesn't fit in 16 bits */
			memory_core.sr.n=0;
			memory_core.sr.z=0;
			memory_core.sr.v=SR_V;
			TRACE("overflow, q doesn't fit in 16 bits\n");
		} else {
			d = ((r & 0x0000ffff) << 16) | (d & 0x0000ffff);
			memory_core.sr.z = ((d & 0x0000ffff) == 0) ? SR_Z : 0;
			memory_core.sr.v = 0;
			TRACE("= 0x%08lx\n",d);
			EA_PutValue(&destination, d);
		}
		memory_core.sr.c = 0;
		
	} else {
		/* s32 format */
		char result_negative = 0;
//		char overflow = 0;
		Memory_RetrWordFromPC(&code[1]);
		if(!EA_GetFromPC(&source, 32, DIV_EAMODE(code[0]), 
					DIV_EAREG(code[0]) )) return;
		if(!EA_GetFromPC(&destination, 32, 0, DIV_L_DX(code[1]) )) 
					return ;
		if(!EA_GetFromPC(&remainder, 32, 0, DIV_L_DW(code[1]) )) 
					return ;
		EA_GetValue(&s, &source);
		EA_GetValue(&d, &destination);
		if(DIV_L_DIVS(code[1])) {
			TRACE("Signed s32 divide/remainder\n");
			/* Turn source and dest into signed values */
			result_negative = (s ^ d) & 0x8000000;
			if((signed)s < 0) s = -(signed)s;
			if((signed)d < 0) d = -(signed)d;
		} else {
			TRACE("unigned s32 divide/remainder\n");
		}
		TRACE("	d=%08lx / s=0x%08lx (neg=%d)\n", d, s, 
				result_negative ? 1 : 0);
		r = d % s;
		d = d / s;
		if(result_negative) d = -(signed)d;
		
		TRACE("	= d=%08lx : r=0x%08lx\n", d, r); 

		if(DIV_L_DX(code[1]) != DIV_L_DW(code[1])) {
			/* REM operation */
			TRACE("writing remainder\n");
			EA_PutValue(&remainder, r);
			memory_core.sr.n = 0; /* cannot have a negative remainder */
			memory_core.sr.z = (r == 0) ? SR_Z : 0;
		} else {
			TRACE("writing quotient\n");
			EA_PutValue(&destination, d);
			memory_core.sr.n = result_negative ? SR_N : 0;
			memory_core.sr.z = (d == 0) ? SR_Z : 0;
		}
		memory_core.sr.c = 0;
		memory_core.sr.v = 0;
	}

	TRACE("Done\n");
	
//FIXME:cycle(DIVSTime[cycle_EA(DIV_EAREG(code[0]),DIV_EAMODE(code[0]))]);
	return;
}

static s32 disassemble(char *Instruction, char *Arg1, char *Arg2) 
{
	u32 code[2];
	
	Memory_RetrWordFromPC(&code[0]);
	
	Addressing_Print(32, DIV_EAMODE(code[0]), DIV_EAREG(code[0]),Arg1);
	if(code[0] & 0x8000) {
		/* word */
		sprintf(Instruction, "DIV%c.W", DIV_W_DIVS(code[0]) ? 'S':'U');
		Addressing_Print(32, 0, DIV_W_REGISTER(code[0]), Arg2);
	} else {
		char rem=0;
		Memory_RetrWordFromPC(&code[1]);
		Arg2[0] = 0;
		/* s32 */
		if(DIV_L_DX(code[1]) != DIV_L_DW(code[1])) {
			rem=1;
			Addressing_Print(32, 0, DIV_L_DW(code[1]), Arg2);
			strcat(Arg2, ":");
		}
		Addressing_Print(32, 0, DIV_L_DX(code[1]), &Arg2[strlen(Arg2)]);

		sprintf(Instruction, "%s%c.L", rem ? "REM" : "DIV", 
				DIV_L_DIVS(code[1]) ? 'S':'U');
	}
	return 0;
}

s32 div_5206e_register(void)
{
	instruction_register(0x81C0, 0xF0C0, &execute, &disassemble);
	instruction_register(0x4C40, 0xFFC0, &execute, &disassemble);
	return 6;
}

