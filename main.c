/**********************************/
/*				  */
/*  Copyright 2000, David Grant   */
/*				  */
/*  see LICENSE for more details  */
/*				  */
/**********************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"
#include "version.h"

TRACER_DEFAULT_CHANNEL(main);

void main_show_help(char **argv)
{
	printf("\nUsage: %s [options]\n", argv[0]);
	printf("Options:\n");
	printf("\t--board <filename>  Load board config file <filename>\n");
	printf("\t--timerhack         Change the timer so the TRR ticks once \n"
	       "\t                      per instruction, instead of at an \n"
	       "\t                      almost correct rate based on \n"
	       "\t                      cycles executed\n");
	printf("\n");
	printf("All CPU options depricated.  Use a board config file instead.\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	int x;	
	char *board_config_file = NULL;
	extern struct _board_data board_data;

	/* Init the board, but don't load the config file yet */
	board_init();
	
	for(x=1;x<argc;x++) {
		if(strcasecmp(argv[x], "--trace")==0) {
			tracer_setuptrace(argv[++x]);
		} else if ((strcasecmp(argv[x], "-v")==0) 
		   || (strcasecmp(argv[x], "--ver")==0 )
		   || (strcasecmp(argv[x], "--version")==0 )) {
			printf("*********************\n");
			printf("%s\n%s\n",TITLE,COPYRIGHT);
			printf("Built:  %s    ", __DATE__);
			printf("Version: %s\n", VERSION);
			printf("\n");
			return 0;
		} else if(strcasecmp(argv[x], "--board") == 0) {
			board_config_file = argv[++x];
		} else if ((strcasecmp(argv[x], "--help")==0)
		   || (strcasecmp(argv[x], "--h")==0)
		   || (strcasecmp(argv[x], "-h")==0)
		   || (strcasecmp(argv[x], "-?")==0)) {
			main_show_help(argv);
			return 0;
		} else if(strcasecmp(argv[x], "--timerhack") == 0) {
			board_data.use_timer_hack = 1;
		} else if(strcasecmp(argv[x], "--tracerun") == 0) {
			board_data.trace_run = 1;
		}
			
				
		
	}

	printf("Use CTRL-C (SIGINT) to cause autovector interrupt 7 (return to monitor)\n");


	/* Initialiize modules */
	Memory_Init();
	Instruction_Init();
	
#ifdef INSTRUCTION_PROFILE /* Native x86 profiling for debugging the emulator */
	Profile_Init();   /* At least mention that it is on.  :) */
	Profile_time_in_ms();
#endif

	/* Now that everything is initialized, load the board 
	 * configuration.  This reads the CPU type and loads 
	 * instructions for the CPU, and it loads the memory map data */
	printf("Loading board configuration...\n");
	board_setup(board_config_file);

	printf("!!! Remember to telnet to the above ports if you want to see any output!\n");

	/* Once the entire board is configured, reset the board (like 
	 * pressing the reset button) */
	printf("Hard Reset...\n");
	board_reset();
	
	Run();

	printf("Shutting down the emulator...\n");

	Instruction_DeInit();
	Memory_DeInit();

	board_fini();
	printf("Done.\n\n");
	
	return 0;
}
