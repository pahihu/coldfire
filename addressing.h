/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/


struct _Address {
	s16 Size;
	char Mode;
	char Register;
	u32 Address;
	u32 Data;
};


/*ng Addressing_Retr(s16 Size, char Mode, char Register, char EAValue);
void Addressing_Stor(s16 Size, char Mode, char Register, s32 Value);*/
s32 Addressing_Print(s16 Size, char Mode, char Register, char *Str);

char EA_GetFromPC(struct _Address *Addr, s16 Size, char Mode, char Register);
char EA_GetValue(u32 *Result,  struct _Address *Addr);
char EA_GetEA(u32 *Result, struct _Address *Addr);
/*s32 EA_GetAddress(struct _Address *Addr);*/
void EA_PutValue(struct _Address *Addr, u32 Value);

void Stack_Push(s16 Size, u32 Value);
u32 Stack_Pop(s16 Size);

