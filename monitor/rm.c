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

/* Modify a register */
int Monitor_RM(int argc, char **argv)
{
	unsigned int x;
	u32 value;
	char register_str[8];

	if(argc != 3) return Monitor_show_help("rm");
	sscanf(argv[1], "%s", register_str);
	sscanf(argv[2], "%x", &value);
	
	printf("%s = %x\n", register_str, value);

	if((register_str[0]=='D') || (register_str[0]=='d')) {
		if(1 != sscanf(register_str+1, "%u", &x)) return Monitor_show_help("rm");
		if(x > 7) {
			printf("Error: Invalid register_str: %s\n", register_str);
			return 1;
		}
		memory_core.d[x]=value;
	} else if((register_str[0]=='A') || (register_str[0]=='a')) {
		if(1 != sscanf(register_str+1, "%u", &x)) return Monitor_show_help("rm");
		if(x > 7) {
			printf("Error: Invalid register_str: %s\n", register_str);
			return 1;
		}
		memory_core.a[x]=value;
	} else if(strcasecmp(register_str, "PC") == 0) {
		memory_core.pc=value;
		/* Also stuff it into the stackframe */
		Memory_Stor(32,memory_core.a[7]+4,memory_core.pc);
	}/* else Monitor_CFRM(Str);*/
	return 1;
}

