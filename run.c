/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/



#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

#include "coldfire.h"

char Run_Exit = 0;

TRACER_DEFAULT_CHANNEL(run);

static int stop_now = 0;

static void sigint(int sig)
{
#ifdef HAVE_LIBREADLINE
	char *input;
#else 
	char input[128];
#endif
	int argc;
	char *argv[8];
	extern struct _board_data board_data;
			void rd_dump_registers(u32 cpc, u32 csr);
	
	printf("execution suspended. \n");
			rd_dump_registers(memory_core.pc, GET_SR());
	printf("0. Exit emulator\n");
	printf("1. External software interrupt lev7 (return to monitor)\n");
	printf("2. Force external software interrupt (resets SP and lev7 ISR)\n");
	printf("3. TraceRun is currently %s, turn it %s.\n", 
		board_data.trace_run ? "ON" : "OFF",
		board_data.trace_run ? "OFF" : "ON" );
		
	

	printf("others <= do nothing, continue execution\n");
#ifdef HAVE_LIBREADLINE
	input=(char *)readline("suspend> ");
	if(!input) exit(1);
#else
	printf("suspend> ");
	fgets(input, 81, stdin);
#endif
	argc = arg_split(&argv[0], input, 8);
	if(argc) {
		switch(argv[0][0]) {
		case '`': case '~':
			exit(1);
			break;
		case '3':
			board_data.trace_run = !board_data.trace_run;
			break;

		case '2':
			/* Search for flash segment */
			/* set lev7 vector in vbr to point there */
			/* set sp to something useful reset sp? */
			/* fall through*/
		case '1':
			stop_now = 1;
			break;
		case '0':
			Run_Exit = 1;
			break;
		}
	}
	signal(SIGINT, sigint);
}

void Run(void)
{
	u32 Instr;
	struct _Instruction *InstructionPtr;
#ifdef INSTRUCTION_PROFILE        
	u64 LowTime=0, HighTime=0;
	char Buffer[16];
#endif        
	extern struct _board_data board_data;

	signal(SIGINT, sigint);
	//signal(SIGQUIT, sigint);
	
	while(!Run_Exit) {
			
		TRACE("New cycle, PC=0x%08lx, SP=0x%08lx\n", 
				memory_core.pc, memory_core.a[7]);
		
		/* Check for any pending exceptions */
		exception_check_and_handle();

		/* As we're coming back from an interrupt, check for exit */
		if(Run_Exit) break;

		/* Save the PC for the beginning of this instruction
		 *  This is useful for exceptions that reset the PC */
		memory_core.pc_instruction_begin = memory_core.pc;

		/* Before we execute this instruction, catch a bad PC counter */
		if(memory_core.pc % 2) {
			exception_do_exception(3);
			continue;
		}

		/* Get the instruction from memory */
		if(!Memory_RetrWord(&Instr, memory_core.pc)) continue;

		/* Look it up */
		InstructionPtr = Instruction_FindInstruction(Instr);

		if(InstructionPtr==NULL) {
			exception_do_exception(4);
			continue;
		} else {
			/* Run the instruction */

#ifdef INSTRUCTION_PROFILE        
                        LowTime=Profile_time_in_ms();
#endif        
			(*InstructionPtr->FunctionPtr)();

#ifdef INSTRUCTION_PROFILE
                        HighTime=Profile_time_in_ms();
#endif
		}

		/* FIXME: this causes the same instruction to be stopped on
		 * if we do a 'trace 1' after hitting a breakpoint.  
		 * The correct behaviour is to NEVER trace the first 
		 * instruction after returning from an exception.  */
		if(memory_core.sr.t) {
#ifdef INSTRUCTION_PROFILE
			Profile_MakeTimeString(Buffer, LowTime, HighTime);
			fprintf(stderr, "ExecTime: %ss\n", Buffer);
#endif
			exception_do_exception(9);
		}
		if(board_data.trace_run) {
			/* dump from 'rd' command */
			char buffer[256];
			void rd_dump_registers(u32 cpc, u32 csr);
			int Monitor_InstructionDI(s32 FromPC, char *buffer);

/*			printf("vbr: %08lx  mbar: %08lx\n", memory_core.vbr,
					memory_core.mbar);*/
			rd_dump_registers(memory_core.pc, GET_SR());
			Monitor_InstructionDI(memory_core.pc, buffer);
			printf("%s\n", buffer);
		}	

		if(stop_now) {
			if(stop_now == 1) {
				/* Send autovector interrupt 7 ,
				 *  (the black button on the Arnewsh board) */
				exception_do_exception(31);
			} else {
				/* Force autovector 7 */
				printf("Forcing Autovector Interrupt 7\n");
				exception_push_stack_frame(31);
				Monitor_HandleException(31);
				exception_restore_from_stack_frame();
			}
			stop_now = 0;
			/* fprintf(stderr, "Stopped at PC=0x%08lx, SP=0x%08lx\n", memory_core.pc, Areg[7]); 
			 * break; */
		}


		/* Now update anything that could cause an interrupt, so we
		 * can catch it in the next cycle */

		/* Call this, which will call an update
		 * for things like the UARTs and Timers */
		memory_update();
	}
}

