/* (c) 2000, Matt Minnis */

#include "config.h"
#include "stats.h"


int Stats_Mode [sBAD]={0};
int Stats_AReg	[8]={0};
int Stats_DReg	[8]={0};

int Stats_Build_EA (int Register, int Mode)
{
	if (Mode!=7) {
		 if (Mode==0) Stats_DReg[Register]++;/* Count # times data register is in use */
		 else Stats_AReg[Register]++;	/* Count # times address register is in use */
	}
		
	switch (Mode) {	/* Count # times each addressing mode is used. */
	case 0:	Stats_Mode[sDRD]++; break;	/* Dy */
	case 1:	Stats_Mode[sARD]++; break;	/* Ay */
	case 2:	Stats_Mode[sARI]++; break;	/* (Ay) */
	case 3:	Stats_Mode[sARIPO]++; break;	/* (Ay)+ */
	case 4:	Stats_Mode[sARIPR]++; break;	/* -(Ay) */
	case 5:	Stats_Mode[sARID]++; break;	/* (d16,Ay) */
	case 6:	Stats_Mode[sARIIB]++; break;	/* (d8,An,Xi) */
	case 7: switch (Register) {
		case 0:	Stats_Mode[sAS]++; break; /* word addressing */
		case 1:	Stats_Mode[sAL]++; break; /* s32 addressing */
		case 2:	Stats_Mode[sPCID]++; break; /* (d16,PC) */
		case 3:	Stats_Mode[sPCIIB]++; break; /* (d8,PC,Xi) */
		case 4:	Stats_Mode[sIM]++; break; /* (d8,PC,Xi) */
		}
		break;
			
	}
	return 0;
}

