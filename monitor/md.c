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
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>

#define TRACER_OFF
#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

u32 next_md_start=0;

static void dump(u32 start, u32 len)
{
	u32 offset;
	int char_num;
	char char_str[17];

	/* Start on a multiple of 16 bytes */
	offset = start & 0xfffffff0;

	char_str[16] = 0;
	char_num=0;
	while(1) {
		/* Print offset, if needed */
		if(char_num==0) printf("%08x: ", offset);

		/* If we're before the beginning or after the end, do 
		 * something useful */
		if(offset < start || len == 0) {
			char_str[char_num] = ' ';
			printf("  ");
		} else {
			/* Get the data as a charater, and print it */
			u32 data;
			/* Get the data */
			Memory_Retr(&data, 8, offset);
			/* Print it */
			printf("%02x", data & 0x000000ff);
			/* Update the string */
			char_str[char_num] = isprint(data) ? data : '.';
			/* Decrement the length, so we know when to end */
			len--;
		}
		/* Insert spaces in appropriate places */
		if(char_num % 2 == 1) printf(" ");

		/* Increment counters */
		char_num++;
		offset++;
		/* Print the end of line, and exit if needed */
		if(char_num == 0x10) {
			printf(" %s\n", char_str);
			char_num = 0;
			if(len==0) break;
		}
	}
	
}

int Monitor_MD(int argc, char **argv)
{
	u32 addr, end, len;

	addr = next_md_start;	/* Default to the next start address */
	end = 0;		/* Default to setting length to 16 lines */

	if(argc > 1) sscanf(argv[1], "%x", &addr);
	if(argc > 2) sscanf(argv[2], "%x", &end);

	/* Calculate length, which must include the end offset */
	if(end >= addr) {
		len = (end+1) - addr;
	} else {
		len = 256; /* default is 16 lines */
	}
	dump(addr, len);
	
	/* Save the position */
	next_md_start = addr+len;
	return 1;
}

