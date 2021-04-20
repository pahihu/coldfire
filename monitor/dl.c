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

static void dl_binary(FILE *in, s32 offset)
{
	char inchar;
	
	printf("Downloading Binary image... (offset=0x%08x)\n", offset);
	while(!feof(in)) {
		fread(&inchar, 1, 1, in);
		Memory_StorByte(offset++, (char)inchar);
	}
}

static int decode_line(char *buffer, unsigned int *return_address, 
				char *return_data, char *return_checksum)
{
	unsigned int count;
	char *ptr = &buffer[4];
	int x;
	sscanf(&buffer[2], "%2x", &count);

	/* Decode the address */
	switch(buffer[1]) {
	case '0': case '1': case '5': case '9': /* 2 byte address */
		sscanf(ptr, "%4x", return_address);
		ptr+=4;
		break;
	case '2': case '8':
		sscanf(ptr, "%6x", return_address);
		ptr+=6;
		break;
	case '3': case '7':
		sscanf(ptr, "%8x", return_address);
		ptr+=8;
		break;
	}

	/* Decode the data */
	for(x=0;x<count;x++) {
		unsigned int tmp;
		sscanf(ptr, "%2x", &tmp);
		return_data[x] = tmp;
		ptr+=2;
		
	}

	/* The last piece of data is the checksum, so store that separately */
	*return_checksum = return_data[count-1];
	return_data[count-1] = 0;

	/* Return the amount of data decoded */
	return count-2;
}

static void dl_srecord(FILE *in)
{
	char buffer[128];
	s32 base_offset=0;
	int s123_count=0;
		
	printf("Downloading S-Record...\n");
	while(!feof(in)) {
		unsigned int count, address;
		char checksum;
		char data[128];
		int x;
		
		fgets(buffer, 127, in);
		if(buffer[0] != 'S') continue;

		count = decode_line(buffer, &address, &data[0], &checksum);

		switch(buffer[1]) {
		case '0':
			/* header */
/*			printf("srec info: header=%-20s\n", data);*/
			
/*			printf("srec info: module=%-20s ver=%2.2s rev=%2.2s\n",
							&data[0], &data[20], &data[22]);
			printf("srec info: comment=%s\n", &data[24]);*/
			break;
		case '1': case '2': case '3':
			/* Copy to memory */
			for(x=0;x<count;x++) {
				Memory_StorByte(x + address + base_offset, 
							data[x] & 0x000000FF);
			}
			s123_count++;
			break;
		case '5':
			/* Count previously transmitted s lines */
			if(address != s123_count) {
				printf("S5 record says there should have been %d S[123] lines, but there have only been %d.\n", 
						address, s123_count);
			}
			break;
		case '7': case '8': case '9':
			/* Set starting address */
			/* Set the PC */
			memory_core.pc = address;
			/* Also stuff it into the stackframe */
			Memory_Stor(32,memory_core.a[7]+4,memory_core.pc);
			break;
		}
	}
	printf("Done downloading S-Record.\n");

}
	

/* Takes 1 arg, the filename */
int Monitor_DL(int argc, char **argv)
{
	FILE *in;
	char type[3];
	s32 offset = 0x10000;
	char *fptr = NULL;
	int x;

	
	for(x=1;x<argc;x++) {
		if(strcasecmp(argv[x], "--offset") == 0) {
			x++;
			if(strlen(argv[x]) > 2 && argv[x][0] =='0' && 
			   (argv[x][1]=='x' || argv[x][1]=='X') ) {
				sscanf(argv[x], "%x", &offset);
			} else {
				sscanf(argv[x], "%d", &offset);
			}
		} else {
			fptr = argv[x];
		}
	}
	if(!fptr) return Monitor_show_help("dl");

	in=fopen(fptr, "rb");
	if(!in) {
		printf("Unable to open file %s\n", fptr);
		return 1;
	}

	fread(type, 2, 1, in);
	fseek(in, 0, SEEK_SET);
	
	if(type[0] == 'S' && type[1] == '0') {
		dl_srecord(in);
	} else {
		dl_binary(in, offset);
	}

	fclose(in);
	
	return 1;
}

