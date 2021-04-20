/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <stdarg.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(handlers);


void SR_Set(s16 Instr, s32 Source, s32 Destination, s32 Result)
{
	char Sm = (Source >= 0) ? 0 : 1;
	char Dm = (Destination >= 0) ? 0 : 1;
	char Rm = (Result >= 0) ? 0 : 1;
	struct _sr sr_bak;

	memcpy(&sr_bak, &memory_core.sr, sizeof(struct _sr));

	TRACE("Setting Source=0x%08lx, Destination=0x%08lx, Result=0x%08lx\n", Source, Destination, Result);
	TRACE("Sm=%d, Dm=%d, Rm=%d\n", Sm,Dm,Rm);

	/* Clear out VCX */
	memory_core.sr.v = 0;
	memory_core.sr.c = 0;
	memory_core.sr.x = 0;

	/* N - Set if result is -ve, cleared otherwise 
	 * They all do this */
	memory_core.sr.n = Rm ? SR_N : 0;
	/* Z - Set if result is zero, cleared otherwise
	 * They all do this too, except I_ADDX, I_SUBX, I_NEGX */
	memory_core.sr.z = (Result == 0) ? SR_Z : 0;
	
	

	switch(Instr) {
	case I_ADDX:
		/* Z - cleared if result is non-zero, unchanged otherwise */
	case I_ADD: case I_ADDI: case I_ADDQ:
		/* Set the status register */
		/* X - Set to value of carry bit
		   N - Set if result is -ve, cleared otherwise
		   Z - Set if result is zero, cleared otherwise
		   V - Set if an overflow occurs, cleared otherwise
		   C - Set if a carry is generated, cleared otherwise
		*/
		if(Instr==I_ADDX) {
			/* If result is 0, Z is unaffected, else cleared */
			memory_core.sr.z = (Result == 0) ? sr_bak.z : 0;
		}
		
		if((Sm && Dm && !Rm) || (!Sm && !Dm && Rm) )
			memory_core.sr.v = SR_V;

		if((Sm && Dm) || (!Rm && Dm) || (Sm && !Rm) ) {
			memory_core.sr.c = SR_C;
			memory_core.sr.x = SR_X;
		}
		break;
	case I_SUBX:
		/* Z - cleared if result is non-zero, unchanged otherwise */
	case I_SUB: case I_SUBI: case I_SUBQ:
		/* Set the status register */
		/* X - Set to value of carry bit
		   N - Set if result is -ve, cleared otherwise
		   Z - Set if result is zero, cleared otherwise
		   V - Set if an overflow occurs, cleared otherwise
		   C - Set if a borrow occurs, cleared otherwise
		*/
		if(Instr==I_SUBX) {
			memory_core.sr.z = (Result == 0) ? sr_bak.z : 0;
		}
		
		if((!Sm && Dm && !Rm) || (Sm && !Dm && Rm) )
			memory_core.sr.v = SR_V;

		if((Sm && !Dm) || (Rm && !Dm) || (Sm && Rm) ) {
			memory_core.sr.c = SR_C;
			memory_core.sr.x = SR_X;
		}
		
		break;
	case I_CMP: case I_CMPA: case I_CMPI:
		/* Set the status register
		 *  X - Not affected 
		 *  N - Set if result is -ve, cleared otherwise
		 *  Z - Set if result is zero, cleared otherwise
		 *  V - Set if an overflow occurs, cleared otherwise
		 *  C - Set if a borrow occurs, cleared otherwise
		 */
		
		if((!Sm && Dm && !Rm) || (Sm && !Dm && Rm) )
			memory_core.sr.v = SR_V;

		if((Sm && !Dm) || (Rm && !Dm) || (Sm && Rm) )
			memory_core.sr.c = SR_C;
		
		/* Restore X */
		memory_core.sr.x = sr_bak.x;
		
		break;

	case I_NEG: 
		/* X - Set to value of carry bit
		   N - Set if result is -ve, cleared otherwise
		   Z - Set if result is zero, cleared otherwise
		   V - Set if an overflow occurs, cleared otherwise
		   C - Cleared if the result is zero, set otherwise
		*/
		if(Dm && Rm) memory_core.sr.v = SR_V;
		if(Dm || Rm) {
			memory_core.sr.c = SR_C;
			memory_core.sr.x = SR_X;
		}
		break;
		
	case I_NEGX:
		/* X - Set to value of carry bit
		   N - Set if result is -ve, cleared otherwise
		   Z - Cleared if the result is non-zero, unchanged otherwise
		   V - Set if an overflow occurs, cleared otherwise
		   C - Cleared if the result is zero, set otherwise
		*/
		memory_core.sr.z = (Result == 0) ? sr_bak.z : 0;
		if(Dm && Rm) memory_core.sr.v = SR_V;
		if(Dm || Rm) {
			memory_core.sr.c = SR_C;
			memory_core.sr.x = SR_X;
		}
		break;


	default:
		ERR("Called with unknown instruction %d\n", Instr);
		break;
	}
	TRACE("X:%d, Neg:%d, Zero:%d, Overflow:%d, Carry:%d\n", 
			memory_core.sr.x ? 1 : 0,
			memory_core.sr.n ? 1 : 0,
			memory_core.sr.z ? 1 : 0,
			memory_core.sr.v ? 1 : 0,
			memory_core.sr.c ? 1 : 0);
	return;
}

