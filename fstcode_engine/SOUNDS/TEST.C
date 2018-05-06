#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <stdarg.h>
#include "sndfiles.h"
#include "sndriver.h"
#include "sblaster.h"

int ReadDMAC( void );


#define BODGE_IT

#ifdef BODGE_IT
int kbhit(void);
#endif

#define MAX_THROTTLE 255
#define MIN_THROTTLE 0
#define NUM_TEST_SAMPLES 20

int EnginePitch=200;
int engine_noise_on=TRUE;
int noise_on=TRUE;
int engine_on=TRUE;



void StartFMHeli(char *file)
		{
			printf("\nHeli %s ON",file);
			}


void StopFMHeli()
		{
			printf("\nHeli OFF");
			}

void stop(char *msg,...)
{
    if (msg!=NULL) printf("\n%s",msg);
}

static char ram_log=TRUE;
void debug(char *format, ...)
{
    va_list argptr;
    FILE *fp;

    if (ram_log == FALSE)
        return;

    fp = fopen("d:log", "a");
    if (fp) {
        va_start(argptr, format);
        vfprintf(fp, format, argptr);
        va_end(argptr);
        fclose(fp);
    }
}


main(int argc, char *argv[])
{
    int key,i,RepOn;
	Sample *TestSamples[NUM_TEST_SAMPLES];
	Sample *SwapSample;
	Sample *beeps;
	int working;
    char file[13];
    int started;
	int OldCount,j;
    int HeliOn;
	int vol=0xff;
	int bal=7;
	char number[16];



	j=0;
	while(j<NUM_TEST_SAMPLES)
	{
		TestSamples[j++]=NULL;
	}
	j=0;
    if ((working=InitSounds(TRUE)))
		printf("\nInitialised OK\n");
    else
		printf("\nNot Initialised OK\n");

    /*SetEngSoundCtrl(&EnginePitch);*/
    if (working==TRUE)
    {
		engine_on = FALSE;
        BaseSample=LoadBaseSample("hcont_i.voc");

		beeps			=LoadSample("beeps.voc",5,TRUE);
        TestSamples[0]	=LoadSample("cannon.voc",5,TRUE);
        TestSamples[1]	=LoadSample("chaff.voc",10,TRUE);
        TestSamples[2]	=LoadSample("end.voc",7,FALSE);
        TestSamples[3] 	=LoadSample("explo1.voc",8,FALSE);
        TestSamples[4] 	=LoadSample("explo2.voc",8,FALSE);
        TestSamples[5] 	=LoadSample("explo3.voc",8,FALSE);
        TestSamples[6] 	=LoadSample("flare.voc",8,FALSE);
        TestSamples[7] 	=LoadSample("gear.voc",8,FALSE);
        TestSamples[8] 	=LoadSample("warn.voc",8,FALSE);
        TestSamples[9] 	=LoadSample("low.voc",8,FALSE);
        TestSamples[10] 	=LoadSample("maw.voc",8,FALSE);
        TestSamples[11] 	=LoadSample("missile.voc",8,FALSE);
        TestSamples[12] 	=LoadSample("rocket.voc",8,FALSE);
        TestSamples[13] 	=LoadSample("swlock.voc",8,FALSE);
        key=0;
        while(key != 0x1b)
        {
            printf("\n\t1.\tcannon");
            printf("\n\t2.\tchaff");
            printf("\n\t3.\tend");
            printf("\n\t4.\texplo1");
            printf("\n\t5.\texplo2");
            printf("\n\t6.\texplo3");
            printf("\n\t7.\tflare");
            printf("\n\t8.\tgear");
            printf("\n\t9.\twarn");
            printf("\n\t10.\tlow");
            printf("\n\t11.\tmaw");
            printf("\n\t12.\tmissile");
            printf("\n\t13.\trocket");
            printf("\n\t14.\tswlock");
			printf("\n\t15.\tSwapSample");
            printf("\n\tZ\tChange Base sound");
            printf("\n\tB (right) / b(left) ");
            printf("\n\tV(louder) / v(quieter)");
            printf("\n\tESC\tExit");
            while ( !kbhit() )
			{
                UpdateSample();
				/*printf("\nDMA count is %d",ReadDMAC());*/
			}
			key=getch();
            switch (key)
            {
                case 'h':
                case 'H':
                    if (HeliOn)
                        StopFMHeli();
                    else
                        StartFMHeli(argv[1]);
                    HeliOn = !HeliOn;
                    printf("\nHeli Noise is %s ",(HeliOn?"On":"Off") );
                    break;
                case '=':
                    i=EnginePitch>>5;
                    if (i!>0)i=1;
                    if (EnginePitch < MAX_THROTTLE-i) EnginePitch+=i;
                    else EnginePitch=MAX_THROTTLE;
                    break;
                case '-':
                    i=EnginePitch>>5;
                    if (i!>0) i=1;
                    if (EnginePitch > MIN_THROTTLE+i) EnginePitch-=i;
                    else EnginePitch=MIN_THROTTLE;
                    break;
                case 'e':
                case 'E':
                    if (engine_noise_on==FALSE) engine_noise_on=TRUE;
                    else engine_noise_on=FALSE;
                    break;
                case 's':
                case 'S':
                    if (noise_on==FALSE) noise_on=TRUE;
                    else noise_on=FALSE;
                    break;
				case 'v':
					vol=vol- (vol>>4) -1;
					if (vol<0) vol=0;
					SetMasterVolume(vol);
					break;
				case 'V':
					vol=vol + (vol>>4) +1;
					if (vol>MAX_VOLUME) vol=MAX_VOLUME;
					SetMasterVolume(vol);
					break;
				case 'b':
					bal--;
					if (bal<0) bal=MAX_BALANCE;			/* Full left */
					SetMasterBalance(bal);
					break;
				case 'B':
					bal++;
					if (bal>MAX_BALANCE) bal=0; /* Full Right */
					SetMasterBalance(bal);
					break;
				case 'Z':
				case 'z':
					SwapBaseSample(&SwapSample);
					break;
				case 0x1b:
					break;
				default:				/* Not a special key, assume its a sample number */
					printf("\nEnter sample number");
					scanf("%d",&j);
					if (j==15) StartRepSample(beeps);
					else
					{
						j--;
						if(j>=0 && j <NUM_TEST_SAMPLES)
						{
							printf("\nStarting sample %d",j);
							MakeBaseSample(TestSamples[j]);
							engine_on=TRUE;
						}
					}
            }
            printf("\nThrottle is %d .Started is %s",EnginePitch ,(started==TRUE?"true":"false"));
            printf("\nVolume is %d .Balance is %d",vol,bal);


        }
        CloseSounds();
    }
/*    else
    {
        printf("\nSoundBlaster not available.");
    }*/
}
