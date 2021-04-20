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

#define TRACER_OFF
#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);


int Monitor_QUIT(int argc, char **argv)
{
	Run_Exit = 1;
	return 0;
}
