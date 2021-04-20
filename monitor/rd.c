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

/* Dump processor registers */
int Monitor_PRD(int argc, char **argv)
{
	printf("VBR: 0x%08x\n", memory_core.vbr);
	return 1;
}


void rd_dump_registers(u32 cpc, u32 csr)
{
	const char sr_str[2][17] = { "t.sm.000...xnzvc", "T.SM.111...XNZVC" };
	int x;
	extern struct _board_data board_data;
	/* Here is the format, right from coldfire
	PC: 0001002C SR: 2000 [t.Sm.000...xnzvc]
	An: 00012000 00011500 00020001 00000000 00000000 00000000 00000000 00080000
	Dn: 00DE90F0 00000000 00000000 FFFFFFFF 00000000 00000000 00000000 00000000
	*/
	/* The PC and SR */
	printf("PC: %08X SR: %04X [", cpc, csr);
	/* The expanded SR */
	for(x=0;x<16;x++)
		printf("%c", sr_str[(csr>>(15-x))&0x1][x]);
	printf("]");
	if(!monitor_config.dbug_compatibility)
		printf("  Cycles: 0x%08x (%dd)",board_data.cycle_count,board_data.cycle_count); 
		
	/* A and D registers */
	printf("\nAn:");
	for(x=0;x<7;x++) printf(" %08X", memory_core.a[x]);
	/* Add 8 to remove the exception stackframe from the stack
	 * before printing it */
	printf(" %08X", memory_core.a[x]/*+8*/); 
	printf("\nDn:");
	for(x=0;x<8;x++) printf(" %08X", memory_core.d[x]);
	printf("\n");
	
}
/* Dump register(s) */
int Monitor_RD(int argc, char **argv)
{
	u32 current_PC;
	u32 current_SR;

	Memory_Retr(&current_PC, 32, memory_core.a[7]+4);
	Memory_Retr(&current_SR, 16, memory_core.a[7]+2);
	if(argc == 1) {
		rd_dump_registers(current_PC, current_SR);
	}
/*	  else Monitor_CFRD(argv[1]);*/
	return 1;

}

