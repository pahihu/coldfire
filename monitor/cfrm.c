/**********************************/
/*	                        */
/*  Copyright 2000, David Grant   */
/*  & Matt Minnis	         */
/*  see LICENSE for more details  */
/*	                        */
/**********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

/* Modify a CFRegister */
int Monitor_CFRM(int argc, char **argv)
{
	struct _sim_register *reg;
	u32 value=0;
	char CFRegister[8];

	if(argc!=3) return Monitor_show_help("cfrm");

	sscanf(argv[1], "%s", CFRegister);
	sscanf(argv[2], "%x", &value);

	reg = sim->register_lookup_by_name(CFRegister);
	if(!reg) {
		printf ("CFRegister: %s not found\n",CFRegister);
		return 1;
	}

	Memory_Stor(reg->width, memory_core.mbar + reg->offset, value);

	printf("%s = %x  [%s: %s]\n", CFRegister, value, reg->name, reg->description);

	return 1;
}

/* Display a CFRegister */
int Monitor_CFRD(int argc, char **argv)
{
	struct _sim_register *reg;
	char CFRegister[8];
	u32 Value;

	if(argc != 2) return Monitor_show_help("cfrd");
	sscanf(argv[1], "%s", CFRegister);

	reg = sim->register_lookup_by_name(CFRegister);
	if(!reg) {
		printf ("CFRegister: %s not found\n",CFRegister);
		return 1;
	}
	Memory_Retr(&Value, reg->width, memory_core.mbar + reg->offset);
	printf("CFRegister(%s) = %8x\n", CFRegister, Value);
	return 1;
}

/* Dump info about all sim registers */
int Monitor_CFRI(int argc, char **argv)
{
	struct _sim_register *reg;
	int x;
	char *rw[4] = { "None, hmm.. doesn't that make it difficult to access?",
			"W", "R", "R/W" };
			
	for(x=0;x<0x200;x++) {
		reg = sim->register_lookup_by_offset(x);
		if(!reg) continue;
		printf("%s: %s\n", reg->name, reg->description);
		printf("\toffset=0x%x, %d-bit, %s, reset=0x%08x\n",
			reg->offset, reg->width, rw[(reg->read<<1)+reg->write],
			reg->resetvalue);	
	}
	return 1;
}

