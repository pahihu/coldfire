/**********************************/
/*                                */
/*  Copyright 2004, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#define TRACER_OFF*/
#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(sim);


struct _sim *sim = NULL;

void sim_register(struct _sim *sim_data)
{
	if(sim) {
		printf("Attempt to register a second sim module.\n");
		printf("Probably an error in a board config file.\n");
		exit(1);
	}
	sim = sim_data;
}

