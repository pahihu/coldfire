/**********************************/
/*                                  */
/*  Copyright 2000, David Grant     */
/*                                  */
/*  see ../LICENSE for more details */
/*                                  */
/**********************************/

/* Function calls the outside world can know about */
/*void Monitor_Init(void);*/
void Monitor_Entry(void);
void Monitor_HandleException(s16 Vector);

/* Functions the outside world shouldn't call */

/* Notes:
 *    - Return values of 1 indicate "stay in monitor"
 *    - Values of 0 indicate, return from exception
 */

int Monitor_ALIAS(int argc, char **argv);
int Monitor_BR(int argc, char **argv);
void Monitor_BR_EnterException(void);
void Monitor_BR_ExitException(void);
void Monitor_BR_Entry(s16 Vector, char *enter_monitor, char *dump_info);
int Monitor_CFRM(int argc, char **argv);
int Monitor_CFRM(int argc, char **argv);
int Monitor_CFRD(int argc, char **argv);
int Monitor_CFRI(int argc, char **argv);
int Monitor_SET(int argc, char **argv);
int Monitor_SS (int argc, char **argv);
int Monitor_DI(int argc, char **argv);
int Monitor_DE(int argc, char **argv);
int Monitor_DL(int argc, char **argv);
int Monitor_DN(int argc, char **argv);
int Monitor_GO(int argc, char **argv);
int Monitor_HELP(int argc, char **argv);
int Monitor_show_help(char *cmd);
int Monitor_HELP_PrintVersion(int argc, char **argv);
int Monitor_InstructionDI(s32 FromPC, char *Buffer);
int Monitor_MD(int argc, char **argv);
int Monitor_MM(int argc, char **argv);
int Monitor_PRD(int argc, char **argv);
int Monitor_QUIT(int argc, char **argv);
int Monitor_RD(int argc, char **argv);
int Monitor_RM(int argc, char **argv);
int Monitor_TRACE(int argc, char **argv);
void Monitor_TRACE_Entry(s16 Vector, char *enter_monitor, char *dump_info);
int PrintVersion(int argc, char **argv);
int Monitor_TIME(int argc, char **argv);
int Monitor_RESET(int argc, char **argv);

int monitor_tracer(int argc, char **argv);

/* Data for the monitor */
struct _monitor_config {
	int disassemble_lines;
	char dbug_compatibility;
};

extern struct _monitor_config monitor_config;


