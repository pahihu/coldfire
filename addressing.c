/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/


#include "coldfire.h"

/* Addressing Mode  Mode   Register
        Dy          000    memory_core.dnum
        Ay          001    memory_core.anum
       (Ay)         010    memory_core.anum
       (Ay)+        011    memory_core.anum
      -(Ay)         100    memory_core.anum
     (d16,Ay)       101    memory_core.anum
    (d8,Ay,Xi)      110    memory_core.anum
     (xxx).W        111    000
     (xxx).L        111    001
     #<data>        111    100
     (d16,PC)       111    010
    (d8,PC,Xi)      111    011
*/

TRACER_DEFAULT_CHANNEL(ea);


/* FUNCTION: Retrives the value in memory based on the supplied Mode and
              Register
       ARGS: Size=8,16,32, we are careful with this because an 8 bit retrive for
                  a Dx, or Ax is the LSB, but for memory addressing, it is the 8
                  bits where the pointer is pointing to.
             Mode=the mode from the instruction
             Register=the register from the instruction
             EAValue=if true, we return the address.. not what is at the address... 
                     this means that we can't use Dx,Ay, and some others (because they
                     don't have an EA
     RETURNS: The value requested, or the EA if EAValue is turned on
    COMMENTS:  */
/* -------------------------------------*/

s32 Addressing_Print(s16 Size, char Mode, char Register, char *Str)
{
	u32 Result=0;
	s16 Scale;
	struct _InstructionExtensionWord *EWordPtr = (void *)&Result;

	switch(Mode) {
	case 0: /* Dy */
		sprintf(Str, "D%d", Register);
		break;
	case 1: /* Ay */
		sprintf(Str, "A%d", Register);
		break;
	case 2: /* (Ay) */
		sprintf(Str, "(A%d)", Register);
		break;
	case 3: /* (Ay)+ */
		sprintf(Str, "(A%d)+", Register);
		break;
	case 4: /* -(Ay) */
		sprintf(Str, "-(A%d)", Register);
		break;
	case 5: /* (d16,Ay) */
		Memory_RetrWordFromPC(&Result);
		sprintf(Str, "%hd(A%d)",(s16)Result,Register);
		break;
	case 6: /* (d8,Ax,Xi) */
		Memory_RetrWordFromPC(&Result);
		if(EWordPtr->Scale == 0) Scale=1;
		else if(EWordPtr->Scale == 1) Scale=2;
		else if(EWordPtr->Scale == 2) Scale=4;
		else Scale=1;
		sprintf(Str, "%d(A%d,%c%d.L*%d)",EWordPtr->Displacement, 
			Register, EWordPtr->AD ? 'A' : 'D', EWordPtr->Register, Scale);
		break;
	case 7: /* Direct modes */
		switch(Register) {
		case 0: /* word addressing */
			Memory_RetrWordFromPC(&Result);
			sprintf(Str, "(0x%04hX.W)", (s16)Result);
			break;
		case 1: /* s32 addressing */
			Memory_RetrLongWordFromPC(&Result);
			sprintf(Str, "(0x%08X.L)", Result);
			break;
		case 2: /* (d16,PC) */
			Memory_RetrWordFromPC(&Result);
			sprintf(Str, "%hd(PC)",(s16)Result);
			break;
		case 3: /* (d8,PC,Xi) */
			Memory_RetrWordFromPC(&Result);
			if(EWordPtr->Scale == 0) Scale=1;
			else if(EWordPtr->Scale == 1) Scale=2;
			else if(EWordPtr->Scale == 2) Scale=4;
			else Scale=1;
			sprintf(Str, "%d(PC,%c%d.L*%d)",EWordPtr->Displacement, 
				EWordPtr->AD ? 'A' : 'D', EWordPtr->Register, Scale);
			break;
		case 4:

			if(Size==8) {
				Memory_RetrByteFromPC(&Result);
/*				if( (char)Result < 0)
					sprintf(Str, "#%d", (char)Result);
				else*/
					sprintf(Str, "#0x%02X", Result);
			} else if(Size==16) {
				Memory_RetrWordFromPC(&Result);
/*				if( (s16)Result < 0)
					sprintf(Str, "#%d", (s16)Result);
				else if((s16)Result == 0)
					sprintf(Str, "#0");
				else*/
					sprintf(Str, "#0x%04X", Result);
			} else if(Size==32) {
				Memory_RetrLongWordFromPC(&Result);
/*				if( (s32)Result < 0)
					sprintf(Str, "#%ld", (s32)Result);
				else*/
					sprintf(Str, "#0x%08X", Result);
			}
			break;
		default:
			sprintf(Str, "---");
			break;
		}
	}
	return 0;
}

/* This gets an address, and all useful information about an address from
 * the PC.  Once the address is retrieved, it can be used (multiple times)
 * to write to, read from, etc.  This allows us to only need to access the
 * PC once during instruction operand fetch.  So if the destionation is (say)
 * an immediate address, we already have it.. we don't need to do any PC 
 * magic to reset it, and re-read the address.  I don't know if this is 
 * actally how the Coldfire works (what happens if you write to the address
 * you're fetching from.. does the board re-fetch the operand, or just use
 * the value of the address it pulled the source from?) */

/* It also lets us do things like auto increment, and autodecrement.. and
 * stores the original address in a place where we can use it again */
char EA_GetFromPC(struct _Address *Addr, s16 Size, char Mode, char Register) 
{
	struct _InstructionExtensionWord *EWordPtr = (struct _InstructionExtensionWord *)&Addr->Data;
	s16 Scale;

	Addr->Mode=Mode;
	Addr->Register=Register;
	Addr->Data=0;	/* This is for storing operands that are in the instruction */
	Addr->Address = 0xdeadbeef;
	Addr->Size=Size;

	TRACE("Size=%d, Mode=%d, Register=%d\n", Size, Mode, Register);

#ifdef MEMORY_STATS
	/* FIXME: this could be moved to the EA_GetValue, GetEA, and PutValue functions,
	 * it might give a more accurate representation there (some instructions do a direct
	 * read from the PC if they know what they are reading, but usually they go through
	 * the EA_* routines), and would let things be split into reads/writes. */
	Stats_Build_EA(Register, Mode);
#endif /* MEMORY_STATS */

	switch(Mode) {
	case 0: /* Dy */
		TRACE("   Mode=Dy, Register=D%d=0x%08x\n", Register, memory_core.d[(int)Register]);
		return 1;
	case 1: /* Ay */
		TRACE("   Mode=Ay, Register=A%d=0x%08x\n", Register, memory_core.a[(int)Register]);
		return 1;
	case 2: /* (Ay) */
		Addr->Address = memory_core.a[(int)Register];
		TRACE("   Mode=(Ay), Register=A%d=0x%08x, Address=0x%08x\n", Register, memory_core.a[(int)Register], Addr->Address);
		return 1;
	case 3: /* (Ay)+ */
		Addr->Address = memory_core.a[(int)Register];
		TRACE("   Mode=(Ay)+, Register=A%d=0x%08x, Address=0x%08x\n", Register, memory_core.a[(int)Register], Addr->Address);
		memory_core.a[(int)Register]+=Size>>3;
		TRACE("   Incremented A%d to 0x%08x\n", Register, memory_core.a[(int)Register]);
		return 1;
	case 4: /* -(Ay) */
		memory_core.a[(int)Register]-=Size>>3;
		TRACE("   Mode=-(Ay), Register=A%d=0x%08x, Decremented 0x%08x\n", Register, memory_core.a[(int)Register], memory_core.a[(int)Register]);
		Addr->Address = memory_core.a[(int)Register];
		TRACE("   Address=0x%08x\n", Addr->Address);
		return 1;
	case 5:	/* (d16,Ay) */
		if(!Memory_RetrWordFromPC(&Addr->Data)) return 0;
		TRACE("   Mode=(d16,Ay), Register=A%d=0x%08x, Displacement=0x%04x\n", Register, memory_core.a[(int)Register], Addr->Data);
		Addr->Address = memory_core.a[(int)Register]+(s16)Addr->Data;
		TRACE("   Address=0x%08x\n", Addr->Address);
		return 1;
	case 6:	/* (d8,An,Xi) */
		if(!Memory_RetrWordFromPC(&Addr->Data)) return 0;
		if(EWordPtr->Scale == 0) Scale=1;
		else if(EWordPtr->Scale == 1) Scale=2;
		else if(EWordPtr->Scale == 2) Scale=4;
		else Scale=1;

		TRACE("   Mode=(d8,An,Xi), Displacement=%d, Index=%s%d=0x%08x * Scale=%d, Register=A%d=0x%08x\n", 
			EWordPtr->Displacement, EWordPtr->AD ? "A" : "D", 
			EWordPtr->Register, EWordPtr->AD ? memory_core.a[EWordPtr->Register] : memory_core.d[EWordPtr->Register], 
			Scale, Register, memory_core.a[(int)Register]);

		Addr->Address = memory_core.a[(int)Register] + EWordPtr->Displacement;
		/* EWordPtr->AD == 0 for memory_core.dister */
		if(EWordPtr->AD==0)
			Addr->Address += memory_core.d[(int)EWordPtr->Register] * Scale;	
		else 
			Addr->Address += memory_core.a[(int)EWordPtr->Register] * Scale;
		TRACE("   Address=0x%08x\n", Addr->Address);
		return 1;
			
	case 7: /* Direct modes */
		switch(Register) {
		case 0: /* word addressing */
			if(!Memory_RetrWordFromPC(&Addr->Data)) return 0;
			Addr->Address = Addr->Data;
			TRACE("   Mode=(xxx).W, Data=0x%04x, Address=0x%08x\n", Addr->Data, Addr->Data);
			return 1;
		case 1: /* s32 addressing */
			if (!Memory_RetrLongWordFromPC(&Addr->Data)) return 0;
			Addr->Address = Addr->Data;
			TRACE("   Mode=(xxx).L, Data=0x%08x, Address=0x%08x\n", Addr->Data, Addr->Data);
			return 1;
		case 2: /* (d16,PC) */
			/* This uses the value of the PC as the address of
				the extenstion word, we are already there */
			Addr->Address = memory_core.pc;
			/* Now alter the PC to get the extension word */
			if(!Memory_RetrWordFromPC(&Addr->Data)) return 0;
			TRACE("   Mode=(d16,PC), Displacement=0x%04x\n", (s16)Addr->Data);
			Addr->Address += (s16)Addr->Data;
			TRACE("   Address=0x%08x\n", Addr->Address);
			return 1;
		case 3: /* (d8,PC,Xi) */
			/* This uses the value of the PC as the address of
				the extenstion word, we are already there */
			Addr->Address = memory_core.pc;
			/* Now alter the PC to get the extension word */
			if(!Memory_RetrWordFromPC(&Addr->Data)) return 0;
			if(EWordPtr->Scale == 0) Scale=1;
			
			else if(EWordPtr->Scale == 1) Scale=2;
			else if(EWordPtr->Scale == 2) Scale=4;
			else Scale=1;

			TRACE("   Mode=(d8,PC,Xi), Displacement=%d, Index=%s%d=0x%08x * Scale=%d\n", 
				EWordPtr->Displacement, EWordPtr->AD ? "A" : "D", 
				EWordPtr->Register, EWordPtr->AD ? memory_core.a[EWordPtr->Register] : memory_core.d[EWordPtr->Register], 
				Scale, memory_core.a[(int)Register]);

			Addr->Address += EWordPtr->Displacement;
			/* EWordPtr->AD == 0 for memory_core.dister */
			if(EWordPtr->AD==0)
				Addr->Address += memory_core.d[(int)EWordPtr->Register] * Scale;
			else 	
				Addr->Address += memory_core.a[(int)EWordPtr->Register] * Scale;
			TRACE("   Address=0x%08x\n", Addr->Address);
			return 1;

		case 4:
			if(!Memory_RetrFromPC(&Addr->Data, Size)) return 0;
			Addr->Address = 0xdeadbeef;
			if(Size==8) {
				TRACE("   Mode=#<data>, Data=0x%02x\n", Addr->Data);
			} else if(Size==16) {
				TRACE("   Mode=#<data>, Data=0x%04x\n", Addr->Data);
			} else {
				TRACE("   Mode=#<data>, Data=0x%08x\n", Addr->Data);
			}
			return 1;
		}
		/* Should never get here */
		break;
	}
	return 0;
}


/* Takes an address, (that we build from EA_GetFromPC), and returns the 
 * value associated with it (with proper masking of bits)
 * This sign extends the return so it can be used directly for math ops
 * or for negative compares without worrying about actual size */
char EA_GetValue(u32 *Result, struct _Address *Addr)
{
	char ReturnValue = 1;
	switch(Addr->Mode) {
	case 0: /* Dy */
		TRACE("Retrieving size=%d value from D%d\n", Addr->Size, Addr->Register);
		*Result = memory_core.d[(int)Addr->Register];
		break;
	case 1: /* Ay */
		TRACE("Retrieving size=%d value from A%d\n", Addr->Size, Addr->Register);
		*Result = memory_core.a[(int)Addr->Register];
		break;
	case 2: /* (Ay) */
	case 3: /* (Ay)+ */
	case 4: /* -(Ay) */
	case 5:	/* (d16,Ay) */
	case 6:	/* (d8,An,Xi) */
		TRACE("Retrieving size=%d value from address 0x%08x\n", Addr->Size, Addr->Address);
		ReturnValue = Memory_Retr(Result, Addr->Size,Addr->Address);
		break;
			
	case 7: /* Direct modes */
		switch(Addr->Register) {
		case 0: /* word addressing */
		case 1: /* s32 addressing */
		case 2: /* (d16,PC) */
		case 3: /* (d8,PC,Xi) */
			TRACE("Retrieving size=%d value from address 0x%08x\n", Addr->Size, Addr->Address);
			ReturnValue = Memory_Retr(Result, Addr->Size,Addr->Address);
			break;
		case 4:
			TRACE("Retrieving size=%d value from immediate data\n", Addr->Size);
			*Result = Addr->Data;
			break;
		}
		break;
	}
	TRACE("32 bit value... 0x%08x\n", *Result);
	/* Now mask it through the size ..
             eg  & 0x000000FF for 8bit, etc. */
	if(Addr->Size & 0x0020) return 1;
	if(Addr->Size & 0x0010) *Result = (s16)*Result;
	else			*Result = (char)*Result;
	TRACE("Returning size modified value... 0x%08x\n", *Result);
	return ReturnValue;
}

/* This is used by instructions that play with effective addresses (EAs)
 * instead of getting the value at an addrss, we get the actual address */
char EA_GetEA(u32 *Result, struct _Address *Addr)
{
	switch(Addr->Mode) {
	case 0: /* Dy */
	case 1: /* Ay */
	case 3: /* (Ay)+ */
	case 4: /* -(Ay) */
		ERR("Can't get the EA of a register..\n");
		return 0;
	case 2: /* (Ay) */
	case 5:	/* (d16,Ay) */
	case 6:	/* (d8,An,Xi) */
		TRACE("Retrieving Effective Address from address 0x%08x\n", Addr->Address);
		*Result = Addr->Address;
		break;
			
	case 7: /* Direct modes */
		switch(Addr->Register) {
		case 0: /* word addressing */
		case 1: /* s32 addressing */
		case 2: /* (d16,PC) */
		case 3: /* (d8,PC,Xi) */
			TRACE("Retrieving Effective Address from address 0x%08x\n", Addr->Address);
			*Result = Addr->Address;
			break;
		case 4:
			return 0;
		}
		break;
	}
	TRACE("   Value=0x%08x\n", *Result);
	return 1;
}

/* Given a value, and an address, this puts that value */
void EA_PutValue(struct _Address *Addr, u32 Value)
{
	/* Value is s32, sign extended */
	
	switch(Addr->Mode) {
	case 0: /* Dy */
		TRACE("Storing size=%d value=0x%08x into D%d\n", Addr->Size, Value, Addr->Register);
		/* Coldfire preserves the bits not written to when writing
		 *  to a D register */
		if(Addr->Size & 0x0020) {
			memory_core.d[(int)Addr->Register] = Value;
			return;
		} else if(Addr->Size & 0x0010) {
			memory_core.d[(int)Addr->Register] &= 0xFFFF0000;
			memory_core.d[(int)Addr->Register] |= Value & 0x0000FFFF;
			return;
		} else {
			memory_core.d[(int)Addr->Register] &= 0xFFFFFF00;
			memory_core.d[(int)Addr->Register] |= Value & 0x000000FF;
			return;
		}
		return;
	case 1: /* Ay */
		TRACE("Storing size=%d value=0x%08x into A%d\n", Addr->Size, Value, Addr->Register);
		/* for both word and s32 writes to the A register, we 
		 * store the s32 sign extended value, but for byte writes, 
		 * we only overwrite the lowest byte. */
		if(Addr->Size & 0x0030) {
			memory_core.a[(int)Addr->Register] = Value;
			return;
/*		} else if(Addr->Size & 0x0010) {
			memory_core.a[(int)Addr->Register] &= 0xFFFF0000;
			memory_core.a[(int)Addr->Register] |= Value & 0x0000FFFF;
			return;*/
		} else {
			memory_core.a[(int)Addr->Register] &= 0xFFFFFF00;
			memory_core.a[(int)Addr->Register] |= Value & 0x000000FF;
			return;
		}
		
		return;
	case 2: /* (Ay) */
	case 3: /* (Ay)+ */
	case 4: /* -(Ay) */
	case 5:	/* (d16,Ay) */
	case 6:	/* (d8,An,Xi) */
		if(Addr->Size & 0x0020);
		else if(Addr->Size & 0x0010) 	Value = (u16)Value;
		else				Value = (unsigned char)Value;
		TRACE("Storing size=%d value=0x%08x into address 0x%08x\n", Addr->Size, Value, Addr->Address);
		Memory_Stor(Addr->Size,Addr->Address,Value);
		return;
			
	case 7: /* Direct modes */
		switch(Addr->Register) {
		case 0: /* word addressing */
		case 1: /* s32 addressing */
			if(Addr->Size & 0x0020);
			else if(Addr->Size & 0x0010) 	Value = (u16)Value;
			else				Value = (unsigned char)Value;
			TRACE("Storing size=%d value=0x%08x into address 0x%08x\n", Addr->Size, Value, Addr->Address);
			Memory_Stor(Addr->Size,Addr->Address,Value);
			return;
		case 2: /* (d16,PC) */
		case 3: /* (d8,PC,Xi) */
		case 4:
			ERR("Can't write to the PC, go away.\n");
			return;
		}
		/* Shouldn't get here */
		break;
	}
}






void Stack_Push(s16 Size, u32 Value)
{
	struct _Address Dest;
	TRACE("SP=0x%08x, Size=%d, Pushing 0x%08x onto stack (predecrement)\n", memory_core.a[7], Size, Value);
	EA_GetFromPC(&Dest, Size, 4, 7);
	EA_PutValue(&Dest, Value);
	TRACE("Done\n");

}

u32 Stack_Pop(s16 Size)
{
	struct _Address Dest;
	u32 Value;
	TRACE("SP=0x%08x, Popping a value off the stack (postincrement)\n", memory_core.a[7]);
	EA_GetFromPC(&Dest, Size, 3, 7);
	EA_GetValue(&Value, &Dest);
	TRACE("Done, Value=0x%08x, Size=%d\n", Value, Size);
	return Value;
}


