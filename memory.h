/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#ifndef MEMORY_H
#define MEMORY_H


/* Memory definitions */

struct _sr {
	u8 p;
	u8 z;
	u8 n;
	u8 x;
	u8 v;
	u8 c;
	u8 t;
	u8 s;
	u8 m;
	u8 i;
};

struct _memory_core {
	u32 pc;
	u32 pc_instruction_begin;
	struct _sr sr;
	u32 sr_reset; 
	u32 mbar;
	u32 rambar;
	u32 rombar;
	u32 vbr;
	u32 cacr;
	u32 d[8];
	u32 a[8];
};

/* Actually declared in memory.c */
extern struct _memory_core memory_core;

#define GET_SR() ((u16)(memory_core.sr.p | memory_core.sr.z |  \
		 memory_core.sr.n | memory_core.sr.x |  \
		 memory_core.sr.v | memory_core.sr.c ) \
		|(((u16)(memory_core.sr.t | memory_core.sr.s | \
		  memory_core.sr.m | memory_core.sr.i))<<8) )  


#define SET_SR_CCR(V) { \
		memory_core.sr.p = (V) & SR_P; \
		memory_core.sr.z = (V) & SR_Z; \
		memory_core.sr.n = (V) & SR_N; \
		memory_core.sr.x = (V) & SR_X; \
		memory_core.sr.v = (V) & SR_V; \
		memory_core.sr.c = (V) & SR_C; \
			}
	
#define SET_SR(V) { \
		SET_SR_CCR(V); \
		memory_core.sr.t = ((V)>>8) & SR_T; \
		memory_core.sr.s = ((V)>>8) & SR_S; \
		memory_core.sr.m = ((V)>>8) & SR_M; \
		memory_core.sr.i = ((V)>>8) & SR_I; \
		  }

		

#if 0
struct _SR {
#ifndef WORDS_BIGENDIAN
	unsigned C:1;
	unsigned V:1;
	unsigned Z:1;
	unsigned N:1;
	unsigned X:1;
	unsigned Unused:3;
	unsigned InterruptPriorityMask:3;
	unsigned Unused2:1;
	unsigned M:1;
	unsigned S:1;
	unsigned Unused3:1;
	unsigned T:1;
	unsigned pad:16;
#else
	unsigned pad:16;
	unsigned T:1;
	unsigned Unused3:1;
	unsigned S:1;
	unsigned M:1;
	unsigned Unused2:1;
	unsigned InterruptPriorityMask:3;
	unsigned Unused:3;
	unsigned X:1;
	unsigned N:1;
	unsigned Z:1;
	unsigned V:1;
	unsigned C:1;
#endif
};

extern struct _SR *SRBits; 
#endif

void memory_reset(void);
void Memory_Init(void);
void Memory_DeInit(void);

/* Memory seek */
char memory_seek(u32 offset);


/* Memory retrive */
#define Memory_RetrLongWord(V,Offset) 	Memory_Retr(V,32,Offset)
#define Memory_RetrWord(V,Offset) 	Memory_Retr(V,16,Offset)
#define Memory_RetrByte(V,Offset) 	Memory_Retr(V,8,Offset)
char Memory_Retr(u32 *Result, s16 Size, s32 Offset);

/* Memory Store */
#define Memory_StorLongWord(Offset,Value) 	Memory_Stor(32,Offset,Value)
#define Memory_StorWord(Offset,Value) 		Memory_Stor(16,Offset,Value)
#define Memory_StorByte(Offset,Value) 		Memory_Stor(8,Offset,Value)
char Memory_Stor(s16 Size, s32 Offset, u32 Value);

/* Memory retrive, and update the PC */
#define Memory_RetrByteFromPC(V)	Memory_RetrFromPC(V,8)
#define Memory_RetrWordFromPC(V) 	Memory_RetrFromPC(V,16)
#define Memory_RetrLongWordFromPC(V) 	Memory_RetrFromPC(V,32)
char Memory_RetrFromPC(u32 *Result, s16 Size);


void memory_update(void);


enum {
	SR_C = 0x0001,
	SR_V = 0x0002,
	SR_Z = 0x0004,
	SR_N = 0x0008,
	SR_X = 0x0010,
	SR_P = 0x0080,
	SR_I = 0x0007,
	SR_M = 0x0010,
	SR_S = 0x0020,
	SR_T = 0x0080,
};


/* Memory modules */
struct _memory_segment {
	char *name;
	u32 base;
	u32 *base_register;
	u32 mask;
	u16 interrupt_line;
	char *code;
	u32 code_len;
	void (*fini)(struct _memory_segment *s);
	char (*read)(struct _memory_segment *s, u32 *result, s16 size, u32 offset);
	char (*write)(struct _memory_segment *s, s16 size, u32 offset, u32 value);
	void (*reset)(struct _memory_segment *s);
	void (*update)(struct _memory_segment *s);
	void *data;
	
};

struct _memory_module {
	char *name;
	void (*setup)(struct _memory_segment *s);
};


void memory_module_register(char *name, 
		void (*setup)(struct _memory_segment *s));
void memory_module_setup_segment(char *module_name, char *data);
struct _memory_segment *memory_find_segment_for(u32 offset);
void memory_core_set_reset_values(char *s);

void memory_dump_segments(void);
#endif
