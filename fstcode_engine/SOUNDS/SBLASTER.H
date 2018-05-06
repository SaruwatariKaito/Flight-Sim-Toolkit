#ifndef sblaster_h
#define sblaster_h

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


/* These are internal to SBLASTER.C */

#define SB_DEFAULT_TCONST 10000

/* SBPRO commands */

#define DIR_DAC_CMD     0x10        /* For direct writes to DAC */
#define DMA_DAC_8_CMD   0x14        /* For DMA DAC modes, uncompressed...*/
#define DMA_DAC_4_CMD   0x75        /* ... 2:1 compression... */
#define DMA_DAC_26_CMD  0x77        /* 3:1 */
#define DMA_DAC_2_CMD   0x17        /* 4:1 compresion */


#define DIR_ADC_CMD     0x20        /* Direct Read of ADC */
#define DMA_ADC_CMD     0x24        /* DMA ADC read,can only have uncompressed*/

#define SET_TCONST_CMD  0x40        /* Sets delay for DMA mode DAC o/p */
#define SET_BLKSZ_CMD   0x48        /* For setting DMA block size */

#define SILENCE_CMD     0x80        /* For initiating a timed silence output */

#define SPKR_OFF_CMD    0xd3        /* WARNING: the speaker commands can be */
#define SPKR_ON_CMD     0xd1        /* very slow. ALSO for early DSP's (v1.5) */
                                    /* speaker commands halt any DMAs in progress */

#define GET_SPKR_CMD    0xd8        /* Only for DSP chip version 2.00 and above*/

#define HALT_DMA_CMD    0xd0        /* Halts DMA accesses */
#define CONT_DMA_CMD    0xd4        /* Restarts halted DMA; seems to be */
                                    /* undefined if no DMA running */

#define GET_DSP_VER_CMD 0xe1        /* Follow this with TWO byte reads, */
                                    /* major then minor */


/* Addresses, IO */

/* SBBaseAddr is read from the "BLASTER" environment setting */

#define DSP_RESET_PORT SBBaseAddr+6
#define DSP_WRITE_PORT SBBaseAddr+0xc
#define DSP_WRITE_STATUS SBBaseAddr+0xc
#define DSP_READ_PORT SBBaseAddr+0xa
#define DSP_READ_STATUS SBBaseAddr+0xe


/* Interrupts, DMA, Memory allocations */
/* The DMA mem size is only adjustable within certain constraints:      */
/* i) Must be 64k max ii) if (!64k) , must be a whole              */
/* fraction(?) ie 32k 16k 8k etc */

#define DMA_INT_STACK_SZ 256        /* zero to use main stack */
#define MAX_DMA_SZ 0x8000L          /* max number of bytes for DMA mode */
#define SAMPLE_MEM_SZ 65536       /* Memory allocation for loading of sampled sounds */
#define MAX_RAM_SAMPLE_SZ SAMPLE_MEM_SZ  /* Max size for any one sample in RAM*/
#define DMAC_STATUS_REG 8           /* Look here to see if DMA run has finished */




/* Used for setting up DMA controller on reqd channel */
/* Will be filled with addresses of DMA registers, one struct for */
/* each DMA channel */
typedef struct
{
    int addr;
    int count;
    int page;
} DMARegisters;


/* structures and types for sound management */

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;



int alloc_real_memory (int);
void DisableSBPIntController(int);
void SetupDMAC( ulong , int , int );
int StartSB_DMA(ulong remaining ,int address, int mode);


/* These are for linkage to calling programs */


extern void SetEngSoundCtrl(int *ctrl);        /* Engine sound pitch control */

extern int InitSoundBlaster(int info);     /* info controls whether information on SB card setup is printf'ed. Normally FALSE */
extern int CloseSoundBlaster(void);
extern Sample *LoadSBSample(char* filename, int priority,int RamLoad); /* user priorities run from 2 upwards */
                                                  /* priority 1 is for BaseSample */
extern int StartSBSample(Sample *sample);
extern void StopSBSample(int TurnInterruptsOffWhileWeDoIt);
extern Sample *LoadSBBaseSample(char *file);               /* Named sample will be */
                                                  /* run when no others specified */
extern int StartSBRepSample(Sample * samp);         /* Autorepeating sample, */
                                                  /* can do multiple calls, */
                                          /* it just forgets the previous one */
extern void StopSBRepSample (void);
extern void UpdateSBSample(void);                   /* sorts out what to play when */
                                                  /* Call it frequently */

extern void SwapSBBaseSample(Sample **swap);
extern void MakeSBBaseSample(Sample *newbase);

extern unsigned char *SamplePtr;
extern int MemMoveOffset;
extern Sample *BaseSample;

extern int SetSBBalance(int balance);
extern int SetSBVoiceVolume(int volume);


#endif
