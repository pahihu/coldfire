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
#include "version.h"

TRACER_DEFAULT_CHANNEL(monitor);


int Monitor_HELP_PrintVersion (int argc, char **argv)
{
	printf("%s\n%s\n",TITLE,COPYRIGHT);
	printf("Version: %s\n", VERSION);
	return 1;

}

int Monitor_ALIAS(int argc, char **argv)
{
	printf("\n");
	printf("Command Aliases:\n");
	printf("--------------------------\n");
	printf("help:\t\thelp, ?\n");
	printf("quit:\t\tquit, exit, q, bye \n");
	printf("reset:\t\treset, reboot \n");
	printf("set:\t\tset, env\n");
	printf("ss:\t\tstat, stats, ss \n");
	printf("trace:\t\tstep, trace, tr\n");
	printf("time:\t\ttime, t\n");
	printf("\n");
	return 1;
}


int Monitor_show_help(char *cmd)
{
	if(strcasecmp(cmd, "mm") == 0) {
		printf("MM.W (address) - Edit words at address\n");
		printf("MM.L (address) - Edit s32s at address\n");
	} else if(strcasecmp(cmd, "rm") == 0) {
		printf("RM ([A|D][0-7])|(PC) - Edit register\n");
	} else if(strcasecmp(cmd, "ss") == 0) {
		printf("SS  'SS' to show statistics , 'SS C' or 'SS Clear' to rezero\n");
	} else if(strcasecmp(cmd, "set") == 0) {
		printf("SET COMPAT [on|off] Try to behave and look like dBUG\n");
		printf("SET DISLEN <num>    Set number of lines DI disassembles\n");
	}

	return 1;
}


int Monitor_HELP(int argc, char **argv)
{
	if(argc == 2) return Monitor_show_help(argv[1]);

	printf("\n");
	Monitor_HELP_PrintVersion(0, NULL);
	printf("\n");
	printf("Commands:\n");
	printf("--------------------------\n");
	/* In alphabetical order ... */
	printf("ALIAS              List alternate command names\n");
	printf("BR [<addr>]        Display or Set Breakpoints\n");
	printf("CFRD <reg>         Coldfire CPU Peripheral Register dump\n");
	printf("CFRI               Dumps Coldfire Peripheral register names & info\n");
	printf("CFRM <reg> <val>   Modify Coldfire CPU Peripheral registers\n");
	printf("DE <filename>      Download elf object file\n");
	printf("DI <address>       Disassemble at address\n");
	printf("DL <filename>      Download filename into the emulator (.s19 or binary)\n");
	printf("DN <filename>      Download Network.  (Not implemented)\n");
	printf("EXIT               Exits the emulator\n");
	printf("GO <address>       Start running at address\n");
	printf("MM.L <address>     Memory edit (s32)\n");
	printf("MM.W <address>     Memory edit (word)\n");
	printf("RD                 Register dump\n");
	printf("PRD                Processor Register dump\n");
/*        printf("PRD                Processor Register dump\n");*/
	printf("RM  <reg> <val>    Modify cpu registers\n");
	printf("SS [(c)lear]       Show Statistics for CPU registers & addressing\n");
	printf("COMPAT <cmd> <val> Set monitor run time options\n");
	printf("TRACE <count>      Trace count instructions\n");
	printf("TIME               Show an estimate for how much time has elapsed\n");
	printf("\n");
	return 1;
}

