extern int InitSounds(int info);
extern void CloseSounds(void);

/* Basic sample handling */

extern Sample *LoadSample(char *filename, int priority, int RamLoad); /* only call once per sample */
extern int StartSample(Sample *s);
extern void UpdateSample(void);
extern int StopSample(Sample *s);

extern int StartRepSample(Sample *s);
extern void StopRepSample(Sample *s);

/* New Base sample functions */

extern Sample *LoadBaseSample(char *filename);
extern void SwapBaseSample(Sample **SwapSample);
extern void MakeBaseSample(Sample *swap);

extern int SetSampleVolume(Sample *s, int volume);
extern int SetSampleBalance(Sample *s, int balance);
extern int SetMasterVolume(int volume);
extern int SetMasterBalance(int balance);
