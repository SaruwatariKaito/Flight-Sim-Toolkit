/* Sound file operations */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sndfiles.h"
#include "sblaster.h"


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*              WAV FILES                                                    */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* Load a WAV file to 'Sample' struct */

static FILE *WavFile;
static int FormatTag,Nchannels,SampleRate,BytesPerSec,
            BlockAlign,BitsPerSample,stereo;
static unsigned long SampleSize;



int ReadChunkID(char *id)
/* Read 4 bytes from file, compare with reqd ID which may be < 4 bytes */
{
    char header[5],i,IdLen;
    int IDok;

    IDok=TRUE;
    i=0;
    IdLen=strlen(id);
    while (i<5)
    {
        if (i<4)                    /* reads 4 bytes from file */
        {
            header[i]=fgetc(WavFile);
        }
        if (i>=IdLen)   /* NULL terminates when reqd string length acquired */
        {
            header[i]=0;
        }
        i++;
    }

    if (strcmp(id,header)!=0) IDok=FALSE;

    return IDok;
}

unsigned long Fread4Bytes(void)
{
    char buff[5];
    unsigned long *uptr;
    int i;

    i=0;
    while (i<4)
    {
        buff[i]=fgetc(WavFile);
        i++;
    }
    uptr=(unsigned long *)buff;

    return *uptr;
}


int Fread2Bytes(void)
{
    char buff[3];
    int *ptr;
    int i;

    i=0;
    while (i<2)
    {
        buff[i]=fgetc(WavFile);
        i++;
    }
    while (i<4)
    {
        buff[i++]=0;
    }
    ptr=(int*)buff;

    return *ptr;
}


unsigned ReadWaveFormatChunk(Sample *samp)
{
    unsigned flen,done;

    ReadChunkID("fmt");

    /*ReadChunkID("fmt");*/
    flen=Fread4Bytes();             /* length of format data */
    done=0;
    FormatTag=Fread2Bytes();        /* ?? - only accept value of 1 */
    done+=2;
    Nchannels=Fread2Bytes();        /* 1/2 for Mono/Stereo */
    done+=2;
    SampleRate=Fread4Bytes();       /* per channel */
    done+=4;
    BytesPerSec=Fread4Bytes();      /* typically channels*sample rate */
    done+=4;
    BlockAlign=Fread2Bytes();
    done+=2;
    while(done <flen)
    {
        fgetc(WavFile);
        done++;
    }

    BitsPerSample=BlockAlign*8/Nchannels;   /* must be 8 */
	samp->rate=SampleRate;
    samp->tconst=256-1000000L/SampleRate;
    samp->DmaMode=DMA_DAC_8_CMD;

    return flen;
}


/* LoadCount=ReadWaveDataChunk(sample,LoadSize,NewSample);*/

unsigned long ReadWaveDataChunk(Sample *samp, unsigned long LoadSize, int ReadHeaders,unsigned char *dest, int AddEngine)
{
    int temp;
    unsigned long datacount;
	int sum;

    if (ReadHeaders==TRUE)
    {
        ReadChunkID("data");
        SampleSize=Fread4Bytes();
    }
    datacount=0;
    if (Nchannels==1)
    {
		if(BaseSample && AddEngine)
		{
        	while(datacount<SampleSize && datacount<LoadSize && !feof(WavFile) )
        	{
            	datacount++;
				sum=fgetc(WavFile) +*(BaseSample->buffer+(MemMoveOffset%BaseSample->size) );
				MemMoveOffset++;
				sum-=127;
				if(sum<0)sum=0;
				if(sum>255)sum=255;
            	*dest++=sum;
        	}
		}
		else
		{
        	while(datacount<SampleSize && datacount<LoadSize && !feof(WavFile) )
        	{
            	datacount++;
            	*dest++=fgetc(WavFile);
        	}
		}
    }
    else                                /** if (Nchannels==2) **/
    {
        while(datacount<SampleSize && datacount/2 < LoadSize && !feof(WavFile) )
        {
            /* convert to mono! */
            datacount++;
            temp=fgetc(WavFile);
            datacount++;
            *dest++=(fgetc(WavFile) + temp) /2 ;
        }
        datacount/=2;   /* bytes loaded = 1/2 bytes read */
    }

    return datacount;
}



unsigned long movewav2ram(Sample *sample, FILE *fp, unsigned char *dest,
    unsigned long LoadSize, int NewSample,int AddEngine)
{
    unsigned long LoadCount;
    int ReadHeaders;
    unsigned RiffLen,WaveLen;
    int Loadok=TRUE;

    WavFile=fp;
    LoadCount=0;
    if (NewSample==FALSE)  /* Part way thro reading data */
    {
		if (AddEngine && BaseSample)
        	LoadCount=ReadWaveDataChunk(sample,LoadSize,NewSample,dest,AddEngine);
		else
        	LoadCount=ReadWaveDataChunk(sample,LoadSize,NewSample,dest,FALSE);

    }
    else                   /* Read headers and set sample info */
    {
        Loadok = TRUE;
        if (ReadChunkID("RIFF")==FALSE) Loadok=FALSE;
        RiffLen=Fread4Bytes();
        if (ReadChunkID("WAVE")==FALSE) Loadok=FALSE;
        if (Loadok==TRUE)
        {
            ReadWaveFormatChunk(sample);

			if (AddEngine && BaseSample)
        		LoadCount=ReadWaveDataChunk(sample,LoadSize,NewSample,dest,AddEngine);
			else
        		LoadCount=ReadWaveDataChunk(sample,LoadSize,NewSample,dest,FALSE);
        }
    }
    if (Loadok==FALSE) LoadCount=0;
    return LoadCount;
}



/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*              VOC FILES                                                    */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/



/* Read the start of a .VOC file, and return offset to first data block */
static int ReadVocHdr(FILE *fp, int info)
{
    char VocHeader[0x1a];
    int i,result,Id_OK;

    i=0;
    Id_OK=TRUE;
    while (i<0x1a)
    {
        VocHeader[i]=fgetc(fp);
        if (i==18)
        {
            VocHeader[19]=0;
            if (info==TRUE) printf("\n%s",VocHeader);
            if (strcmp(VocHeader,"Creative Voice File")!=0) Id_OK=FALSE;
        }
        if (i==19) if (VocHeader[19]!=0x1a) Id_OK=FALSE;
        if (i==21 && info==TRUE) printf("\nData Offset %xh,%xh",VocHeader[20],VocHeader[21]);
        if (i==23 && info==TRUE) printf("\nFile format version %xh,%xh",VocHeader[22],VocHeader[23]);
        if (i==25 && info==TRUE) printf("\nID code %xh,%xh",VocHeader[24],VocHeader[25]);
        i++;
    }

    if (Id_OK==TRUE) result=VocHeader[21]*256+VocHeader[20]; /* File Offset to first data blk */
    else result=0;
    while(i<result && !feof(fp))
    {
        i++;
        fgetc(fp);  /* position file at Block Header */
    }

    return result;
}


/* Read .VOC file data block header and return block type */
/* Also sets (via *block pointer) where the next data block starts */
/* Also modifies file offset counter ( *file ) as it reads bytes from file */

static int ReadBlockHeader (FILE *fp,unsigned long *block,unsigned long *file)
{
    int ch,result;
    unsigned long BlkLen;

    result=fgetc(fp);  /* Block Type */
    ch=(unsigned char)fgetc(fp);  /* Block length, 3 bytes */
    BlkLen=ch;
    ch=(unsigned char)fgetc(fp);
    BlkLen+=(unsigned long)ch*256;
    ch=(unsigned char)fgetc(fp);
    BlkLen+= 65536*(unsigned long)ch;
    *file+=4;
    *block=*file+BlkLen;    /* Next data block header is at */
                                    /* current position+this blocks data size*/
    return result;
}

/* Moves sample data from a .VOC file into supplied RAM address */
/* See also movewav2ram() for .WAV files */
/* This function is used for loading Ram and disk samples */
/* and has to cope with being called whilst at any position*/
/* in a disk sample. Hence static vars and the 'ReadHeaders' flag, */
/* which prevents block headers from being read when entering function*/
/* part way thro a load. The structure of a .VOC file is not simple, */
/* and this loader DOES NOT handle all possible types of block  */
/* ( see 'BlockType' switch statement ) */

unsigned long movevoc2ram(Sample *sample,FILE *fp, unsigned char *dest,
    unsigned long LoadSize, int NewSample, int AddEngine)
{
    /*
	These variables are now in the Sample structure to enable multiple
	samples to be played from disk.

	static unsigned long FileCounter, NextBlock;
    static int BlockType;
	*/
    unsigned long LoadCount,temp;
    int ReadHeaders,packing;
	int sum;


    ReadHeaders=NewSample;
    if (NewSample==TRUE)
    {
        sample->NextBlock = ReadVocHdr(fp,FALSE);
        sample->FileCounter=sample->NextBlock;
        sample->BlockType=1;
    }
    LoadCount=0;
    while(sample->BlockType !=0 && LoadCount < LoadSize)
    {
if (BaseSample)
debug("\nReading Base sample data from %s %p",BaseSample->file,BaseSample);
        if (sample->FileCounter==sample->NextBlock) ReadHeaders=TRUE;
        if (ReadHeaders==TRUE)
        {
            sample->BlockType=ReadBlockHeader(fp,&(sample->NextBlock),&(sample->FileCounter)); /* NextBlock now contains file offset of next block header */
            temp=sample->FileCounter;
        }
        switch (sample->BlockType)
        {
        case 0: /* last block */
            break;
        case 1: /* Load sample data */
            if(ReadHeaders==TRUE)       /* Cannot cope with a change part-way thro ! */
            {
                sample->tconst=fgetc(fp);
                sample->rate=1000000L/(256-(sample->tconst));
                sample->FileCounter++;
                packing=fgetc(fp);
                sample->DmaMode=DMA_DAC_8_CMD;
                sample->FileCounter++;
            }
			if (AddEngine && BaseSample)
			{
				debug("\nAdding:-");
            	while (sample->FileCounter < sample->NextBlock && LoadCount < LoadSize) /* Read data for this block*/
            	{
					sum=fgetc(fp) +*(BaseSample->buffer+(MemMoveOffset%BaseSample->size) );
					MemMoveOffset++;
					sum-=127;
					if (sum<0)sum=0;
					if (sum>255)sum=255;
            		*dest++=sum;
                	sample->FileCounter++;
                	LoadCount++;
            	}
			}
			else
			{
            	while (sample->FileCounter < sample->NextBlock && LoadCount < LoadSize) /* Read data for this block*/
            	{
                	*dest++=fgetc(fp);
                	sample->FileCounter++;
                	LoadCount++;
            	}
			}
            break;
        case 2: /* Continuation block */
			if (AddEngine && BaseSample)
			{
            	while (sample->FileCounter < sample->NextBlock && LoadCount < LoadSize) /* Read data for this block*/
            	{
					sum=fgetc(fp) +*(BaseSample->buffer+(MemMoveOffset%BaseSample->size) );
					MemMoveOffset++;
					sum-=127;
					if (sum<0)sum=0;
					if (sum>255)sum=255;
            		*dest++=sum;
                	sample->FileCounter++;
                	LoadCount++;
            	}
			}
			else
			{
            	while (sample->FileCounter < sample->NextBlock && LoadCount < LoadSize) /* Read data for this block*/
            	{
                	*dest++=fgetc(fp);
                	sample->FileCounter++;
                	LoadCount++;
            	}
			}
            break;
        case 3: /* Silence block */
        case 4: /* Marker block */
        case 5: /* Text block */
        case 6: /* Repeat start */
        case 7: /* Repeat end */
        case 8: /*Extended block */
        default:
            break;

        }
    }
    return LoadCount;
}

/* Simple test determines if file is WAV. Voc is assumed if not TRUE. */
int IsWavFile (char *filename)
{
    char i,IsWav;

    i=0;
    strlwr(filename);
    while (filename[i] != '.' && i<8)
        i++;
    if ( strcmp(&filename[i],".wav")==0 ) IsWav=TRUE;
    else IsWav=FALSE;

    return IsWav;
}

/* convert hex number string to int */
/* Used for address string read from BLASTER setting */
int atox(char *word)
{
    int result;
    int more=TRUE;

    result=0;
    while (more==TRUE)
    {
        if (*word >= '0' && *word <='9')
        {
            result *=16;
            result+= *word-'0';
        }
        if (*word >='A' && *word < 'G')
        {
            result *=16;
            result+= *word+10-'A';
        }
        if (*word >='a' && *word < 'g')
        {
            result *=16;
            result+= *word+10-'a';
        }
        word++;
        if (isxdigit(*word)==0) more = FALSE;
    }
    return result;
}
