 
#ifndef SIM5206_H
#define SIM5206_H

/* DRAM controller bank (0x000c bytes)*/
struct _dram_bank {
	u16	DCAR;		/* 00: DRAM Controller Address Register */
	u8	pad0[2];	/* 02: */
	u32	DCMR;		/* 04: DRAM Controller Mask Register */
	u8	pad[3];		/* 08: */
	u8	DCCR;		/* 0b: DRAM Controller Control Register */
};

/* DRAM controller (0x001e bytes)*/
struct _dram {
	u16	DCRR;		/* 00: DRAM Controller Refresh */
	u8	pad0[2];	/* 02: */
	u16	DCTR;		/* 04: DRAM Controller Timing Register */
	struct _dram_bank Bank0;	/* 06: Bank 0 */
	struct _dram_bank Bank1;	/* 12: Bank 1 */
};

/* One Bank for the The Chip Select module (0x000c bytes) */
struct _chip_select {
	u16	CSAR;		/* 00: Chip-Select Address Register */
	u8	pad0[2];	/* 02: */
	u32	CSMR;		/* 04: Chip-Select Address Mask Register */
	u8	pad1[2];	/* 08: */
	u16	CSCR;		/* 0a: Chip-Select Control Register */
};


/* Timer module (0x0020 bytes) */
struct _timer {
	u16	TMR;		/* 00: Timer Mode Register */
	u8	pad0[2];	/* 02: */
	u16	TRR;		/* 04: Timer Reference Register */
	u8	pad1[2];	/* 06: */
	u16	TCR;		/* 08: Timer Capture Register */
	u8	pad2[2];	/* 0a: */
	u16	TCN;		/* 0c: Timer Counter */
	u8	pad3[3];	/* 0e: */
	u8	TER;		/* 11: Timer Event Register */
	u8	pad4[0xe];	/* 12: */
};

/* M-BUS module (0x0014 bytes( */
struct _mbus {
	u8	MADR;		/* 00: M-BUS Address Register */
	u8	pad0[0x03];	/* 01: */
	u8	MFDR;		/* 04: M-BUS Frequency Divider Register */
	u8	pad1[0x03];	/* 05: */
  	u8	MBCR;		/* 08: M-BUS Control Register */
	u8	pad2[0x03];	/* 09: */
  	u8	MBSR;		/* 0c: M-BUS Status Register */
	u8	pad3[0x03];	/* 0d: */
  	u8	MBDR;		/* 10: M-BUS Data I/O Register */
	u8	pad4[0x03];	/* 11: */
};


#if 0

/* DMA structure, (0x0040 bytes) */
struct _dma {
	u32	SAR;		/* 00: DMA Source Address Register */
	u32	DAR;		/* 04: DMA Destination Address Register */
	u16	DCR;		/* 08: DMA Control Register */
	u8	pad0[0x02];	/* 0a: */
	u16	BCR;		/* 0c: DMA Byte Count Regsiter */
	u8	pad1[0x02];	/* 0e: */
	u8	DSR;		/* 10: DMA Status Register */
	u8	pad2[0x03];	/* 11: */
	u8	DIVR;		/* 14: DMA Interrupt Vector Register */
	u8	pad3[0x2B];	/* 15: */
};

#endif


struct _sim_5206 {
	/* MBar only needs 22 bits, we have 24 here, so we could store mbar
	 * in the first 3 bytes of the sim..... ah well.... */
	u8	pad00[0x03];	/* 000: */
	u8	SIMR;		/* 003: SIM Configuration Register */
	u8	pad01[0x10];	/* 004: */
	u8	ICR[13];	/* 014: Interrupt Control Register 1 - External IRQ1/IPL1 */
				/* 015: Interrupt Control Register 2 - External IPL2 */
				/* 016: Interrupt Control Register 3 - External IPL3 */
				/* 017: Interrupt Control Register 4 - External IRQ4/IPL4 */
				/* 018: Interrupt Control Register 5 - External IPL5 */
				/* 019: Interrupt Control Register 6 - External IPL6*/
				/* 01a: Interrupt Control Register 7 - External IRQ7/IPL7 */
				/* 01b: Interrupt Control Register 8 - SWT */
				/* 01c: Interrupt Control Register 9 - Timer1 Interrupt */
				/* 01d: Interrupt Control Register 10 - Timer2 Interrupt */
				/* 01e: Interrupt Control Register 11 - MBUS Interrupt */
				/* 01f: Interrupt Control Register 12 - UART1 Interrupt */
				/* 020: Interrupt Control Register 13 - UART2 Interrupt */
#if 0
	/* For the 5206e */
	u8	ICR14;		/* 021: Interrupt Control Register 14 - ??? */
	u8	ICR15;		/* 022: Interrupt Control Register 15 - ??? */
#else
	u8	pad02[0x02];	/* 021: */
#endif
	u8	pad03[0x13];	/* 023: */
	u16	IMR;		/* 036: Interrupt Mask Register */
	u8	pad04[0x02];	/* 038: */
	u16	IPR;		/* 03a: Interrupt Pending Register */
	u8	pad05[0x04];	/* 03c: */
	u8	RSR;		/* 040: Reset Status Register */
	u8	SYPCR;		/* 041: System Procection Control Register */
	u8	SWIVR;		/* 042: Software Watchdog Interrupt Vector Register */
	u8	SWSR;		/* 043: Software Watchdog Service Register */
  	u8	pad06[0x02];	/* 044 */
	struct _dram DRAM;	/* 046: DRAM stuff */
	struct _chip_select	CSBank0; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank1; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank2; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank3; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank4; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank5; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank6; /* 064: Chip-Select Bank 0 */
	struct _chip_select	CSBank7; /* 0b8: Chip-Select Bank 7 */
  	u8	pad07[0x02];	/* 0c4: */
	u16	DMCR;		/* 0c6: Default Memory Control Register */
	u8	pad08[0x02];	/* 0c8: */

#if 0
	u16	PAR;		/* 0ca: Pin Assignment Register */
#else
	u8	pad09;		/* 0ca: */
	u8	PAR;		/* 0cb: Pin Assignment Register */
#endif
	u8	pad0a[0x34];	/* 0cc: */
	u8	timer1[0x20];	/* 100: Timer 1 Module placeholder */
  	u8	timer2[0x20];	/* 120: Timer 2 Module placeholder */
	u8	UART1[0x40];	/* 140: UART1 placeholder */
	u8	UART2[0x40];	/* 180: UART1 placeholder */
	u8	pad0b[0x05];	/* 1c0: */
	u8	PPDDR;		/* 1c5: Port A Data Direction Register */
	u8	pad0c[0x03];	/* 1c6: */
	u8	PPDAT;		/* 1c9: Port A Data  */
	u8	pad0d[0x16];	/* 1ca: Port A Data  */
	struct _mbus	MBUS;	/* 1e0: MBUS module */
#if 0	
	u8 	pad0e[0x0c];	/* 1f4: */
	struct _dma DMA0;	/* 200: DMA Channel 0 */
	struct _dma DMA1:	/* 240: DMA Channel 1 */
#endif

};
		    
#endif
