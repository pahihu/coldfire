/**********************************/
/*                                */
/*  Copyright 2002, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/* ISA BUS emulation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*#define TRACER_OFF*/
#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(isa);

static void isa_setup(struct _memory_segment *s);
static void isa_fini(struct _memory_segment *s);
static char isa_read(struct _memory_segment *s, u32 *result, s16 size, u32 offset);
static char isa_write(struct _memory_segment *s, s16 size, u32 offset, u32 value);
static void isa_reset(struct _memory_segment *s);

void isa_init(void)
{
	memory_module_register("dummy", &isa_setup);
}

static void isa_setup(struct _memory_segment *s)
{
	/* Do some checks to make sure people don't try dumb things */
	if(s->base_register != NULL) {
		printf("warning: creating movable dummy area\n");
	}
	s->fini = &isa_fini;
	s->read = &isa_read;
	s->write = &isa_write;
	s->reset = &isa_reset;
	s->update = NULL;
	s->data = NULL;
}
	

static void isa_fini(struct _memory_segment *s)
{
	free(s->name);
	free(s->data);
}

static void isa_reset(struct _memory_segment *s)
{
}


static char isa_read(struct _memory_segment *s, u32 *result, 
					s16 size, u32 offset)
{
	*result = 0;
	return 1;
}

static char isa_write(struct _memory_segment *s, s16 size, 
			u32 offset, u32 value)
{
	return 1;

}

