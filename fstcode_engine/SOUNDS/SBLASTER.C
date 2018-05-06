/* This version provides (1) faster reversion to BaseSample by holding
it in its own DMA area and (2) maintains BaseSample while loading
another sample to play, this avoids clicks and smaal gaps at the start
of samples.  S.E. 26 Apr 1994 */

/* Sound Blaster Driver Copyright Simis Ltd 1993 */
/* Basic Priciple of operation:
* This file is to be compiled using DOS extender (-mx option).
* Samples are loaded by LoadSample() into the allocated storage
* memory (of SAMPLE_MEM_SZ bytes) if
* there is room, otherwise they play from disk. LoadSample() must
* only be called once per sound sample (otherwise memory is wasted) .
* Base Sample must be in RAM,
* (although with mods, could play from disk). Two DMA areas are allocated, each
* of MAX_DMA_SZ bytes, samples are moved from RAM or disk into these
* by StartSample and (if the sample is larger than MAX_DMA_SZ) UpdateSample.
* A Base Sample will play (if LoadBaseSample() has been called) which will run
* if no others are playing.
* Samples are assigned priorities, a higher priority sound will kill stop the
* current sound and start itself. Sounds are not overlayed or 'stacked'
* (ie restart themselves when the higher priority one finishes)
* Using functions: see TEST.C for example.
* Call InitSoundBlaster() first, then LoadBaseSample() if reqd, then
* LoadSample() for each sound file. Calling SetEngSoundCtrl() can supply a
* replay speed for the Base Sample, its value must be 0-255.
* Filenames must supply the correct extension, *.VOC or *.WAV
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <int.h>
#include <conio.h>  /* for getch() */
#include <string.h>
#include <time.h>
#include <ctype.h>




#include "sndfiles.h"
#include "sblaster.h"


static volatile int SBDMAIntFlag;   /* Flag signifies interrupt received */
static int DMASeg1=0, DMASeg2=0;    /* Addresses of DMA memory allocations*/
/* Card parameters read from ENVIRONMENT */
static int SBBaseAddr=0x220;
static int SB_IRQNum=7;             /* Hardware IRQ line number,not CPU int*/
static int SB_DMAChan=1;
static int SBType=1;
static int DMACMode;                /* only use 'read single block' transfers*/
static int DMAAddrReg,DMACountReg,DMAPageReg; /* DMA control register i/o addresses */
static int SBIntNum;                /* Int number as seen by CPU */
static Sample *RepeatSample=NULL;   /* Autorepeating sample */
static int RepeatOn;                /* TRUE if autorepeating sample reqd */
static unsigned char OldInt1Mask;           /* Preserve interrupt controller settings*/
static unsigned char OldInt2Mask;           /* WARNING: if other sections have stored */
                                    /* the masks too, they need to be restored in */
                                    /* the reverse order to that which they were stored in */
static int DoneIntMasks=FALSE;      /* Prevents storing int masks twice */
static unsigned char SBCardReady = FALSE;   /* TRUE if InitSoundBlaster() is OK */
static unsigned char *SampleMemBuffer;      /* Points to start of sound sample buffer */
static unsigned char *SamplePtr;            /* points to next available byte in sample buffer */
static Sample *CurrentSample=NULL;  /* Sample currently being played */
static volatile int CurrentPriority;/* 0-no sample 1-base sample 2..N loadable samples */
static int ChanMask;                /* used for turning DMAC off */
static FILE *PlayFile;              /* playing sample direct from Disk*/
static Sample ready;                /* Dummy used for Init's DMA and interrupt tests */
static int *ThrottleSetting=NULL;
static volatile int RunBaseSample=FALSE;
static int SBPro=FALSE;




int MemMoveOffset;
Sample *BaseSample=NULL;     /* This sample runs if no others started*/
extern int noise_on;
extern void *_x386_zero_base_ptr;

static const DMARegisters DMARegs[4]=
{
    {0,1,0x87},     /* I/O space addresses of DMAC registers, 4 chans */
    {2,3,0x83},
    {4,5,0x81},
    {6,7,0x82}
};


/****************************************/
/* Interrupt handler for DMA interrupts */

static int SBDMAInterrupt(struct INT_DATA *pd)
{
    int result, MyInt;
    long Remaining;

    MyInt=(inp(DMAC_STATUS_REG) & ( 1 << SB_DMAChan));  /* Read Terminal count bit for channel */
                                        /* from DMAC */
    if (MyInt==0) result = 0; /* if zero is returned, calls previous int handler */
    else
    {
        SBDMAIntFlag=TRUE;
        result=1;
        Remaining=CurrentSample->remaining;
        if (Remaining > 0)
        {
            if (Remaining >= MAX_DMA_SZ)
            {
                CurrentSample->NextBlockNeeded=TRUE;
                Remaining=MAX_DMA_SZ;
            }
            CurrentSample->remaining -=Remaining;
            StartSB_DMA(Remaining,CurrentSample->segment,CurrentSample->DmaMode);
        }
        else
        {
			if(RunBaseSample)
			{
				StartSB_DMA(BaseSample->size,BaseSample->segment,DMA_DAC_8_CMD);
				BaseSample->remaining=0;
				BaseSample->NextBlockNeeded=FALSE;
				CurrentPriority=1;
			}
			else
			{
            	CurrentPriority=0;
            	/* Dummy setup, clears Terminal Count bit-flag in DMAC */
            	/* Prevents further interrupts being routed in here */
            	SetupDMAC(10,DMASeg1,0);
			}
        }
        /* This must be done after any data reads: */
        inp(DSP_READ_STATUS);        /* acknowledges interrupt */
        outp(0x20,0x20);                    /* send EOI to Interrupt controller */
        if (SB_IRQNum >=10)                 /* and controller 2 if reqd */
                outp(0xa0,0x20);
    }
    return result;
}





static int EnableSBIntController(int IRQLine)
{
    /* Enable Interrupt Controller. Int Controller 1 for ints number 2,5,7  */
    /* Int Controller 2 for int number 10   */
    /* Controller 2 daisy-chains into controller 1 on int 2 */
    /* Bit is cleared to enable specified interrupt */
    /* MUST call with valid IRQ number  */

    int result;

    int_off();
    if (DoneIntMasks==FALSE)
    {
        OldInt1Mask = inp(0x21);
        OldInt2Mask = inp(0xa1);
        DoneIntMasks=TRUE;
    }

    if (IRQLine >= 10)
    {
    	outp( 0xa1, OldInt2Mask & (~(0x01<<(IRQLine-8))) );
    	outp( 0x21, OldInt1Mask & 0xfb );
    }
	else
    	outp( 0x21, OldInt1Mask&(~(0x01<<IRQLine) ));
    int_on();

	return TRUE;
}

static void DisableSBIntController(int IRQLine)
/* Restore mask to state it was before I fiddled with it. */
{
    int_off();
    if (DoneIntMasks==TRUE)
    {
        if (IRQLine !=0)
        {
            outp(0x21,OldInt1Mask);
            if (IRQLine >= 10) outp(0xa1,OldInt2Mask);
        }
    }
    int_on();
}

/* MUST check status before reading data */
static int ReadDSPData(void)
{
    int i,result;

    i=0;
    while ( (inp(DSP_READ_STATUS) & 0x80)==0 && i < 5000 )
		i++;
    if (i<5000) result = inp(DSP_READ_PORT);
    else result = 0;
	return result;
}


/* Is the DSP chip ready after a reset */
static int DSPReady(void)
{
    int result,i;

    i=0;
    while ( (ReadDSPData() != 0xaa) && i < 5000)
        i++;
    if (i<5000) result=TRUE;
    else result = FALSE;
	return result;
}

static int ResetDSP(void)
{
    int delay;
	outp(DSP_RESET_PORT,1);	/* set this bit high to reset DSP chip */
    delay=0;
    while (delay++ < 5000)   /* wait 3us (minimum), then clear it */
        ;
    outp(DSP_RESET_PORT,0);
    return ( DSPReady() );
}



/* called every time sample is started */
/* This function is called from the interrupt routine routine as well as*/
/* in line code. Therefore it must be protected by int_off() and */
/* int_on() when used in-line. */
static void SetupDMAC( unsigned long length, int segment, int offset)
{

    outp(0x0a,ChanMask);            /* Mask DMA channel */
    outp(0x0c,0);                   /* clear upper/lower data byte flip-flop*/
    outp(0x0b,DMACMode);             /* set DAC output mode */
    outp(DMAAddrReg,offset &0xff);  /* offset lo byte */
    outp(DMAAddrReg,(offset>>8) &0xff);
    outp(DMAPageReg,segment);       /* number of the 64k RAM page */
    outp(DMACountReg,length&0xff);  /* low byte of bytecount */
    outp(DMACountReg,(length>>8) & 0xff   );
    outp(0x0a,SB_DMAChan);          /* enable DMA channel  */
}



int ReadDMAC( void )
{
	int LowOffset,HiOffset;
	int LowCount,HiCount;

	int_off();
    outp(0x0a,ChanMask);            /* Mask DMA channel */
    outp(0x0c,0);                   /* clear upper/lower data byte flip-flop*/
    LowOffset=inp(DMAAddrReg);  /* offset lo byte */
    HiOffset=inp(DMAAddrReg);

    LowCount=inp(DMACountReg);  /* low byte of bytecount */
    HiCount=inp(DMACountReg);

    outp(0x0a,SB_DMAChan);          /* enable DMA channel  */
	int_on();

	return ( (HiCount&0xff)<<8 & (LowCount&0xff));
}



/* This function is called from the interrupt routine routine as well as*/
/* in line code. Therefore it must be protected by int_off() and */
/* int_on() when used in-line. */
/* MUST NOT send command unless DSP is ready */
static int SendCommand(int com)
{
    int i;

    i=0;
    while (i<20000 && (inp(DSP_WRITE_STATUS) & 0x80) != 0)
    {
        i++;
    }
    if (i<20000)
    {
        outp(DSP_WRITE_PORT, com);
        i=TRUE;
    }
    else i=FALSE;

    return i;
}

/* called every time sample is started */
/* If packed samples are used, it will set DSP to */
/* process as appropriate. */
/* This function is called from the interrupt routine as well as*/
/* in line code. Therefore it must be protected by int_off() and */
/* int_on() when used in-line. */

static int StartSB_DMA(unsigned long length, int segment,int mode)
{
        int offset;

        if (length>0)length--;
        offset=segment&0xffff;
        segment>>=16;    /* convert linear address to page no */
        SetupDMAC(length,segment,offset);
        SendCommand(mode);  /* packing type ( 8bit, 2.6 bit etc) */
        SendCommand(length & 0xff);  /* lo byte */
        SendCommand((length>>8) & 0xff);  /* hi byte */

}


/* initialisation of DMA , called from InitSoundBlaster() and AutoDetectSB() */

static int SetupSB_DMA(int info)
{
    int result=TRUE;
    long t1;
    int DMALength, segment, offset;
    int SBTimeConst;

	/* NOTE on interrupt numbers:	*/
    /* SB standard interrupt line is 7 (2,5 and 10 are others) */
	/* This is IRQ7 hardware interrupt line which passes through */
	/* the 8259 interrupt controller, and is seen by the CPU as int 0xf */
	/* This is the same as the parallel printer interrupt */
	/* Likewise IRQ2,5, and 10 are seen as ints 0xa, 0xd and 0x72 */
    /* (Yes, 72hex, coz it daisy-chains thro second 8259. */






    /* Setup DMAC register addresses as globals for speed & ease of use */
    /* Read I/O addresses from DMA regs table */

    DMAPageReg=DMARegs[SB_DMAChan].page;
    DMAAddrReg=DMARegs[SB_DMAChan].addr;
    DMACountReg=DMARegs[SB_DMAChan].count;

    DMACMode=0x48+SB_DMAChan; /* Single Block transfer output mode for DMAs */




    ready.remaining=0;
    CurrentSample=&ready;
    if (ResetDSP() !=TRUE) result=FALSE;
    else
    {
        switch (SB_IRQNum)
        {
            case 2:
            case 5:
            case 7:
                SBIntNum=SB_IRQNum+8;
                break;
            case 10:
                SBIntNum=0x72;
                break;
        }
        SBTimeConst= 256-(unsigned)(1000000L/(long)SB_DEFAULT_TCONST);

        int_off();
        if (int_intercept(SBIntNum,SBDMAInterrupt,DMA_INT_STACK_SZ) !=0) result=FALSE;
        else
        {

            ChanMask=4;
            ChanMask +=SB_DMAChan;          /* chan is 0 to 3 */
            SendCommand(SET_TCONST_CMD);
            SendCommand(SBTimeConst & 0xff);
            EnableSBIntController(SB_IRQNum);
            segment = DMASeg1;
            offset = 0; /* DMASegs have been adjusted for zero offset */
            SBDMAIntFlag=FALSE;
            StartSB_DMA(3,segment,DMA_DAC_8_CMD);
            int_on();
            t1 = clock() + 100;
            while (clock() < t1 && SBDMAIntFlag == FALSE)
                    {}
        }
        int_on();
    }
    if (SBDMAIntFlag !=TRUE)
    {
        result = FALSE;
        if (info==TRUE) printf("\nNo interrupts!");
    }
    return result;
}

/* Checks the globals set by GetSBEnvironment() */
static int CheckSBEnv(int info)
{
    int result=TRUE;
    int x;


    x=0x210;        /* Valid addresses are 0x210 - 0x260 */
    while ( x != SBBaseAddr && x<0x270)
        x +=0x10;
    if (x==0x270) result=FALSE;
    if (SB_IRQNum != 2 &&
        SB_IRQNum != 5 &&
        SB_IRQNum != 7 &&
        SB_IRQNum != 10 )
        result=FALSE;
    if (SB_DMAChan !=0&&
        SB_DMAChan !=1&&
        SB_DMAChan !=3)
        result=FALSE;
    if (SBType <1 || SBType >4) result=FALSE;
    if (info==TRUE)
        printf("\n BLASTER: Address %x Interrupt: %d DMA chan: %d Type %d",
            SBBaseAddr,SB_IRQNum,SB_DMAChan,SBType);
    return result;
}



/* Read one (unique) letter from BLASTER environment setting */
/* and return setting value */
static int ReadEnvItem (char letter)
{
    int result;
    char word[10];
    int j;
    char *EnvPtr;

    EnvPtr=getenv("BLASTER");

    if (EnvPtr !=NULL)
    {
        j=0;
        while (toupper(*EnvPtr++) != letter && j < 20)
            j++;
        while (isspace(*EnvPtr))
            EnvPtr++;     /* Test is seperate from inc, may be no space at all*/
        j=0;
        while ( isxdigit(*EnvPtr) && j<5)
        {
            word[j++] = *EnvPtr++;
        }
        word[j]=0;                  /* NULL terminate string */
        if (letter =='A') result = atox(word);
        else result = atoi(word);
    }
    else result = -1;

    return result;
}

/* Read BLASTER environment and set SB card globals */
static int GetSBEnvironment(int info)
{

    SBBaseAddr = ReadEnvItem('A');
    SB_IRQNum   = ReadEnvItem('I');
    SB_DMAChan = ReadEnvItem('D');
    SBType     = ReadEnvItem('T');
    if (info==TRUE)printf ("\nA:%x I:%d D:%d T:%d",SBBaseAddr,SB_IRQNum,
            SB_DMAChan,SBType);
    return CheckSBEnv(FALSE);
}





static int DetectAddress(void) /*Check for response from card at various addr*/
{

    int delay;

    SBBaseAddr=0;
    while (TRUE)
    {
        switch(SBBaseAddr)
        {
            case 0:     SBBaseAddr=0x220;break;
            case 0x220: SBBaseAddr=0x240;break;
            case 0x240: SBBaseAddr=0x210;break;
            case 0x210: SBBaseAddr=0x230;break;
            case 0x230: SBBaseAddr=0x250;break;
            case 0x250: SBBaseAddr=0x260;break;
            case 0x260: return FALSE;break;
        }
        /* Check by resetting card */
        outp(DSP_RESET_PORT,1);
        delay=0;
        while (delay++ < 500)
            ;
        outp(DSP_RESET_PORT,0);
        if (DSPReady()==TRUE)
        {
        return TRUE;
        }
    }
}

static int DetectInts(void)
{

    SB_IRQNum=0;
    SB_DMAChan=1;       /* Only allow auto detect on DMA chan 1 */
    while(TRUE)
    {
        switch(SB_IRQNum)
        {
            case 0:     SB_IRQNum=7;break;
            case 7:     SB_IRQNum=5;break;
            case 5:     SB_IRQNum=10;break;
            case 10:    SB_IRQNum=2;break;
        }
        if (SetupSB_DMA(FALSE)==FALSE)  /* Set up and test card */
        {
            int_restore(SBIntNum);
            DisableSBIntController(SB_IRQNum);
        }
        else
        {
            return TRUE;
        }
        if (SB_IRQNum==2) return FALSE;
    }
}


static int AutoDetectSB(void)
{
    int DetectOK;
    DetectOK   =DetectAddress();
    if(DetectOK==TRUE) DetectOK =DetectInts();
    return DetectOK;
}





/* Calls assembler routine in RALLOC_A.ASM to allocate 640k space memory */
/* Uses int 21h function 48h */
/* Offsets the allocated memory blocks to start on a 64k boundary */
static int AllocSB_DMAMem(int info)
{
    int result,DMASeg3,extra,DMASegEnd;


    result=TRUE;
    DMASeg1= 16*alloc_real_memory(MAX_DMA_SZ/16);
    DMASeg2= 16*alloc_real_memory(MAX_DMA_SZ/16);
    DMASegEnd=DMASeg2+MAX_DMA_SZ;


    if (DMASeg1==0||DMASeg2==0) result=FALSE;


    /* Are they one contiguous block ( we're sunk if they arent! */
    /* because the following adjustment to 64k page boundary is not possible) */
    if (DMASeg2 != (DMASeg1+MAX_DMA_SZ))
    {
        result = FALSE;
    }

    /* Adjust so each seg is within 64k boundary */
    /* (required by DMAC's addressing mechanism) */
    /* Seg size must be 64k, 32k, 16k etc */

    if (info==TRUE) printf("\nBefore shift, DMASeg1 %x DMASeg2 %x",DMASeg1, DMASeg2);

    DMASegEnd=DMASeg1+MAX_DMA_SZ-1;
    if ( (DMASegEnd&0xffff) < (DMASeg1&0xffff)) /* has it gone over boundary */
    {
        /* need to shift both DMAsegs */
        extra=(0x10000-DMASeg1&0xffff);
        DMASeg3=16*alloc_real_memory(extra/16);
        if (DMASeg3==0) result=FALSE;
        if (DMASeg3 != (DMASeg2+MAX_DMA_SZ) ) result=FALSE;
        DMASeg1+=extra;
        DMASeg2+=extra; /* DMASeg2 will be OK provided that MAX_DMA_SZ is*/
                        /* integer fraction of 64k*/

    }
    else    /* DMASeg1 is OK, therefore need to check DMASeg2 */
    {
        DMASegEnd=DMASeg2+MAX_DMA_SZ-1;
        if ( (DMASegEnd&0xffff) < (DMASeg2&0xffff)) /* has it gone over boundary */
        {
            extra=(0x10000-DMASeg2&0xffff);
            DMASeg3=16*alloc_real_memory(extra/16);
            if (DMASeg3==0) result=FALSE;
            if (DMASeg3 != (DMASeg2+MAX_DMA_SZ) ) result=FALSE;
            DMASeg2+=extra;
        }
    }
    if (info==TRUE) printf("\nAfter shift, DMASeg1 %x DMASeg2 %x",DMASeg1, DMASeg2);
    return result;
}



static void MemoryMove(unsigned char *dest, unsigned char *src, unsigned long size,int AddEngine)
{
    unsigned long i;
	int sum;

    i=0;
	if(RunBaseSample && AddEngine)
	{
debug("\nReading Base sample data from %s %p",BaseSample->file,BaseSample);
    	while(i<size)
    	{
			sum= (*src++) + *(BaseSample->buffer +(MemMoveOffset%BaseSample->size));
			sum-=127;
			if(sum<0)sum=0;
			if(sum>255)sum=255;
        	*dest++=sum;
			MemMoveOffset++;
        	i++;
    	}
	}
	else
	{
    	while(i<size)
    	{
        	*dest++=*src++;
        	i++;
    	}
	}
}


void SetEngSoundCtrl(int *ctrl)
{
    ThrottleSetting=ctrl;
}



/* Registers a Sample, and loads it into RAM storage if its approx size is */
/* less than the maximum #defined in SBLASTER.H. Sets 'fixed' data in */
/* Sample structure. Should only be called once per sample. */
Sample *LoadSBSample(char* file, int priority,int RamLoad)
{
    static unsigned long MemUsed=0;
    FILE *fp;
    Sample *new;
    unsigned long size;

    if (SBCardReady==TRUE)
    {
        size=filesize(file);
        fp=fopen(file,"rb");
        new=malloc(sizeof(Sample));
        if (fp!=NULL && new!=NULL)
        {
            new->priority=priority;
            if (!RamLoad || size>MAX_RAM_SAMPLE_SZ || size > SAMPLE_MEM_SZ-MemUsed )
            {
                strcpy(new->file,file);
                new->InRam=FALSE;
                new->size=MAX_DMA_SZ+2;
            }
            else
            {
                new->InRam=TRUE;
                new->buffer=SamplePtr;
                if (IsWavFile(file)==FALSE)
                    new->size = movevoc2ram(new,fp,SamplePtr,MAX_RAM_SAMPLE_SZ,TRUE,FALSE);
                else
                    new->size = movewav2ram(new,fp,SamplePtr,MAX_RAM_SAMPLE_SZ,TRUE,FALSE);
                SamplePtr += new->size;
                MemUsed+=new->size;
            }
            fclose(fp);
        }
        else
        {
            if (new!=NULL)
            {
                free(new);
            }
            if (fp!=NULL)
            {
                fclose(fp);
            }
            new=NULL;
        }
        return new;
    }
    else return NULL;
}


/* Loads a sample to play as background */
/* Loads into its own DMA mem area */

Sample *LoadSBBaseSample(char *file)
{
	int size;
	FILE *fp;
	int SegEnd,extra,DMASeg3;


	if (BaseSample) debug("LoadSBBaseSample called twice");
	debug("\nLoading Base Sample");
	if(SBCardReady)
	{
		size=filesize(file);
		if (size && size <MAX_DMA_SZ)
		{
			fp=fopen(file,"rb");
			if (fp) BaseSample=malloc(sizeof(Sample) );
			if (BaseSample)
			{
				size=size/16+1;
				BaseSample->segment=16*alloc_real_memory(MAX_DMA_SZ/16);
				if ( !BaseSample->segment)
				{
					free((void*)BaseSample);
					BaseSample=NULL;
				}
				else
				{
    				SegEnd=BaseSample->segment+16*size;
    				if ( (SegEnd&0xffff) < (BaseSample->segment&0xffff))
    				{
						debug("\nAdjusting Base Sample DMA seg");

        				extra=(0x10000-(BaseSample->segment&0xffff));
        				DMASeg3=16*alloc_real_memory(extra/16);
        				if (DMASeg3 && (DMASeg3 == (BaseSample->segment+16*size) ))
						{
        					BaseSample->segment+=extra;
						}
						else
						{
							free(BaseSample);
							BaseSample=NULL;
							debug("\nDMA adjustment failed");
							return BaseSample;
						}
    				}
					BaseSample->buffer=(unsigned char *)_x386_zero_base_ptr+BaseSample->segment;
					BaseSample->InRam=TRUE;
					BaseSample->priority=0;
					BaseSample->NextBlockNeeded=FALSE;
					BaseSample->remaining=0;
					sprintf(BaseSample->file,file);
					if(IsWavFile(file))
						BaseSample->size=
							movewav2ram(BaseSample,fp,
							BaseSample->buffer,
							MAX_DMA_SZ,TRUE,FALSE);
					else
						BaseSample->size=
							movevoc2ram(BaseSample,fp,
							BaseSample->buffer,
							MAX_DMA_SZ,TRUE,FALSE);
				}
			}
			if (fp) fclose(fp);
		}
	}
	return BaseSample;
}

/* Resets variables for current sample and halts DMA */
/* If not called from StartSample(), IntsOff should be TRUE */
/* to protect variables altered by interrupt routine */
void StopSBSample(int IntsOff)
{
    if (IntsOff==TRUE)int_off();
    if (CurrentSample!=NULL)
    {
        CurrentSample->remaining=0;
        CurrentSample->NextBlockNeeded=FALSE;
        SendCommand(HALT_DMA_CMD);
        CurrentPriority=0;
    }
    if (IntsOff==TRUE)int_on();
}

/* Start a registered sample. Plays from RAM, or disk if too big */
/* NOTE: use of int_off() and int_on() to avoid */
/* re-entry into (e.g.) SendCommand(), but minimise interrupt off time */
/* The DSP time constant is reprogrammed if reqd.*/
/* Base Sample is not reloaded if its already in DMA mem */
int StartSBSample(Sample *sample)
{
    int i;
    int allowed;
    if (SBCardReady==TRUE && sample !=BaseSample)
    {

        allowed=TRUE;
        if ( (sample==NULL)|| (sample->priority < CurrentPriority)) allowed=FALSE;
        if (noise_on==FALSE) allowed=FALSE;
        if ( allowed==TRUE )
        {
			MemMoveOffset=0;
            fclose(PlayFile); /* Might not be open. */
            sample->segment=DMASeg1;
            sample->remaining=sample->size;
            if (sample->InRam ==TRUE)
            {
                if (sample->size >=MAX_DMA_SZ )
                {
                    MemoryMove( (unsigned char *)_x386_zero_base_ptr+DMASeg1,
                        sample->buffer, MAX_DMA_SZ ,TRUE);

            		int_off();
            		SendCommand(HALT_DMA_CMD);
            		if(sample!=CurrentSample)
            		{
                		StopSBSample(FALSE);
                		SendCommand(SET_TCONST_CMD);
                		SendCommand(sample->tconst);
            		}
                    StartSB_DMA(MAX_DMA_SZ,sample->segment,sample->DmaMode);
                    sample->NextBlockNeeded=TRUE;
                    sample->remaining -= MAX_DMA_SZ;
                	int_on();
                }
                else
                {
					/***
                    if(sample !=CurrentSample) * dont reload it unnecessarily *
					****
					always Reload since AddEngine may have
					changed state since last start
					***/
                    {
                        MemoryMove( (unsigned char *)_x386_zero_base_ptr+DMASeg1,
                            sample->buffer,sample->size,TRUE);
                    }
            		int_off();
            		SendCommand(HALT_DMA_CMD);
            		if(sample!=CurrentSample )
            		{
                		StopSBSample(FALSE);
                		SendCommand(SET_TCONST_CMD);
                		SendCommand(sample->tconst);
            		}
                    StartSB_DMA(sample->remaining,sample->segment,sample->DmaMode);
                    sample->NextBlockNeeded=FALSE;
                    sample->remaining=0;
					int_on();
                }
            }
            else
            {
                PlayFile=fopen(sample->file,"rb");
                if (IsWavFile(sample->file))
                    sample->size = movewav2ram(sample,PlayFile,
                    (unsigned char *)_x386_zero_base_ptr+DMASeg1,MAX_DMA_SZ,TRUE,TRUE);

                else
                    sample->size = movevoc2ram(sample,PlayFile,
                        (unsigned char *)_x386_zero_base_ptr+DMASeg1,MAX_DMA_SZ,TRUE,TRUE);

            	int_off();
            	SendCommand(HALT_DMA_CMD);
                SendCommand(SET_TCONST_CMD);
                SendCommand(sample->tconst);    /* done here coz its not known until load is started */
                if (sample->size==MAX_DMA_SZ)
                {
                    sample->remaining=MAX_DMA_SZ+2;
                    sample->NextBlockNeeded=TRUE;
                    StartSB_DMA(MAX_DMA_SZ,sample->segment,sample->DmaMode);
                }
                else
                {
                    sample->remaining=0;
                    sample->NextBlockNeeded=FALSE;
                    StartSB_DMA(sample->size,sample->segment,sample->DmaMode);
                }
                int_on();


            }
            CurrentSample=sample;
            CurrentPriority=sample->priority;

        }
    }
    return allowed;     /* TRUE if sample started */
}

int StartSBRepSample(Sample * samp)
{
    if (samp !=NULL)
    {
        RepeatOn=TRUE;
        RepeatSample=samp;
        return TRUE;
    }
    return FALSE;
}

void StopSBRepSample (void)
{
    RepeatOn=FALSE;
}

static void SwapSampleMem(Sample *out,Sample *in)
{
	int i;
	static int SwapSize=0;
	register unsigned char byte;

	i=0;
	if (SwapSize==0)
	{
		SwapSize=in->size;
	}
	while (i<SwapSize)
	{
		byte            = *(i+in->buffer);
		*(i+in->buffer) = *(i+out->buffer);
		*(i+out->buffer) = byte;
		i++;
	}

	debug("\nSwapped %d bytes of sample mem ",i);
}

/* Kicks off interrupts; BaseSample controlled from interrupts after that */
void StartSBBaseSample(Sample *s)
{
	if (BaseSample)
	{
		int_off();
    	SendCommand(HALT_DMA_CMD);
    	SendCommand(SET_TCONST_CMD);
    	SendCommand(BaseSample->tconst);
		StartSB_DMA(BaseSample->size,BaseSample->segment,DMA_DAC_8_CMD);
		BaseSample->remaining=0;
		BaseSample->NextBlockNeeded=FALSE;
		CurrentPriority=1;
		RunBaseSample=TRUE;
		int_on();
	}
}


void SwapSBBaseSample(Sample **swap)
{
	/* Straight swap of two samples */
	/* Must load the one to be played into the DMA-able memory */
	/* which is pointed to by the original BaseSample */

	Sample *temp,*s;
	unsigned char *buftemp;
	int TempSeg;

	s=*swap;
	if (s->size < MAX_DMA_SZ && s->InRam)
	{
			/* Move sample data from swap sample into Base samples buffer */
			SwapSampleMem(BaseSample,s);
			int_off();
			StopSBSample(FALSE);
			temp = BaseSample;
			BaseSample=s;
			*swap=temp;

			s=*swap;						/* ...the previous BaseSample */
			TempSeg=BaseSample->segment;
			BaseSample->segment=s->segment;
			s->segment=TempSeg;
			buftemp=BaseSample->buffer;
			BaseSample->buffer=s->buffer;
			s->buffer=buftemp;

			int_on();
			if (RunBaseSample) StartSBBaseSample(BaseSample);
	}
}

static void MoveSampleMem(Sample *to,Sample *from)
{
	unsigned char *dest, *src;
	int size,i;

	src=from->buffer;
	dest=to->buffer;
	size=from->size;
	i=0;
	while(i<size)
	{
		*dest++=*src++;
		i++;
	}
}


void MakeSBBaseSample(Sample *newbase)
{
	/* 'newbase' is loaded into BaseSamples memory, and its size written */
	/* to the BaseSample's structure.	*/
	/* Sample rate is assumed to be same for all Base Samples */

	if (newbase->size < MAX_DMA_SZ && newbase->InRam)
	{
			/* Move sample data from swap sample into Base samples buffer */
			MoveSampleMem(BaseSample,newbase);

			int_off();
			{
				StopSBSample(FALSE);

				BaseSample->size=BaseSample->remaining=newbase->size;
			}
			int_on();

			if (RunBaseSample)
				StartSBBaseSample(BaseSample);
	}
	else debug("\nSample to load is not suitable for a Base Sample");
}



/* This function handles all processing needed for continuation of long */
/* samples, whether disk or RAM */
/* Must call this function at least every (DMA_MEM_SZ/sample_rate) */
/* seconds from main program loop */
/* eg. 64k of DMA mem, 10khz sample rate, call every 6.4 seconds max */
/* If this update rate is only achieved marginally, then int_off() and int_on()*/
/* may be needed */

void UpdateSBSample(void)
{
    if (SBCardReady==TRUE)
    {
        int seg,size,dun;
        unsigned char * segptr;


		if(noise_on && BaseSample)
		{
			if (!RunBaseSample)
			{
				StartSBBaseSample(BaseSample);
				debug("\nStarting base sample from update fn");
			}

		}
		else
			RunBaseSample=FALSE;
        if (RepeatOn==TRUE)
        {
            if (RepeatSample->priority >= CurrentPriority)
            {
                /* If its not currently running,                */
                /* or if its current but has reached the end... */
                if (RepeatSample !=CurrentSample || CurrentPriority <2)
                {
                    StartSBSample(RepeatSample);
                }
            }
        }

/******************
        if (CurrentPriority <2 && BaseSample !=NULL)    * update BaseSample *
        {
        	StopSBSample(TRUE);
        }

        else

		*************/
        {
            if (noise_on==TRUE)
            {
                if ( CurrentSample->NextBlockNeeded==TRUE &&
                        CurrentSample->remaining>0)
                {
                    seg = CurrentSample->segment;
                    if (seg==DMASeg1) seg=DMASeg2;
                    else seg = DMASeg1;
                    CurrentSample->segment=seg; /* For interrupt routine */
                    segptr= (unsigned char*)_x386_zero_base_ptr+seg;

                    if(CurrentSample->InRam==FALSE)
                    {

                        if (IsWavFile(CurrentSample->file))
                        {
                            CurrentSample->remaining= movewav2ram(CurrentSample,PlayFile,segptr,MAX_DMA_SZ,FALSE,TRUE);
                        }
                        else
                        {
                            CurrentSample->remaining= movevoc2ram(CurrentSample,PlayFile,segptr,MAX_DMA_SZ,FALSE,TRUE);
                        }

                        if (CurrentSample->remaining==MAX_DMA_SZ) CurrentSample->remaining+=2;
                        else
                        {
                            fclose(PlayFile);
                        }
                    }
                    else
                    {
                        if (CurrentSample->remaining >=MAX_DMA_SZ) size=MAX_DMA_SZ;
                        else size=CurrentSample->remaining;
                        dun=CurrentSample->size-CurrentSample->remaining;
                        MemoryMove(segptr, CurrentSample->buffer + dun,size,TRUE);
                    }
                    CurrentSample->NextBlockNeeded=FALSE;
                }
            }
            else
            {
                StopSBSample(TRUE);
            }
        }

    }
}



#define MixAddrReg SBBaseAddr+4
#define MixDataReg SBBaseAddr+5


static WriteMixReg(int reg,int data)
{
    outp(MixAddrReg,reg);
    outp(MixDataReg,data);
}

static int ReadMixReg(int reg)
{
    outp(MixAddrReg,reg);
    return inp(MixDataReg);
}

/* Use Master volume to control balance */
/* zero is fully left */
int SetSBBalance(int balance)
{
	int bvol;
	if (SBPro)
	{
		if (balance>15) balance=15;
		if (balance<0)  balance=0;

		bvol=15-balance;	/* left volume */
		bvol<<=4;
		bvol=bvol & balance;	/* right vol */

		WriteMixReg(0x22,bvol);
	}
}


/* volume of Voice (Sample) sounds */

int SetSBVoiceVolume(int volume)
{
	int myvol;
	if (SBPro)
	{
		if (volume>15) volume=15;
		if (volume<0)  volume=0;

		myvol=volume &(volume<<4);	/* SB vol is two 4 bit fields */

		WriteMixReg(4,myvol);
	}
}

static void SetVoiceStereo(void)
{
	WriteMixReg(0x0e,0b00000010);	/* obvious, isn't it? */
}


int SBProMixerReset(int info)
{
    int FoundMixer=TRUE;
	int data;

    WriteMixReg(0,0);                         /* Perform reset */

    /* Now check default values in Mixer regs */
    /* should get garbage if no mixer */
    /* Volume regs have two 4-bit settings in a byte. Range is 0 to 0xf, and */
    /* default setting is 9. Therefore byte should contain */
    /* 0x99 (one nibble per channel) */


	SetVoiceStereo();

	data=ReadMixReg(4);
    if (data !=0x99)
	{
		FoundMixer=FALSE; /* Voice output volume */
		if (info) printf("\n1.Voice volume %b",data);
	}

	WriteMixReg(4,0x55);
	data=ReadMixReg(4);
    if (data !=0x55)
	{
		FoundMixer=FALSE; /* Voice output volume */
		if (info) printf("\n2.Voice volume %b",data);
	}

	WriteMixReg(4,0x99);
	data=ReadMixReg(4);
    if (data !=0x99)
	{
		FoundMixer=FALSE; /* Voice output volume */
		if (info) printf("\n3.Voice volume %b",data);
	}

	data=ReadMixReg(0x22);
	if (data!=0x99)
	{
		FoundMixer=FALSE; /* Master output volume */
		if (info)
		{
			printf("\n1.Master volume %b",data);
		}
	}

	WriteMixReg(0x22,0x55);
	data=ReadMixReg(0x22);
	if (data!=0x55)
	{
		FoundMixer=FALSE; /* Master output volume */
		if (info)
		{
			printf("\n2.Master volume %b",data);
		}
	}

	WriteMixReg(0x22,0x99);
	data=ReadMixReg(0x22);
	if (data!=0x99)
	{
		FoundMixer=FALSE; /* Master output volume */
		if (info)
		{
			printf("\n3.Master volume %b",data);
			getch();
		}
	}

    return FoundMixer;
}

void FindMixer(int info)
{
	if (SBProMixerReset(info))
	{
		SBPro=TRUE;
	}
	else
	{
		SBPro=FALSE;
	}
}


/* Initialises the sound card, will call autodetect if BLASTER environment */
/* is wrong. Allocates storage memory, DMA RAM, setups up int routine and card*/
int InitSoundBlaster(int info)
{
    int result=TRUE;
    int AutoDetected=FALSE;

	if (SBCardReady==FALSE)
	{
        if (result == TRUE) SampleMemBuffer=malloc(SAMPLE_MEM_SZ);
		if (SampleMemBuffer==NULL) result = FALSE;
		else SamplePtr=SampleMemBuffer;
        if (result==FALSE && info==TRUE) printf("\nNo sample memory");
        if (result ==TRUE) if (GetSBEnvironment(info) == FALSE)
        {
            if (AutoDetectSB()==FALSE) result=FALSE;
            else AutoDetected=TRUE;
        }
        if (result==FALSE && info==TRUE) printf("\nBad Environment");
        if (result ==TRUE) if (AllocSB_DMAMem(info)==FALSE) result=FALSE;
        if (result==FALSE && info==TRUE) printf("\nNo DMA memory");
        if (result ==TRUE && AutoDetected==FALSE)
            if (SetupSB_DMA(info) == FALSE) result = FALSE;
        if (result==FALSE && info==TRUE) printf("\nSetup card failed");
        SBCardReady=result;
    }
    else result=TRUE; /* already done */

	SBPro=FALSE;
	FindMixer(info);
	if (info)
	{
		if (SBPro) printf("\n\n SoundBlaster Pro!!");
		else printf("\nNot a SBPro");
	}

	int_off();
    SendCommand(SPKR_ON_CMD);
    int_on();
    CurrentPriority=0;
    RepeatOn=FALSE;
    return result;
}

int CloseSoundBlaster(void)
{
    /* Restore interrupt controllers status */
	if (SBCardReady==TRUE)
	{
        DisableSBIntController(SB_IRQNum);
		int_restore(SBIntNum);
		/* Cannot deallocate memory allocated via int 21h function 48h */
		/* if FlashTek DOS extender used */
    	ResetDSP();
		SampleMemBuffer=NULL;
		SamplePtr=NULL;
        SBCardReady=FALSE;
        fclose(PlayFile);
	}
}
