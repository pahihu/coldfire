#ifdef INSTRUCTION_PROFILE

#include <stdio.h>

#define PPRO 1
#define CPU_SPEED 600000000 

u32 s32 Profile_time_in_ms(void)
{
	u32 timer_hi;
	u32 timer_lo;
	u32 s32 result=0;

#ifdef PPRO
	asm volatile ("cpuid"
		: /* no ouputs */
		: /* no inputs   */
		); 
#endif
   
	asm volatile ("rdtsc;
		 movl %%edx, %0;
		 movl %%eax, %1" 
		: "=r" (timer_hi),
		  "=r" (timer_lo)
		: /* no inputs   */
		: "%edx", "%eax");

	asm volatile ("nop"
		: /* no ouputs */
		: /* no inputs   */
		); 

	result = result << 32;
	result += timer_lo;
	return(result);		 
}


static char *eng(double value, int places)
{
	const char * const prefixes[] = {
		"a", "f", "p", "n", "u", "m", "", "k", "M", "G", "T"
		};
	int p = 6;
	static char result[30];
	char *res = result;

	if (value < 0.) {
		*res++ = '-';
		value = -value;
	}
	while (value != 0 && value < 1. && p > 0) {
		value *= 1000.;
		p--;
	}
	while (value != 0 && value > 1000. && p < 10 ) {
		value /= 1000.;
		p++;
	}
	if (value > 100.)
		places--;
	if (value > 10.)
		places--;
	sprintf(res, "%.*f %s", places-1, value, prefixes[p]);
	return result;
}


void Profile_MakeTimeString(char *Buffer, u32 s32 Low, u32 s32 High)
{
	double ExecTime;
	u32 s32 Elapsed=0;
	Elapsed=(abs((High-Low)));
	ExecTime=(double) (((double)Elapsed)/(double)CPU_SPEED);
        sprintf(Buffer, "%s", eng(ExecTime,6));
}

void Profile_Init (void)
{
	printf ("Native Instruction Profiling:  Enabled\n");	
}

#endif
