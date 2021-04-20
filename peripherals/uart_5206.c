/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/


/* NOTES:
	- TxEMP interrrupts untested.  Use TxRDY for interrupts for 
		guaranteed proper operation.
	
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(serial);

/* UMR1 -- Mode Register 1
 *    7     6    5   4   3   2   1   0
 * +-----+-----+---+-------+---+-------+
 * |RxRTS|RxIRQ|ERR| PM1-0 |PT |B/C1-0 |
 * +-----+-----+---+-------+---+-------+
 */

struct _UMR1 {
	u8 RxRTS;
	u8 RxIRQ;
	u8 ER;
	u8 PM;
	u8 PT;
	u8 BC;
};

/* UMR2 -- Mode Register 2
 *   7   6    5     4    3   2   1   0
 * +-------+-----+-----+---------------+
 * | CM1-0 |TxRTS|TxCTS|    SB3-0      |
 * +-------+-----+-----+---------------+
 */
struct _UMR2 {
	u8 CM;
	u8 TxRTS;
	u8 TxCTS;
	u8 SB;
};


/* USR -- Status Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | RB  | FE  | PE  | OE  |TxEMP|TxRDY|FFULL|RxRDY|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _USR {
	u8 RB;
	u8 FE;
	u8 PE;
	u8 OE;
	u8 TxEMP;
	u8 TxRDY;
	u8 FFULL;
	u8 RxRDY;
};


/* UCSR -- Clock Select Register
 *    7     6     5     4     3     2     1     0
 * +-----------------------+-----------------------+
 * |        RCS3-0         |        TCS3-0         |
 * +-----------------------+-----------------------+
 */
struct _UCSR {
	u8 RCS;
	u8 TCS;
};


/* UCR -- Command Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----------------+-----------+-----------+
 * | --- |     MISC2-0     |   TC1-0   |   RC1-0   |
 * +-----+-----------------+-----------+-----------+
 */
struct _UCR {
	u8 MISC;
	u8 TC;
	u8 RC;
};

/* UIPCR - Input Port Change Register 
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * |  0  |  0  |  0  | COS |  1  |  1  |  1  | CTS |
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _UIPCR {
	u8 COS;
	u8 CTS;
};


/* UISR - Interrupt Status Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | COS |  -  |  -  |  -  |  -  |  DB |RxRDY|TxRDY|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _UISR {
	u8 COS;
	u8 DB;
	u8 RxRDY;
	u8 TxRDY;
};


/* UIMR - Interrupt Mask Register
 *    7     6     5     4     3     2     1     0
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * | COS |  -  |  -  |  -  |  -  |  DB |FFULL|TxRDY|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 */
struct _UIMR {
	u8 COS;
	u8 DB;
	u8 FFULL;
	u8 TxRDY;
};


struct _uart_data {
	/* Read */
	struct _UMR1 UMR1;
	struct _UMR2 UMR2;
	u8 UMR_pointer;
	struct _USR USR;
	char URB[3];	/* Recieve Buffer */
	u8 URB_count;
	struct _UIPCR UIPCR;
	struct _UISR UISR;
	u8 UBG1;
	u8 UBG2;
	u8 UIVR;
//	u8 UIP; /* Don't need this, can figure it out from the fd state */
	
	/* Write */
	struct _UCSR UCSR;
	struct _UCR UCR; /* We dont' need to save this, but thisi is a good
			  * place to put the register for decoding */
	char UTB;	/* Transmit Buffer */
	char UACR;
	struct _UIMR UIMR;

	/* Connections and threads */
	char transmitter_enabled;
	char receiver_enabled;
	int fd;

	pthread_t tid;
	pthread_mutex_t lock;
	int port;

};

static void serial_setup(struct _memory_segment *s);
static void serial_fini(struct _memory_segment *s);
static void serial_reset(struct _memory_segment *s);
static void serial_update(struct _memory_segment *s);
static char serial_read(struct _memory_segment *s, u32 *result, 
					s16 size, u32 offset);
static char serial_write(struct _memory_segment *s, s16 size, 
				u32 offset, u32 value);



void serial_5206_init(void)
{
	memory_module_register("uart_5206", &serial_setup);
}

static void *serial_thread(void *arg)
{
	struct _memory_segment *s = arg;
	struct _uart_data *uart = s->data;
	int status;
	int bind_fd;

	TRACE("Serial Thread for %s starting...\n", s->name);
		
	/* Setup the socket and wait */
	status = network_setup_on_port(&bind_fd, uart->port);

	while(1) {

		/* We're not clear to send */
		uart->UIPCR.CTS = 1;
		uart->UIPCR.COS = 0;

		/* Block on Accept */
		status = network_accept(&bind_fd, &uart->fd);
		
		/* fd_connected */
		send(uart->fd, s->name, 5, 0);
		send(uart->fd, "\r\n", 2, 0);

		uart->UIPCR.CTS = 0;
		uart->UIPCR.COS = 1;

		while(1) {
			u8 c;
			status = read(uart->fd, &c, 1);

			/* If status == 0, that's EOF */
			
			if(status <= 0) break;
//			if(status == -1 || c==0) goto interrupt_update;
			if(c==0) continue; //goto interrupt_update;
			if(c==0xff) {
				/* Escape sequence */
				status = recv(uart->fd, &c, 1, 0);
				status = recv(uart->fd, &c, 1, 0);
				continue;
//				goto interrupt_update;
			}

			TRACE("%s: Got character %d[%c]\n", s->name, c, c);
	
			pthread_mutex_lock(&uart->lock);
			/* If URB has room, store the character */
			if(uart->URB_count < 3) {
				uart->URB[(int)uart->URB_count]=c;
				uart->URB_count++;
			} else {
				ERR("%s: URB_count just overflowed!\n", s->name);
			}

			/* There's something in the buffer now */
			uart->USR.RxRDY = 1;
			if(uart->URB_count == 3) uart->USR.FFULL=1;

			if(uart->UMR1.RxIRQ==0)
				uart->UISR.RxRDY = uart->USR.RxRDY;
			else
				uart->UISR.RxRDY = uart->USR.FFULL;
			pthread_mutex_unlock(&uart->lock);
			
		}
		
		close(uart->fd);
		uart->fd = 0;
	}
		
	return NULL;
}


static u32 *default_uart_base_reg = NULL;
static u32 default_uart_base = 0;
static int default_port = 5206;

static void serial_setup(struct _memory_segment *s)
{
	struct _uart_data *uart;
	int res;
	
	if(s->mask != 0 && s->mask != 0xFFFFFFC0) {
		printf("warning: %s length must be 0x3F, not 0x%08x. reset.\n",
				s->name, ~s->mask);
	}
	s->mask = 0xFFFFFFC0;
	s->fini = &serial_fini;
	s->read = &serial_read;
	s->write = &serial_write;
	s->reset = &serial_reset;
	s->update = &serial_update;

	if(s->interrupt_line == 0) {
		/* uart1 - 12
		 * uart2 - 13 */
		if(s->name[strlen(s->name) - 1] == '1') {
			s->interrupt_line = 12;
		} else {
			s->interrupt_line = 13;
		}
		ERR("Interrupt line for %s not set, defaulting to %d\n", 
				s->name, s->interrupt_line);
	}
	
	uart = malloc(sizeof(struct _uart_data));
	memset(uart, 0, sizeof(struct _uart_data));
	s->data = uart;

	uart->port = default_port++;
	uart->fd = 0;
	printf("(on port %d)", uart->port);
	
	/* Setup a bind for the port */
//	uart->fd_connected = 0;
//	while(-1 == network_setup_on_port(&uart->fd, port)) port++;
//	printf("(on port %d)", port);

	/* Ensure we have a default serial segment, it will be the one
	 *  marked with code=xx, or the first segment we find. */
	if(s->code_len || default_uart_base_reg == NULL) {
		default_uart_base_reg = s->base_register;
		default_uart_base = s->base;
	}

	pthread_mutex_init(&uart->lock, NULL);

	
	/* Start the thread */
	res = pthread_create(&uart->tid, NULL, &serial_thread, s);
	
}

static void serial_fini(struct _memory_segment *s)
{
/*	struct _uart_data *uart = (struct _uart_data *)s->data;*/
	free(s->name);
	free(s->data);
}


static void serial_reset(struct _memory_segment *s)
{
	struct _uart_data *uart = (struct _uart_data *)s->data;
	memset(&uart->UMR1, 0, sizeof(struct _UMR1));
	memset(&uart->UCR, 0, sizeof(struct _UCR));
	uart->URB[0]=0;
	uart->URB_count=0;
}

/* This is called on every tick */
static void serial_update(struct _memory_segment *s)
{
	/* This routine handles transmitting, receiving, and is
	 * in charge of asserting, or withdrawing the serial interrupt 
         */
	struct _uart_data *uart = (struct _uart_data *)s->data;
	int status;
	
/*	s16 Baud = (uart->UBG1 << 8) | uart->UBG2;*/

/*	TRACE("%s: update: connected=%d, transmitterenabled=%d, receiverenabled=%d\n",
		PortNumber, uart->fd_connected, uart->transmitter_enabled, uart->receiver_enabled);
*/
	if(!uart->fd) {
		/* If there is no connection, there's no chance we can
		 * do anything with this port */
		return;
	}
	
	/* Transmit stuff */
	if(uart->transmitter_enabled) {
		/* If TransmitterReady (TxRDY bit of Status Reg USR)
		 *  0 - Character waiting in UTB to send
		 *  1 - Transmitter UTB is empty, and ready to be loaded */
		if(uart->USR.TxRDY == 0) {
			/* We use the fact that send() buffers characters
			 * for us, so we don't need to shift anything
			 * into transmit buffers */ 
			TRACE("%s: Sending character %c(%d)\n", s->name, uart->UTB,uart->UTB);
			if(uart->fd) send(uart->fd, &uart->UTB, 1, 0);
			/* Signal that there is room in the buffer */
			uart->USR.TxRDY = 1;
			/* The UISR duplicates the USR for TxRDY */
			uart->UISR.TxRDY = 1;

			/* Now that the character is sent, set the buffer
			 * empty condition */
			uart->USR.TxEMP=1;
		}
	}

	
interrupt_update:
	pthread_mutex_lock(&uart->lock);
	/* Adjust the interrupt status */
	/* If the TX is masked on, and there is room in the buffer, we 
		want the interrupt on */
	if(uart->UIMR.TxRDY && uart->USR.TxRDY) {
		TRACE("%s: Transmit Interrupt Condition exists\n", s->name);
		goto interrupts_on;
	}

	/* If Rx is masked on, and we are interrupting on RxRDY, and 
		RxRDY is on, then interrupt */
	if(uart->UIMR.FFULL && !uart->UMR1.RxIRQ && uart->USR.RxRDY) {
		TRACE("%s: Recieve RxRDY interrupt Condition exists\n", s->name);
		goto interrupts_on;
	}
		
	/* If Rx is masked on, and we are interrupting on FFULL, and 
		FFULL is on, then interrupt */
	if(uart->UIMR.FFULL && uart->UMR1.RxIRQ && uart->USR.FFULL) {
		TRACE("%s: Recieve FFULL interrupt Condition exists\n", s->name);
		goto interrupts_on;
	}
		

	/* If we get here, we couln't find a condition to turn/leave
		the interrupts on , so turn 'em off */
/*	TRACE("%s: No interrupt conditions, withdrawing any interrupt requests\n", s->name);*/
	sim->interrupt_withdraw(s->interrupt_line);
	pthread_mutex_unlock(&uart->lock);
	return;

interrupts_on:
	TRACE("%s: Posting interrupt request for %s\n", s->name);
	sim->interrupt_assert(s->interrupt_line, uart->UIVR);
	pthread_mutex_unlock(&uart->lock);
	return;
	

}


/* Handles hardware writes to the serial ports */
static char serial_write(struct _memory_segment *s, s16 size, 
				u32 offset, u32 value)
{
	struct _uart_data *uart = (struct _uart_data *)s->data;

	/* We ASSUME that we are passed an offset that is valid for us, 
		and that MBAR has already been subtracted from it */

	TRACE("%s: size=%d, offset=0x%04lx, value=0x%08lx\n", s->name, size, offset, value);
	pthread_mutex_lock(&uart->lock);

	switch(offset) {
	case 0x0000: /* Mode Register (UMR1, UMR2) */
		if(uart->UMR_pointer==0) {
			uart->UMR1.RxRTS = (value & 0x80) ? 1 : 0;
			uart->UMR1.RxIRQ = (value & 0x40) ? 1 : 0;
			uart->UMR1.ER   = (value & 0x20) ? 1 : 0;
			uart->UMR1.PM = (value & 0x18) >> 3;
			uart->UMR1.PT = (value & 0x04) ? 1 : 0;
			uart->UMR1.BC = (value & 0x03);
			
			uart->UMR_pointer=1;
			TRACE("%s: Setting Mode Register 1 (UMR1)\n", s->name);
			TRACE("%s:    RxRTS=%d, RxIRQ=%d, ErrorMode(ERR)=%d, ParityMode(PM)=%d\n", s->name, uart->UMR1.RxRTS, uart->UMR1.RxIRQ, uart->UMR1.ER, uart->UMR1.PM);
			TRACE("%s:    ParityType(PT)=%d, BitsPerCharacter(BC)=%d\n", s->name, uart->UMR1.PT, uart->UMR1.BC);
		} else {
			uart->UMR2.CM = (value & 0xC0) >> 6;
			uart->UMR2.TxRTS = (value & 0x02) ? 1 : 0;
			uart->UMR2.TxCTS = (value & 0x01) ? 1 : 0;
			uart->UMR2.SB = (value & 0x0F);
			TRACE("%s: Setting Mode Register 2 (UMR2)\n", s->name);
			TRACE("%s:    CM=%d, TxRTS=%d, TxCTS=%d, SB=%d\n", s->name, 
					uart->UMR2.CM, uart->UMR2.TxRTS, 
					uart->UMR2.TxCTS, uart->UMR2.SB);
		}
		break;
	case 0x0004: /* Clock-Select Register (UCSR) */
		uart->UCSR.RCS = (value & 0xF0) >> 4;
		uart->UCSR.TCS = (value & 0x0F);
		TRACE("%s: Clock-Select Register (UCSR)\n", s->name);
		TRACE("%s:    RCS=%d, TCS=%d\n", s->name, 
				uart->UCSR.RCS, uart->UCSR.TCS); 
		break;
		
	case 0x0008: /* Command Register (UCR) */
		uart->UCR.MISC = (value & 0x70) >> 4;
		uart->UCR.TC = (value & 0x0C) >> 2;
		uart->UCR.RC = (value & 0x03);
		switch(uart->UCR.MISC) {
		case 0: /* No Command */
			break;
		case 1: /* Reset Mode Register Pointer */
			TRACE("   Resetting Mode Register Pointer\n");
			uart->UMR_pointer=0;
			break;
		case 2: /* Reset Receiver */
			TRACE("   Resetting Receiver\n");
			uart->receiver_enabled=0;
			uart->USR.FFULL=0;
			uart->USR.RxRDY=0;
			break;
		case 3: /* Reset Transmitter */
			TRACE("   Resetting Transmitter\n");
			uart->transmitter_enabled=0;
			uart->USR.TxEMP=0;
			uart->USR.TxRDY=0;
			uart->UISR.TxRDY=0;
/*				Serial_InterruptUpdate(PortNumber);*/
			break;
		case 4: /* Reset Error Status */
			TRACE("   Resetting Error Status\n");
			uart->USR.OE=0;
			uart->USR.FE=0;
			uart->USR.PE=0;
			uart->USR.RB=0;
			break;
		case 5: /* Reset Break-Change Interrupt */
			ERR("%s: Resetting Break-Change Interrupt (NOT IMPLEMENTED)\n", s->name);
			break;
		case 6: /* Start Break */
			ERR("%s: Setting Start Break (NOT IMPLEMENTED)\n", s->name);
			break;
		case 7: /* Stop Break */
			ERR("%s: Setting Stop Break (NOT IMPLEMENTED)\n", s->name);
			break;
		}

		switch(uart->UCR.TC) {
		case 0:	/* No action */
			break;
		case 1: /* Transmitter Enable */
			TRACE("   Enabling Transmitter\n");
			if(!uart->transmitter_enabled) {
				uart->transmitter_enabled=1;
				uart->USR.TxEMP=1;
				uart->USR.TxRDY=1;
				uart->UISR.TxRDY=1;
/*					Serial_InterruptUpdate(PortNumber);*/
			}
			break;
		case 2: /* Transmitter Disable */
			TRACE("   Disabling Transmitter\n");
			if(uart->transmitter_enabled) {
				uart->transmitter_enabled=0;
				uart->USR.TxEMP=0;
				uart->USR.TxRDY=0;
				uart->UISR.TxRDY=0;
/*					Serial_InterruptUpdate(PortNumber);*/
			}
			break;
		case 3: /* Do Not Use */
			ERR("%s: Accessed DO NOT USE bit for UCR:TC bits\n", s->name);
			break;
		}

		/* Receiver stuff */
		switch(uart->UCR.RC) {
		case 0:	/* No action */
			break;
		case 1: /* Receiver Enable */
			TRACE("   Enabling Receiver\n");
			if(!uart->receiver_enabled)
				uart->receiver_enabled=1;
			break;
		case 2: /* Receiver Disable */
			TRACE("   Disabling Receiver\n");
			if(uart->transmitter_enabled)
				uart->receiver_enabled=0;
			break;
		case 3: /* Do Not Use */
			ERR("%s: Accessed DO NOT USE bit for UCR:RC bits\n", s->name);
			break;
		}
		break;
		
	case 0x000C: /* Transmitter Buffer (UTB) */
		TRACE("   Transmitting character 0x%02x\n", value);
		uart->UTB = (char)value;

		/* A write to the UTB Clears the TxRDY bit */
		uart->USR.TxRDY=0;
		uart->UISR.TxRDY=0;

		/* Also ensure the empty bit is off, we just wrote
		 * to the transmitter, the buffer is not empty */
		uart->USR.TxEMP=0;

		break;

	case 0x0010: /* Auxiliary Control Register (UACR) */
		/* We can't interrupt on a change in the CTS line of 
		  * the serial port.. because.. well.. we're not connected
		  * to a real serial port.. so we ignore writes to
		  * this register, but print an error if someone
		  * tries to enable this */
		if( ((char)value) & 0x01) {
			/* Enable CTS interrupt */
			ERR("%s: Setting Auxiliary Control Register (UACR) "
				"IEC (Interrupt Enable Control) has no effect, "
				"because we don't have a CTS pin on the serial "
				"port to interrupt on.  Sorry.\n", s->name);
		}
		break;
	case 0x0014: /* Intrerrupt Mask Register (UIMR) */
		TRACE("   Setting Interrupt Mask Register (UMIR) to 0x%02x\n", value);
		uart->UIMR.COS = (value &0x80) ? 1 : 0;
 		uart->UIMR.DB =  (value &0x04) ? 1 : 0;
 		uart->UIMR.FFULL = (value &0x02) ? 1 : 0;
 		uart->UIMR.TxRDY = (value &0x01) ? 1 : 0;
		TRACE("   Change-of-State (COS) interrupt: %s\n", uart->UIMR.COS ? "enabled":"disabled");
		TRACE("   Delta Break (DB) interrupt: %s\n", uart->UIMR.DB ? "enabled":"disabled");
		TRACE("   FIFO Full (FFULL) interrupt: %s\n", uart->UIMR.FFULL ? "enabled":"disabled");
		TRACE("   Transmitter Ready (TxRDY) interrupt: %s\n", uart->UIMR.TxRDY ? "enabled":"disabled");
/*		Serial_InterruptUpdate(PortNumber);*/
		break;
	case 0x0018: /* Baud Rate Generator Prescale MSB (UBG1) */
		TRACE("   Baud Rate Generator Prescale MSB to 0x%02x\n", value);
		uart->UBG1 = (unsigned char)value;
		TRACE("   Baud Rate is %d\n", (uart->UBG1 << 8) + uart->UBG2);
		break;
	case 0x001C: /* Baud Rate Generator Prescale LSB (UBG2) */
		TRACE("   Baud Rage Generator Prescale LSB to 0x%02x\n", value);
		uart->UBG2 = (unsigned char)value;
		TRACE("   Baud Rate is %d\n", (uart->UBG1 << 8) + uart->UBG2);
		break;
	case 0x0030: /* Interrupt Vector Register (UIVR) */
		TRACE("   Setting Interrupt Vector Register to 0x%02x\n", value);
		uart->UIVR = (unsigned char)value;
		break;
	case 0x0034: /* NO NOT ACCESS */
		break;
	case 0x0038: /* Output Port Bit Set CMD (UOP1) */
		/* Since we don't have a direct connection to a sieral
		 * port, this does nothing */
		TRACE("%s: Set Output Port Bit (UOP1) (no effect)\n", s->name);
		break;
	case 0x003C: /* Output Port Bit Reset CMD (UOP0) */
		TRACE("%s: Reset Output Port Bit (UOP0) (no effect)\n", s->name);
		break;
	default:
		pthread_mutex_unlock(&uart->lock);
		return 0;

	}
	pthread_mutex_unlock(&uart->lock);
	return 1;
	
}



static char serial_read(struct _memory_segment *s, u32 *result, 
					s16 size, u32 offset)
{
	struct _uart_data *uart = (struct _uart_data *)s->data;
	/* We ASSUME that we are passed an offset that is valid for us, 
		and that MBAR has already been subtracted from it */

	TRACE("%s: size=%d, offset=0x%04lx\n", s->name, size, offset);

	*result=0;

	pthread_mutex_lock(&uart->lock);
	
	switch(offset) {
	case 0x0000: /* Mode Register (UMR1, UMR2) */
		break;
	case 0x0004: /* Status Register (USR) */
		*result = 
			(uart->USR.RB   ? 0x80 : 0x00) |
			(uart->USR.FE   ? 0x40 : 0x00) |
			(uart->USR.PE   ? 0x20 : 0x00) |
			(uart->USR.OE   ? 0x10 : 0x00) |
			(uart->USR.TxEMP? 0x08 : 0x00) |
			(uart->USR.TxRDY? 0x04 : 0x00) |
			(uart->USR.FFULL? 0x02 : 0x00) |
			(uart->USR.RxRDY? 0x01 : 0x00);
		break;

	case 0x0008: /* DO NOT ACCESS */
		return 0;

	case 0x000C: /* Receiver Buffer (URB) */
		*result = uart->URB[0];
		/* Shift the FIFO */
		uart->URB[0] = uart->URB[1];
		uart->URB[1] = uart->URB[2];

		/* See if we can decrement the fifo count */
		if(uart->URB_count == 0) {
			ERR("%s: underrun (read but no data in FIFO)\n", s->name);
			uart->URB_count=0;
		} else {
			uart->URB_count--;
		}
			

		/* Check to see if there is nothing left in the buffer */
		if(uart->URB_count==0)
			uart->USR.RxRDY = 0;

		/* The buffer cannot be full anymore, we just did a read */
		uart->USR.FFULL=0;

		/* Set the UISR */
		if(uart->UMR1.RxIRQ==0)
			uart->UISR.RxRDY = uart->USR.RxRDY;
		else
			uart->UISR.RxRDY = uart->USR.FFULL;

		/* Update the status of the serial intrrupt */
/*		Serial_InterruptUpdate(PortNumber);*/
		break;

	case 0x0010: /* Input Port Change Register (UIPCR) */
		*result = (uart->UIPCR.COS ? 0x40 : 0x00) |
			  (uart->UIPCR.CTS ? 0x01 : 0x00);
		/* Reading from this register should clear the 
		 * COS (change of state) if it was asserted */
		uart->UIPCR.COS = 0;
		break;
	case 0x0014: /* Interrupt Status Register (UISR) */
		*result = (uart->UISR.COS  ? 0x80 : 0x00) |
			  (uart->UISR.DB   ? 0x04 : 0x00) |
			  (uart->UISR.RxRDY? 0x02 : 0x00) |
			  (uart->UISR.TxRDY? 0x01 : 0x00);
		break;
	case 0x018: /* Baud Rate Generator Prescale MSB (UBG1) */
		*result = uart->UBG1;
		break;
	case 0x01C: /* Baud Rate Generator Prescale LSB (UBG2) */
		*result = uart->UBG2;
		break;
	case 0x0030: /* Interrupt Vector Register (UIVR) */
		*result = uart->UIVR;
		break;
	case 0x0034: /* Input Port Register (UIP) */
		/* Set to 0 (meaning nCTS=0, meaning we're Clear to Send) 
		 * if there is something connected */
		*result = (uart->fd != 0) ? 0 : 1;
		break;
	case 0x0038: /* NO NOT ACCESS */
	case 0x003C: /* NO NOT ACCESS */
	default: 
		ERR("%s: Unaligned access!\n",s->name);
		pthread_mutex_unlock(&uart->lock);
		return 0;

	}
	pthread_mutex_unlock(&uart->lock);
	TRACE("%s: result=0x%08lx\n", s->name, *result);
	return 1;
	
}


/* FIXME: We need to fix the Serial_getch and Serial_putch routines, 
 * so they use the serial port access rouintes above... The routines
 * below are for TRAP #15 */
char serial_getch(s16 port_number)
{
	u32 base = *default_uart_base_reg + default_uart_base;
	u32 usr;
	u32 c;
	
	/* Enable Receiver */
	Memory_StorByte(base + 0x08, 0x05);

	TRACE("default uart: Getting a character..\n");
	while(1) {
		Memory_RetrByte(&usr, base + 0x04);
		if(usr & 0x01) {
			/* Reveiver has 1 or more characters */
			Memory_RetrByte(&c, base + 0x0C);
			break;
		}
		memory_update();
	}
	TRACE("default uart: done, returning character %d[%c]\n", c, c);
	return (char)c;
}

void serial_putch(s16 port_number, char c)
{
	u32 base = *default_uart_base_reg + default_uart_base;
	u32 usr;

	/* Enable transmitter */
	Memory_StorByte(base + 0x08, 0x05);
	
	/* Wait for TxRDY to go low */
	TRACE("default uart: Directly putting character %d[%c]\n", c, c);
	while(1) {
		Memory_RetrByte(&usr, base + 0x04);
		if(usr & 0x04) {
			Memory_StorByte(base + 0x0C, c);
			break;
		}
		memory_update();
	}
/*Serial_Stor(8, 0x014C + (0x40*PortNumber), c);*/
/*	send(Serialuart[PortNumber].fd, &c, 1, 0);*/
	TRACE("default uart: done\n");
}

