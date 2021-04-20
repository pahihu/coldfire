/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <string.h>

#include "tracer/tracer.h"

#include "config.h"
#include "configdefs.h"

/* Convenient types, ripped right out of the linux kernel */
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;


struct _Instruction {
	void (*FunctionPtr)(void);
	u16 Code;
	u16 Mask;
	s32 (*DIFunctionPtr)(char *Instruction, char *Arg1, char *Arg2);
};

/* This is ALWAYS cast into a s32word, so we need to pad it to a s32word */
struct _InstructionExtensionWord {
#ifndef WORDS_BIGENDIAN					
	signed Displacement:8;
	unsigned EV:1;
	unsigned Scale:2;
	unsigned WL:1;
	unsigned Register:3;
	unsigned AD:1;
	unsigned pad:16;
#else
	unsigned pad:16;
	unsigned AD:1;
	unsigned Register:3;
	unsigned WL:1;
	unsigned Scale:2;
	unsigned EV:1;
	signed Displacement:8;
#endif
};

enum {
	I_ADD, I_ADDA, I_ADDI, I_ADDQ, I_ADDX,
	I_AND, I_ANDI, I_ASL, I_ASR, I_BCC, I_BCHG, I_BCLR, I_BRA,
	I_BSET, I_BSR, I_BTST, I_CLR, I_CMP, I_CMPA, I_CMPI,
	I_DIVS, I_DIVL, I_DIVU, I_DIVUL, I_EOR, I_EORI, I_EXT, I_JMP, I_JSR,
	I_LEA, I_LINK, I_LSR, I_LSL, I_MOVE, I_MOVEC, I_MOVEA, I_MOVEM, I_MOVEQ,
	I_MOVETOSR, I_MULS, I_MULU, I_NEG, I_NEGX, I_NOP, I_NOT, I_OR,
	I_ORI, I_RTE, I_RTS, I_SCC, I_SUB, I_SUBA, I_SUBI, I_SUBQ, I_SUBX,
	I_SWAP, I_TRAP, I_TRAPF, I_TST, I_UNLK,

	I_LAST
};

enum _coldfire_cpu_id {
	CF_NULL, 
	CF_5206,
	CF_5206e,
	CF_5276, 
	CF_5307,
	CF_5407,
	CF_LAST
};

	
#define INSTRUCTION_(I,A) \
	typedef union _##I##_instr {			\
		struct _##I##_bits {			\
				A;			\
			} Bits;				\
			u32 Code;		\
		} I##_Instr


#ifndef WORDS_BIGENDIAN					
	#define INSTRUCTION_1ARG(I,A1,S1)			\
		INSTRUCTION_(I,	A1:S1)

	#define INSTRUCTION_2ARGS(I,A1,S1,A2,S2)			\
		INSTRUCTION_(I,	A2:S2; A1:S1)

	#define INSTRUCTION_3ARGS(I,A1,S1,A2,S2,A3,S3)			\
		INSTRUCTION_(I,	A3:S3; A2:S2; A1:S1)

	#define INSTRUCTION_4ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4)		\
		INSTRUCTION_(I,	A4:S4; A3:S3; A2:S2; A1:S1)

	#define INSTRUCTION_5ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4,A5,S5)	\
		INSTRUCTION_(I,	A5:S5; A4:S4; A3:S3; A2:S2; A1:S1)

	#define INSTRUCTION_6ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4,A5,S5,A6,S6) \
		INSTRUCTION_(I, A6:S6; A5:S5; A4:S4; A3:S3; A2:S2; A1:S1)

	#define INSTRUCTION_7ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4,A5,S5,A6,S6,A7,S7) \
		INSTRUCTION_(I,	A7:S7; A6:S6; A5:S5; A4:S4; A3:S3; A2:S2; A1:S1)

#else							
	#define INSTRUCTION_1ARG(I,A1,S1)			\
		INSTRUCTION_(I,	unsigned pad:(32-S1);		\
				A1:S1)

	#define INSTRUCTION_2ARGS(I,A1,S1,A2,S2)			\
		INSTRUCTION_(I,	unsigned pad:(32-S1-S2);		\
				A1:S1; A2:S2)

	#define INSTRUCTION_3ARGS(I,A1,S1,A2,S2,A3,S3)			\
		INSTRUCTION_(I,	unsigned pad:(32-S1-S2-S3); 		\
				A1:S1; A2:S2; A3:S3)

	#define INSTRUCTION_4ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4)		\
		INSTRUCTION_(I,	unsigned pad:(32-S1-S2-S3-S4); 		\
				A1:S1; A2:S2; A3:S3; A4:S4)

	#define INSTRUCTION_5ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4,A5,S5);	\
		INSTRUCTION_(I,	unsigned pad:(32-S1-S2-S3-S4-S5); 	\
				A1:S1; A2:S2; A3:S3; A4:S4; A5:S5)

	#define INSTRUCTION_6ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4,A5,S5,A6,S6)\
		INSTRUCTION_(I,	unsigned pad:(32-S1-S2-S3-S4-S5-S6); 	\
				A1:S1; A2:S2; A3:S3; A4:S4; A5:S5; A6:S6)

	#define INSTRUCTION_7ARGS(I,A1,S1,A2,S2,A3,S3,A4,S4,A5,S5,A6,S6,A7,S7)	\
		INSTRUCTION_(I,	unsigned pad:(32-S1-S2-S3-S4-S5-S6-S7); 	\
				A1:S1; A2:S2; A3:S3; A4:S4; A5:S5; A6:S6; A7:S7)
#endif	


#include "memory.h"
#include "addressing.h"
#include "monitor/monitor.h"
	
#include "isa_a/isa_a.h"
#include "isa_b/isa_b.h"
#include "peripherals/peripherals.h"		

/* board.c -- definitions of various eval board layouts */
struct _board_data {
	char *cpu_id;
	u32 clock_speed;
	u32 cycle_count;
	u32 total_cycle_count;
	char use_timer_hack;
	char trace_run;
	enum _coldfire_cpu_id cpu;
};

void board_init(void);
void board_reset(void);
void board_fini(void);
void board_setup(char *file);
struct _board_data *board_get_data(void);

/* cycle.c */
void cycle(u32 number);
int cycle_EA(s16 reg, s16 mode);

/* exception.c -- exception generators */
s32 exception_do_raw_exception(s16 vector);
s32 exception_do_exception(s16 vector);
void exception_restore_from_stack_frame(void);
void exception_push_stack_frame(s16 vector);
void exception_post(u32 interrupt_level, 
		u32 (*func)(u32 interrupt_level) );
void exception_withdraw(u32 interrupt_level);
void exception_check_and_handle(void);

/* handlers.c -- misc functions */
void SR_Set(s16 Instr, s32 Source, s32 Destination, s32 Result);

/* i.c -- instructions */
void Instruction_Init(void);
void instruction_register(u16 code, u16 mask, 
		void (*execute)(void),
		s32 (*disassemble)(char *, char *, char *));
void Instruction_DeInit(void);
struct _Instruction *Instruction_FindInstruction(u16 Instr);
void instruction_register_instructions(void);

/* misc.c -- Misc functions */
u32 arg_split(char **argv, char *buffer, int max_args);
u32 arg_split_chars(char **argv, char *buffer, int max_args, char *split);

/* network.c -- Network functions */
int network_setup_on_port(int *fd, u16 port);
int network_check_accept(int *fd);
int network_accept(int *server_fd, int *client_fd);

/* run.c -- Running the core */
extern char Run_Exit;
void Run(void);

/* sim.c -- System Integration Module */
struct _sim_register {
	char *name;
	s32 offset;
	char width;
	char read;
	char write;
	s32 resetvalue;
	char *description;
};

struct _sim {
	/* These are for peripherals to talk to the sim */
	void (*interrupt_assert)(s16 number, s16 vector);
	void (*interrupt_withdraw)(s16 number);
	/* These are for the monitor to query SIM registers */
	struct _sim_register *(*register_lookup_by_offset)(s32 offset);
	struct _sim_register *(*register_lookup_by_name)(char *name);
};
void sim_register(struct _sim *sim_data);
extern struct _sim *sim;


/* INSTRUCTION TIMING is firewalled inside profile.h */
#include "profile.h"
/* MEMORY_STATS is firewalled inside stats.h */
#include "stats.h"

