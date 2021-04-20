/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

#define MAX_BREAKPOINTS 16
struct _breakpoint {
	u32 address;
	u32 count;
	u32 save_word;
	char trigger;
} breakpoint[MAX_BREAKPOINTS];
/*	= {[0 ... MAX_BREAKPOINTS-1] {0,0,0,0}};*/

int breakpoint_count = 0;

int Monitor_BR(int argc, char **argv)
{
	u32 addr;
	int x;
	if(argc < 2) {
		/* Print breakpoints */
		printf(" Address      Count  Trigger\n");
		for(x=0; x<breakpoint_count;x++) {
			printf("%08x:  %08x %08x\n", 
				breakpoint[x].address, breakpoint[x].count, 
				breakpoint[x].trigger);
		}
		return 1;
	}

	/* Pull the address */
	sscanf(argv[1], "%x", &addr);

	for(x=0; x<MAX_BREAKPOINTS;x++) {
		if(breakpoint[x].address == addr) {
			/* FIXME: enable the breakpoint??? maybe?? */
			return 1;
		}
	}
	if(x==MAX_BREAKPOINTS) {
		/* Breakpoint not found, add it */
		if(breakpoint_count < MAX_BREAKPOINTS) {
			breakpoint[breakpoint_count].address = addr;
			breakpoint[breakpoint_count].count = 0;
			breakpoint[breakpoint_count].trigger = 1;
			/* Save word will be written when execution is
			 *  resumed, except if the breakpoint is the
			 *  current PC, in which case it won't, and we'll 
			 *  restore it as 0x0000. So we might as well always
			 *  get the saveword */
			Memory_RetrWord(&breakpoint[breakpoint_count].save_word, breakpoint[breakpoint_count].address);
			breakpoint_count++;
		}
	}
	return 1;
}


void Monitor_BR_EnterException(void)
{
	int x;
	/* Here, we want to write the save words over all the ILLEGAL
	 * instruction's we've written, this is so DI doesn't show
	 * the ILLEGAL instructions */
	for(x=0;x<MAX_BREAKPOINTS;x++) {
		if(breakpoint[x].trigger) {
			TRACE("restoring breakpoint %d with 0x%04x\n", x, breakpoint[x].save_word);
			Memory_StorWord(breakpoint[x].address, breakpoint[x].save_word);
		}
	}
}

void Monitor_BR_ExitException(void)
{
	int x;
	u32 current_PC;
	Memory_Retr(&current_PC, 32, memory_core.a[7]+4);

	/* This writes all the breakpoints __except one that that is on the
	 * PC, if any__ with ILLEGAL instrtuctions, and saves the 
	 * old word in the breakpoint structure */
	for(x=0;x<MAX_BREAKPOINTS;x++) {
		if(breakpoint[x].trigger) {
			if(breakpoint[x].address == current_PC) continue;
			Memory_RetrWord(&breakpoint[x].save_word, breakpoint[x].address);
			Memory_StorWord(breakpoint[x].address, 0x4AFC);
		}
	}
}

void Monitor_BR_Entry(s16 Vector, char *enter_monitor, char *dump_info)
{
	int x;
	u32 current_PC;

	Memory_Retr(&current_PC, 32, memory_core.a[7]+4);

	*dump_info=1;
	*enter_monitor = 1;
	/* See if we hit a breakpoint */
	for(x=0;x<MAX_BREAKPOINTS;x++) {
		if(breakpoint[x].address == current_PC) {
			/* Breakpoint */
			printf("Breakpoint encountered at 0x%08X\n", 
								current_PC);
			breakpoint[x].count++;
			return;
		}
	}
	printf("Illegal Instruction at 0x%08x\n", current_PC);
}
