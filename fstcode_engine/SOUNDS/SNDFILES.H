#ifndef SNDFILES_H
#define SNDFILES_H 1
#define SBLASTER 10
#define GRAVIS 11
#define PC_SPK 12
#define FULL_PATHNAME 52	/* drive, 4 subdirs, filename */

#define MAX_VOL_RANGE 10000
#define MIN_VOL_RANGE 100



typedef struct
{
    unsigned char priority;     /* Allows multiple samples to be played */
    unsigned char InRam;        /* in ram, FALSE if on disk */
    volatile unsigned char NextBlockNeeded;
    unsigned char *buffer;      /* Whereabouts in RAM its stored (if InRam==TRUE) */
    volatile unsigned long remaining; /* bytes left to move to DMA RAM */
    unsigned long size;         /* bytes in sample */
    int segment;        /* DMA segment its currently playing from;modified as sample is played */
    int tconst;         /* DMA time const as sent to DSP chip */
    int DmaMode;        /* command parameter to send to DSP when starting */
                        /* o/p; determined by sample compression */
    char file[FULL_PATHNAME];      /* Needed if too big for RAM loading */
    unsigned long GravAddress;    /* Address of start in Gravis DRAM */
    int rate;           /* sample rate for non SB samples */
	char running;		/* TRUE if sound is running in repetitive mode*/
						/* it'll need turning off if noise is turned off */
	FILE *fp;			/* needed for multiple gravis file playing */
	int StopMe;			/* for Gravis fileplay */
    unsigned long FileCounter, NextBlock;	/* Needed for moving multiple VOCs to RAM*/
    int BlockType;							/* "	"	"	*/
} Sample;


extern unsigned long movewav2ram(Sample *sample, FILE *fp, unsigned char *dest,
    	unsigned long LoadSize, int NewSample, int AddEngine);
extern unsigned long movevoc2ram(Sample *sample,FILE *fp, unsigned char *dest,
    	unsigned long LoadSize, int NewSample, int AddEngine);


extern int atox(char *word);
extern int IsWavFile (char *filename);


extern void debug(char *format,...);




#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


#define MAX_VOLUME 0x1ff
#define MAX_BALANCE 0xf			/* max for Gravis */


/* originally SBLASTER, may be useful here */
#define DMA_DAC_8_CMD   0x14        /* For DMA DAC modes, uncompressed...*/

#endif /* SNDFILES_H */
