#ifndef PROFILE_H
#define PROFILE_H

#ifdef INSTRUCTION_PROFILE

u32 Profile_time_in_ms(void);
void Profile_MakeTimeString(char *Buffer, u32 s32 Low, u32 s32 High);
void Profile_Init (void);


#endif
#endif

