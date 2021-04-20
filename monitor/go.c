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

int Monitor_GO(int argc, char **argv)
{
	if(argc == 1) {
		/* Use the existing PC */
	} else {
		/* Take the AddressString, and stuff it into PC */
		sscanf(argv[1], "%x", &memory_core.pc);
		/* Also stuff it into the stackframe */
		Memory_Stor(32,memory_core.a[7]+4,memory_core.pc);
	}
	return 0;
}

