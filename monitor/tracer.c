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

int monitor_tracer(int argc, char **argv)
{
	
	if(argc > 1)
		tracer_setuptrace(argv[1]);
	/* Stay in the monitor */
	return 1;
}


