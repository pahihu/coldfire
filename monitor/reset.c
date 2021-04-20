/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/* reset.c                 */
/* by Matt Minnis          */

/* 2003-03-31 David Grant
 * 	- Changed to use board_reset 
 */


#include <stdio.h>

#include "coldfire.h"

int Monitor_RESET(int argc, char **argv)
{
	printf("Hard Reset...");
	board_reset();
	return 1;	
}
