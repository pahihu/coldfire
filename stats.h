/*  Stats.h */
#ifndef STATS_H
#define STATS_H

/* Addressing Mode  Mode   Register    Description
        Dy          000    Dregnum     data register direct      
        Ay          001    Aregnum     address register direct   
       (Ay)         010    Aregnum     address register indirect 
       (Ay)+        011    Aregnum     ari with postincrement    
      -(Ay)         100    Aregnum     ari with predecrement     
     (d16,Ay)       101    Aregnum     ari with displacement 
    (d8,Ay,Xi)      110    Aregnum     ari with index base
     (xxx).W        111    000         absolute s16
     (xxx).L        111    001         absolute s32
     #<data>        111    100         immediate  
     (d16,PC)       111    010         program counter indirect disp
    (d8,PC,Xi)      111    011         pci with index base
*/                   
#ifdef MEMORY_STATS

enum {	sDRD=0, sARD, sARI, sARIPO, sARIPR, sARID, sARIIB, sAS, sAL,
            sIM, sPCID, sPCIIB, sBAD}; /* sBAD must be last addressing mode */

int Stats_Build_EA (int Register, int Mode);

                        
extern int Stats_Mode [sBAD];
extern int Stats_AReg  [8];
extern int Stats_DReg  [8];

#endif  /* MEMORY_STATS */
#endif  /* STATS_H */
