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


char Monitor_Initialized = 0;

struct _monitor_config monitor_config;

static void Monitor_Init(void)
{
	struct _board_data *bd = board_get_data();
	printf("Initializing monitor...\n");
	/* Set default PC */
/*	memory_core.pc = 0x10000;*/

	/* Initialize default config */
	monitor_config.disassemble_lines = 16;
	monitor_config.dbug_compatibility = 1;

	if(bd->cpu == CF_5206) {
		/* Initialize the ICR registers... the real dBUG apparently 
		 * changes the default inizialization from the chip */
		unsigned char ICR_init_value[13] = {0x85, 0x8B, 0x8E, 0x12, 
			0x95, 0x98, 0x9F, 0x1E, 0x97, 0x96, 0x8C, 0x8E, 0x8D};
		int x;
		for(x=0;x<13;x++) {
			Memory_Stor(8, memory_core.mbar + 0x14 + x, 
					ICR_init_value[x]);
		}

		/* Also change the default mask */
		Memory_Stor(16, memory_core.mbar+0x36, 0x3F7E);
	}
/*
	{
		int x, y;
		char line[128];
		FILE *out;
		printf("Preping disassemble list\n");
		out=fopen("di.txt", "wt");
		for(x=0;x<0xffff;x++) { 
			Memory_Stor(16, 0x10000, x);
			printf("0x%08x\r", x);
			for(y=0;y<0xffff; y++) {
				Memory_Stor(16, 0x10002, y);
				Monitor_InstructionDI(0x10000, line);
				fprintf(out, "%s\n", line);
			}
		}
		fclose(out);
	}
*/				
	printf("Enter 'help' for help.\n");
}

struct _MonitorCommand {
	char *cmd;
	int (*func)(int argc, char **argv);	
} MonitorCommand[] = { 	
		{"alias",	&Monitor_ALIAS},
		{"br",		&Monitor_BR},
		{"cfrd",        &Monitor_CFRD},
		{"cfri",        &Monitor_CFRI},
		{"cfrm",        &Monitor_CFRM},
		{"de",		&Monitor_DE},
		{"dl",		&Monitor_DL},
		{"di",		&Monitor_DI},
		{"dn",		&Monitor_DN},
		{"go",		&Monitor_GO},
		{"md",          &Monitor_MD},
		{"mm.w",	&Monitor_MM},
		{"mm.l",	&Monitor_MM},
		{"prd",		&Monitor_PRD},
		{"trace",	&Monitor_TRACE},
		{"step",	&Monitor_TRACE},
		{"tracer",	&monitor_tracer},
		{"rd",		&Monitor_RD},
		{"reset",       &Monitor_RESET},  /* Eventually: Soft boot */
		{"reboot",      &Monitor_RESET},  /* Eventually: Hard boot */
		{"rm",		&Monitor_RM},
		{"set",		&Monitor_SET},
		{"env",		&Monitor_SET},
		{"ss",          &Monitor_SS},
		{"stat",	&Monitor_SS},
		{"stats",	&Monitor_SS},
		{"t",           &Monitor_TIME},
		{"time",        &Monitor_TIME},

		{"help",	&Monitor_HELP},
		{"?",		&Monitor_HELP},
		{"ver",         &Monitor_HELP_PrintVersion},  /* Cover all the bases here  :) */
		{"v",           &Monitor_HELP_PrintVersion},
		{"quit",	&Monitor_QUIT},
		{"q",		&Monitor_QUIT},
		{"exit",	&Monitor_QUIT},
		{"bye",		&Monitor_QUIT},
		{NULL, NULL} };

#ifdef HAVE_LIBREADLINE
extern unsigned char* readline(char *prompt);
extern void add_history(unsigned char *line);
#endif

void Monitor_Entry(void)
{
#ifdef HAVE_LIBREADLINE
	char *input;
#else 
	char input[128];
#endif
	int x, Result=0;
	int argc;
	char *argv[8];
	
	while(1) {
#ifdef HAVE_LIBREADLINE
                input=(char *)readline("dBug> ");
		if(!input) {
			/* End of input from command line */
			Run_Exit = 1;
			break;
		}
		
		if(input[0]) add_history((unsigned char *)input);
#else
		printf("dBug> ");
		input[0] = 0;
		fgets(input, 81, stdin);
#endif
		argc = arg_split(&argv[0], input, 8);
		if(argc == 0) continue;
/*		printf("%d args: ", argc);
		for(x=0;x<argc;x++)  printf("'%s' ", argv[x]);
		printf("\n");*/
		
		Result=-1;
		for(x=0;;x++) {
			if(MonitorCommand[x].cmd==NULL)
				break;
			if(strcasecmp(argv[0], MonitorCommand[x].cmd) == 0) {
				Result = (MonitorCommand[x].func)(argc,argv);
				break;
			}
		}
		if(Result == -1) {
			printf("Error:  Unknown Command.\n");
			Result=1;
		}
		/* This provides us with a way to exit, safely */	
		if(Result==0) 
			break;
	}
}



/* This provides an alternative for every exception */
void Monitor_HandleException(s16 Vector)
{
	char Buffer[128];
	char dump_info = 1;
	char enter_monitor = 1;
	struct _Address Source,Destination;
	u32 SValue, DValue;
	u32 current_PC;

	/* get the PC off the stack */
	Memory_Retr(&current_PC, 32, memory_core.a[7]+4);

	/* Restore the proper code from breakpoints */
	Monitor_BR_EnterException();

	TRACE("handling vector=%d\n", Vector);
	
	switch(Vector) {
	case 2: /* Illegal memory access */
		printf("Memory access out of bounds\n");
		Monitor_RD(1, NULL);
		/* Don't try to disassemble the instruction */
		dump_info=0;
		break;

	case 3: /* Address error */
		printf("Address Error: FS=4, Physical bus errror on instruction fetch\n");
		break;
	case 4: /* Illegal instruction */
		/* dBug uses ILLEGAL for breakpoints */
		Monitor_BR_Entry(Vector, &enter_monitor, &dump_info);
		break;
	case 8: /* Privilege violation */
		printf("Privilege violation\n");
		break;
	case 9: /* Trace interrupt */
		Monitor_TRACE_Entry(Vector, &enter_monitor, &dump_info);
		break;
	case 25: case 26: case 27: case 28: case 29: case 30: case 31: 
		/* Autovectored interrupt level 1-7 */
		printf("Autovector interrupt level %d\n", Vector - 24);
		break;
	case 47: /* Trap #15 */
		/* Trap function is in D0 */
		EA_GetFromPC(&Source, 32, 0, 0);
		EA_GetValue(&SValue, &Source);
		EA_GetFromPC(&Destination, 8, 0, 1);
		TRACE("TRAP #15: Function is 0x%02x\n", SValue);
		switch(SValue) {
		case 0: /* Return to Monitor */
			TRACE("TRAP #15:0x00: Return to Monitor\n");
			if(!Monitor_Initialized) {
				TRACE("First time into the monitor.  Initializing it...");
				Monitor_Init();
				Monitor_Initialized=1;
			}
			break;
		case 0x10: /* In Character */
			/* Get the character */
			TRACE("TRAP #15:0x10: InChar, Blocking waiting for a character...\n");
			DValue=serial_getch(1);
			TRACE("TRAP #15:0x10: Char=0x%02x(%c), storing it in D1\n", DValue, DValue);
			/* Put character in D1 */
			EA_PutValue(&Destination, DValue);
			enter_monitor = 0;
			break;
		case 0x13: /* Out Character */
			/* Retrive D1 */
			EA_GetValue(&DValue, &Destination);
			TRACE("TRAP #15:0x10: Put Char=0x%02x(%c)\n", DValue, DValue);
			TRACE("mbar=%08x\n", memory_core.mbar);
			serial_putch(1, DValue);
			enter_monitor = 0;
			break;
		default:
			printf("Unhandled TRAP #15 exception, func=0x%x\n", SValue);
			break;
		}
		/* Clobber the D0, the #15 command */
		EA_PutValue(&Source, 0);
		/* Don't dump any info */
		dump_info=0;
		break;
	default:
		printf("Unhandled exception, vector = 0x%x\n", Vector);
		break;
	}
	
	if(dump_info) {
		Monitor_RD(1, NULL);
		Monitor_InstructionDI(current_PC,Buffer);
		printf("%s\n", Buffer);
	}
	if(enter_monitor) {
		Monitor_Entry();
	}
	
	/* Restore breakpoints */
	Monitor_BR_ExitException();

}
