/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(timer);

/* TMR (timer mode register) :
 *  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
 * +-------------------------------+-------+---+---+---+-------+---+
 * |            Prescalar          | CE1-0 |OM |ORI|FRR|CLK1-0 |RST|
 * +-------------------------------+-------+---+---+---+-------+---+
 * */
struct _TMR {
	unsigned char PS; 	/* PS 7-0 */
	char CE; 		/* CE 1-0 */
	char OM;		/* OM */
	char ORI;		/* ORI */
	char FRR;		/* FRR */
	char CLK;		/* CLK 1-0 */
	char RST;		/* RST */
};

/* TER (timer event register) :
 *   7   6   5   4   3   2   1   0
 * +-----------------------+---+---+
 * |       Reserved        |REF|CAP|
 * +-----------------------+---+---+
 * */

struct _TER {
	char REF;
	char CAP;
};


struct _timer_data {
	struct _TMR TMR;
	u16 TRR;
	u16 TCR;
	u16 TCN;
	struct _TER TER;
	char enable;
	u32 cycles_per_tick;
	u32 next_tick;
};



static void timer_setup(struct _memory_segment *s);
static void timer_fini(struct _memory_segment *s);
static char timer_read(struct _memory_segment *s, u32 *result, s16 size, u32 offset);
static char timer_write(struct _memory_segment *s, s16 size, u32 offset, u32 value);
static void timer_reset(struct _memory_segment *s);
static void timer_update(struct _memory_segment *s);

void timer_5206_init(void)
{
	memory_module_register("timer_5206", &timer_setup);
}

static void timer_setup(struct _memory_segment *s)
{
	struct _timer_data *timer;

	if(s->mask != 0 && s->mask != 0xFFFFFFE0) {
		printf("warning: %s length must be 0x1F, not 0x%08x. reset.\n",
				s->name, ~s->mask);
	}
	s->mask = 0xFFFFFFE0;
	s->fini = &timer_fini;
	s->read = &timer_read;
	s->write = &timer_write;
	s->reset = &timer_reset;
	s->update = &timer_update;

	if(s->interrupt_line == 0) {
		/* timer1 - 9
		 * timer2 - 10 */
		if(s->name[strlen(s->name) - 1] == '1') {
			s->interrupt_line = 9;
		} else {
			s->interrupt_line = 10;
		}
		ERR("Interrupt line for %s not set, defaulting to %d\n", 
				s->name, s->interrupt_line);
	}

	timer = malloc(sizeof(struct _timer_data));
	memset(timer, 0, sizeof(struct _timer_data));
	s->data = timer;
}

static void timer_fini(struct _memory_segment *s)
{
/*	struct _timer_data *timer = (struct _timer_data *)s->data;*/
	free(s->name);
	free(s->data);
}

static void timer_reset(struct _memory_segment *s)
{
	struct _timer_data *timer = (struct _timer_data *)s->data;
	memset(&timer->TMR, 0, sizeof(struct _TMR));
	timer->TRR = 0xffff;
	timer->TCR = 0;
	timer->TCN = 0;
	memset(&timer->TER, 0, sizeof(struct _TER));
	timer->enable=0;
	timer->cycles_per_tick = 1;
	timer->next_tick = 0;
	/* Leave timer_interrupt_number alone :) */
}

static void timer_update(struct _memory_segment *s)
{
	/* This routine is in charge of asserting, or withdrawing the 
	 * serial interrupt */
	/* FIXME: use the global tick counter, not ++, because the global
	 * tick counter is incremented according to each instruction's 
	 * execution time. */
	struct _timer_data *timer = (struct _timer_data *)s->data;
	extern struct _board_data board_data;

/*	TRACE("timer%d: Updating timer\n", timerNumber+1);*/

	if(!timer->enable) return;

	if(board_data.use_timer_hack) {		
		/* Hack to make things work right, ignore the prescalar and the
		 * CLK setting and the elapsed cycles, just increment 
		 * the TRR every update */
	} else {
		/* Proper timing for the timer */
		/* If we're not ready to tick, we should at least
		 *  check interrupts, because we may need to withdraw
		 *  an interrupt */
		if(board_data.cycle_count < timer->next_tick) 
			goto check_interrupts;
		timer->next_tick += timer->cycles_per_tick;
	}
	

	/* From the docs, the reference isn't matched until the TCN==TRR, 
	 * AND the TCN is ready to increment again, so we'll hold off
	 * incrementing the TCN until we compare it to the TRR */

	if(timer->TCN == timer->TRR) {
		/* timer reference hit */
		TRACE("         %s: TCN == TRR = %ld\n", s->name, timer->TRR);
		timer->TER.REF=1;
		if(timer->TMR.FRR) {
			TRACE("         %s: Restart flag is on, restarting the timer\n", s->name);
			/* Restart the timer */
			timer->TCN=0;
		}
	} else {
		/* Ensure the interupt condition is off */
		timer->TER.REF=0;
	}
	
	/* Now increment the TCN
	 * FIXME: maybe we shouldn't do this if we just reset the TCN? */
	timer->TCN++;

check_interrupts:

	/* If the timer is at its reference, and ORI is set, then interrupt */
	if(timer->TER.REF && timer->TMR.ORI) {
		TRACE("          %s: timer reference condition exists\n", s->name);
		goto interrupts_on;
	}

	/* If we get here, we couln't find a condition to turn/leave
		the interrupts on , so turn 'em off */
/*	TRACE("          No interrupt conditions, withdrawing any interrupt requests\n");*/
	sim->interrupt_withdraw(s->interrupt_line);
	return;

interrupts_on:
	TRACE("          Posting interrupt request for %s\n", s->name);
	sim->interrupt_assert(s->interrupt_line, 0);
	return;
	

}


/* Handles hardware writes to the serial ports */
static char timer_write(struct _memory_segment *s, s16 size, 
				u32 offset, u32 value)
{
	struct _timer_data *timer = (struct _timer_data *)s->data;
	struct _board_data *b = board_get_data();

	TRACE("%s: size=%d, offset=0x%04lx, value=0x%08lx\n", s->name, size, offset, value);

	switch(offset) {
	case 0x0000: /* timer Mode Register (TMR) */
		/* Before writing, check for a 1->0 transition on reset */
		TRACE("   Setting timer Mode Register (TMR)\n");
		if((timer->TMR.RST==1) && (value & 0x0001)==0) {
			timer_reset(s);
		}
		timer->TMR.PS = (value & 0xFF00) >> 8;
		timer->TMR.CE = (value & 0x00C0) >> 6;
		timer->TMR.OM = (value & 0x0020) ? 1 : 0;
		timer->TMR.ORI= (value & 0x0010) ? 1 : 0;
		timer->TMR.FRR= (value & 0x0008) ? 1 : 0;
		timer->TMR.CLK= (value & 0x0006) >> 1;
		timer->TMR.RST= (value & 0x0001) ;
		
		TRACE("      PS=0x%02x, CE=%d, OM=%d, ORI=%d\n", timer->TMR.PS, timer->TMR.CE, timer->TMR.OM, timer->TMR.ORI);
		TRACE("      FRR=%d, CLK=%d, RST=%d\n", timer->TMR.FRR, timer->TMR.CLK, timer->TMR.RST);

		/* Recompute the cycles_per_tick */
		if(timer->TMR.CLK==1) {
			timer->cycles_per_tick = (timer->TMR.PS + 1);
		} else if(timer->TMR.CLK==2) {
			timer->cycles_per_tick = (timer->TMR.PS + 1) * 16;
		} else {
			timer->cycles_per_tick = 0;
		}
		TRACE("   cycles per tick=%ld\n", timer->cycles_per_tick);
		
		/* See if the timer should be on or off */
		/* If RST is 0, values can still be written, but clocking 
		 *  doesn't happen (1->0) transition resets the timer */
		if(timer->TMR.CLK == 0 || timer->TMR.CLK == 3 
				|| timer->TMR.RST==0) {
			/* timer is disabled */
			timer->enable=0;
		} else {
			timer->enable=1;
			timer->next_tick = b->cycle_count + 
					timer->cycles_per_tick;
		}
	
		break;
	case 0x0004: /* timer Reference Register  (TRR) */
		timer->TRR = (u16)value;
		TRACE("   Setting timer Reference Register (TRR)\n");
		break;
	case 0x0008: /* timer Capture Register (TCR) */
		timer->TCR = (u16)value;
		TRACE("   Setting timer Capture Register (TCR)\n");
		break;
	case 0x000C: /* timer Counter (TCN) */
		timer->TCN = (u16)value;
		TRACE("   Setting timer Counter (TCN)\n");
		break;
	case 0x0011: /* timer Event Register (TER) */
		TRACE("   Setting timer Event Register (TER)\n");
		if(value & 0x2) {
			timer->TER.REF = 0;
			TRACE("      Clearing Output Reference Event\n");
		}
		if(value & 0x1) {
			timer->TER.CAP = 0;
			TRACE("      Clearing Capture Event\n");
		}
		/* INterrupts status will be updated on the next
		 * Tick() call */
		break;
	default:
		return 0;
	}
	
	return 1;
}


static char timer_read(struct _memory_segment *s, u32 *result, 
					s16 size, u32 offset)
{
	struct _timer_data *timer = (struct _timer_data *)s->data;
	
	TRACE("%s: size=%d, offset=0x%04lx\n", s->name, size, offset);

	switch(offset) {
	case 0x0000: /* timer Mode Register (TMR) */
		*result =
			(((u32)timer->TMR.PS         ) << 8) |
			(((u32)timer->TMR.CE & 0x0002) << 6) |
			(timer->TMR.OM ? 0x00000020 : 0x0) |
			(timer->TMR.ORI ? 0x00000010 : 0x0) |
			(timer->TMR.FRR ? 0x00000008 : 0x0) |
			(((u32)timer->TMR.CLK & 0x0002) << 1) |
			(timer->TMR.RST ? 0x00000001 : 0x0);
			
		TRACE("   Retrieving timer Mode Register (TMR)\n");
		break;
	case 0x0004: /* timer Reference Register  (TRR) */
		*result = timer->TRR;
		TRACE("   Retrieving timer Reference Register (TRR)\n");
		break;
	case 0x0008: /* timer Capture Register (TCR) */
		*result = timer->TCR;
		TRACE("   Retrieving timer Capture Register (TCR)\n");
		break;
	case 0x000C: /* timer Counter (TCN) */
		*result = timer->TCN;
		TRACE("   Retrieving timer Counter (TCN)\n");
		break;
	case 0x0011: /* timer Event Register (TER) */
		*result =
			(timer->TER.REF ? 0x00000002 : 0x0) |
			(timer->TER.CAP ? 0x00000001 : 0x0);
		TRACE("   Retrieving timer Event Register (TER)\n");
		break;
	default:
		return 0;
	}

	TRACE("   Returning 0x%08lx\n", *result);
	
	return 1;
}

