/**********************************/
/*                                */
/*  Copyright 2000, David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

/* Monitor Show Stats      */
/* by Matt Minnis          */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

int Monitor_SS(int argc, char **argv)
{
#ifdef MEMORY_STATS
	int Temp;
	int i;

	if(argc==1) {
		printf("\nColdfire CPU Usage Statisitcs:\n"
		         "==================================\n");
		Temp=0;
		printf("Data Registers:\t\t");
		printf("Address Registers:\n");
		for (i=0; i < 8; i++) {
			printf("%6d : D%d",Stats_DReg[i],i);
			printf("\t\t\t%6d : A%d\n",Stats_AReg[i],i);
		}
		printf("\nAddressing Modes:\n");
		printf("%6d : Dy         data register direct\n",Stats_Mode[sDRD]);
		printf("%6d : Ay         address register direct\n",Stats_Mode[sARD]);
		printf("%6d : (Ay)       address register indirect\n",Stats_Mode[sARI]);
		printf("%6d : (Ay)+      ari with postincrement\n",Stats_Mode[sARIPO]);
		printf("%6d : -(Ay)      ari with predecrement\n",Stats_Mode[sARIPR]);
		printf("%6d : (d16,Ay)   ari with displacement\n",Stats_Mode[sARID]);
		printf("%6d : (d8,Ay,Xi) ari with index base\n",Stats_Mode[sARIIB]);
		printf("%6d : (xxx).W    absolute s16\n",Stats_Mode[sAS]);
		printf("%6d : (xxx).L    absolute s32\n",Stats_Mode[sAL]);
		printf("%6d : #<data>    immediate\n",Stats_Mode[sIM]);
		printf("%6d : (d16,PC)   program counter indirect disp\n",Stats_Mode[sPCID]);
		printf("%6d : (d8,PC,Xi) pci with index base\n",Stats_Mode[sPCIIB]);
		printf("%6d : None       Bad Addressing Mode\n",Stats_Mode[sBAD]);
	} else if(argc==2 && (argv[1][0] == 'C' || argv[1][0] == 'c')) {
		for (i=0; i<8; i++) {
			Stats_DReg[i]=0;
			Stats_AReg[i]=0;
		}
		for (i=0; i < sBAD; i++) Stats_Mode[i]=0;
	} else {
		return Monitor_show_help("SS");
	}
#else
	printf("Memory Stats were not compiled into this build.\n");
	printf("Use  'configure --enable-mem-stats'  and rebuild to enable stat gathering\n");
#endif
	return 1;
}
