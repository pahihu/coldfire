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


int Monitor_MM(int argc, char **argv)
{
	s32 addr;
	u32 value;
	char command[128];
	char *cmd;
	char width;

	if(argc != 2) return Monitor_show_help("mm");

	sscanf(argv[0], "mm.%c", &width);
	sscanf(argv[1], "%x", &addr);
	
	if(width=='w') 	width=16;
	else		width=32;
	
	while(1) {
		if(width==16) {
			Memory_RetrWord(&value, addr);
			printf("%08x: [%04x] ", addr, value);
		} else {
			Memory_RetrLongWord(&value, addr);
			printf("%08x: [%08x] ", addr, value);
		}
		/* FIXME: Remove this, and have MM use the main (readlined) 
		 * prompt below */
		fgets(&command[0], 81, stdin);
		cmd=strtok(&command[0], "\r\n ");
		if(cmd==NULL)
			; /* Do nothing, just prevent the other ifs from accessing it */
		else if(strcmp(cmd, ".") == 0)
			break;
		else {
			if(width==16) {
				s16 TempS;
				sscanf(cmd, "%hx", &TempS);
				Memory_StorWord(addr, TempS);
			} else {
				s32 TempL;
				sscanf(cmd, "%x", &TempL);
				Memory_StorLongWord(addr, TempL);
			}
		}
		addr+=(width/8);
	}
	return 1;
}

