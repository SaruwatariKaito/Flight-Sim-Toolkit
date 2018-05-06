#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>


#include "sndfiles.h"
#include "sndriver.h"
#include "sblaster.h"
#include "sound.h"


static int CardType;




int InitSounds(int info)
{
    int GotCard;
	int i;


    if (InitSoundBlaster(info)==TRUE)
        CardType=SBLASTER;
    else
        CardType=PC_SPK;

    GotCard=TRUE;
    if (CardType==PC_SPK) GotCard=FALSE;


    return GotCard;
}

void CloseSounds(void)
{
    if (CardType==SBLASTER) CloseSoundBlaster();

}

Sample *LoadSample(char *filename, int priority, int RamLoad)
{
	Sample *s;

	if (filesize(filename) > 0)
	{
    	if (CardType==SBLASTER)
        	s=LoadSBSample(filename,priority,RamLoad);
	}
	else
		s=(Sample*)NULL;
	debug("\nLoading %s %s", filename, (s==NULL?"Bad!":"OK"));
	return s;
}



extern int StartSample(Sample *s)
{
	int started=FALSE;

	if (s!=NULL)
	{
		if (CardType==SBLASTER)
        	started=StartSBSample(s);
	}
	return started;
}


extern void UpdateSample(void)
{
    if (CardType==SBLASTER)
        UpdateSBSample();
}


extern int StopSample(Sample *s)
{
    if (CardType==SBLASTER)
        StopSBSample(TRUE);
}


int StartRepSample(Sample *s)
{
	int started=FALSE;

	if (s!=NULL)
	{
    	if (CardType==SBLASTER)
        	started=StartSBRepSample(s);
	}
	return started;
}

void StopRepSample(Sample *s)
{

    if (CardType==SBLASTER)
        StopSBRepSample();
}


void SwapBaseSample(Sample **swap)
{
	int i;

	if (CardType==SBLASTER && BaseSample && *swap)
	{
		SwapSBBaseSample(swap);
	}
}

void MakeBaseSample(Sample *swap)
{
	int i;

	if (CardType==SBLASTER && BaseSample && swap)
	{
		MakeSBBaseSample(swap);
	}
}

Sample *LoadBaseSample(char *filename)
{
    if (CardType==SBLASTER)
        LoadSBBaseSample(filename);
}

int SetSampleVolume(Sample *s, int volume)
{
	int done=FALSE;


}

int SetSampleBalance(Sample *s, int balance)
{
	int done=FALSE;


}

/* volume is 0 to 511 */
int SetMasterVolume(int volume)
{
	int done=FALSE;

	if (CardType==SBLASTER)
	{
		done=SetSBVoiceVolume(volume>>5);		/* SB-Pro volumes 0-15 */
	}
	return done;
}


/* balance is 0 to 15 */
int SetMasterBalance(int balance)
{
	int done=FALSE;

	if (CardType==SBLASTER)
		done=SetSBBalance(balance);

	return done;
}
