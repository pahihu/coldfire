/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/* Time.c                  */
/* Provide Cycle Counting  */
/* to Approximate Elapsed  */
/* Time.                   */
/* by Matt Minnis          */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

int Monitor_TIME (int argc, char **argv)
{
	register u32 seconds;
	register u32 nano_seconds;
	struct _board_data *b = board_get_data();
	
	/*    1e9 ns              1             ns
	 * ------------  *  -------------  =  ------
	 *    second        cycles/second      cycle
	 */
	u32 ns_per_cycle = 1000000000 / b->clock_speed;
	/* Remember, clock speed is   cycles in a second */

	TRACE("ns_per_cycle = %u\n", ns_per_cycle);

	if(argc==2 && (argv[1][0] == 'c' || argv[1][0] == 'r')) {
		b->total_cycle_count += b->cycle_count;
		b->cycle_count = 0;
		printf("Cycle Counter Cleared:  OK\n");

	} else {
		u32 total = b->total_cycle_count + b->cycle_count;
		/*    cycles           1           seconds
		 *             * -------------  =
		 *               cycles/second */
		seconds = total / b->clock_speed;
		/* Grab cycles and multiply by ns / cycle to give ns */
		nano_seconds = (total % b->clock_speed) * ns_per_cycle;
		printf("\nTotal Cycles: %8u (%u.%09us)\n", total, 
						seconds, nano_seconds);
		seconds = b->cycle_count / b->clock_speed;
		nano_seconds = (b->cycle_count % b->clock_speed) * ns_per_cycle;
		printf("      Cycles: %8u (%u.%09us)\n", b->cycle_count,
						seconds, nano_seconds);

	} 
	return 1;
}
 
