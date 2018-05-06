/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		sound.c
 * Ver:			2.00
 * Desc:		Sound event functions
 *
 * - driver implemented PC speaker
 * - hooks for sampled sound drivers
 *
 * -------------------------------------------------------------------------- */
/* Sounds for FST */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "sound.h"

#define CANNON	1
#define FIRE	2
#define GEAR	3
#define EXPLO1	4
#define EXPLO2	5
#define EXPLO3	6
#define LAND	7
#define MAW		8
#define PING	9
#define CLUNK	10
#define SWLOCK	11
#define BULLET	12
#define EJECT	13
#define FLARE	14
#define CHAFF	15
#define KLAX	16
#define SWITCH	17
#define GLIDE	18
#define LOWWARN	19
#define ENGINE  20
#define STALL   21
#define START	22
#define DIE		23

extern int gear_down;

#define ARM_ROCKET 1
#define ARM_SW 2
#define ARM_MAVERICK 3
#define ARM_BOMB 4

int normalised_aircraft_rpm = 0; /* in range 0-255 - determines frequency
								  * of basesoundsample */

int engine_on 		= TRUE;
int noise_on        = TRUE;
int engine_noise_on = FALSE;
int disable_sounds  = TRUE;

static int jet_sound_stopped = FALSE;

static int sound_driver_on = FALSE;

static void play_sound(int effect);
static void stop_sound(void);
static void set_sound_freq(int freq);
static void set_sound_volume(int volume);

/* Sampled sound pointers */
/* ---------------------- */
#define MAX_EFFECT 32

static int jet_data[100];

static void setup_jet_data(int rpm)
{
	int i;

	for (i = 0; i < 100; i += 10) {
		jet_data[i] = 5000 - (30 * rpm);
		jet_data[i + 1] = 5020 - (30 * rpm);
		jet_data[i + 2] = 5000 - (30 * rpm);
		jet_data[i + 3] = 5020 - (30 * rpm);
		jet_data[i + 4] = 5000 - (30 * rpm);
		jet_data[i + 5] = 5020 - (30 * rpm);
		jet_data[i + 6] = 5000 - (30 * rpm);
		jet_data[i + 7] = 5020 - (30 * rpm);
		jet_data[i + 8] = 5000 - (30 * rpm);
		jet_data[i + 9] = 5020 - (30 * rpm);
	}
}

/* Plays on channel 1 on multi-channel hardware */
static void jet_noise(int rpm)
{
	static int32 next_time = 0L;
	static int last_rpm = 0, jet_on = FALSE;
	int freq;

	freq = rpm * 4;
    if (engine_noise_on && engine_on && noise_on) {
			if ((rpm != last_rpm) || (total_time > next_time)) {
				setup_jet_data(rpm);
				pc_sound(jet_data, 70);
				next_time = total_time + 60;
				last_rpm = rpm;
				jet_on = TRUE;
			}
   	} else if (jet_on) {
		pc_sound(jet_data, 1);
		jet_on = FALSE;
	}
}

static int cannon_data[30] = {
9000,15000,9000,9000,15000,30000,30000,4000,9000,9000,
9000,15000,9000,9000,15000,30000,30000,4000,9000,9000,
9000,15000,9000,9000,15000,30000,30000,4000,3000,2000,
};

void cannon_noise(void)
{
	if (noise_on) {
		if (sound_driver_on)
			play_sound(CANNON);
		else
			pc_sound(cannon_data, 40);
    }
}

static int fire_fx = 7;
static int fire_length = 0, *fire_data = NULL;
static int rocket_data[40] = {
8000,7600,7200,6800,6400,6000,5600,5200,4800,4400,
4200,4000,3800,3600,3400,3200,3000,2800,2600,2400,
2200,2000,1800,1700,1600,1500,1400,1300,1200,1100,
1000,950,900,850,800,750,700,650,600,550,
};
static int sw_data[20] = {
6000,6000,6000,5000,6000,6000,5000,6000,6000,7000,
8000,9000,10000,11000,12000,13000,14000,15000,16000,17000,
};

void fire_noise(int length)
{
	if (noise_on) {
		if (sound_driver_on) {
			play_sound(FIRE);
		} else {
			if (length == 20) {
				pc_sound(sw_data, length);
			} else if (length == 40) {
				pc_sound(rocket_data, length);
			}
		}
    }
}

static int gear_data[20] = {
5000,4000,5000,4000,5000,5000,4000,5000,4000,5000,
5000,4000,5000,4000,5000,5000,4000,5000,4000,5000,
};
void gear_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(GEAR);
		else
			pc_sound(gear_data, 20);
    }
}

static int explo1_data[20] = {
30000,20000,30000,20000,30000,30000,20000,30000,20000,30000,
30000,20000,30000,20000,30000,30000,20000,30000,20000,30000,
};
void explo1_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(EXPLO1);
		else
			pc_sound(explo1_data, 20);
    }
}

static int explo2_data[20] = {
30000,28000,27000,26000,25000,24000,23000,22000,20000,22000,
24000,22000,20000,20000,20000,18000,18000,17000,17000,16000,
};
void explo2_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(EXPLO2);
		else
			pc_sound(explo2_data, 20);
    }
}

static int explo3_data[20] = {
30000,28000,27000,26000,25000,24000,23000,22000,20000,22000,
24000,22000,20000,20000,20000,18000,18000,17000,17000,16000,
};
void explo3_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(EXPLO3);
		else
			pc_sound(explo3_data, 20);
    }
}

static int landing_data[20] = {
1000,1000,1500,1500,2000,2000,1500,2000,2000,1500,
1000,1000,1000,1500,1500,1500,2000,1000,2000,2500
};
void landing_sound(void)
{
	if (noise_on && gear_down) {
		if (sound_driver_on)
			play_sound(LAND);
		else
			pc_sound(landing_data, 20);
    }
}

static int maw_data[40] = {
1300,1300,1300,1300,1300,2500,2500,2500,2500,2500,
1300,1300,1300,1300,1300,2500,2500,2500,2500,2500,
1000,1000,1000,1000,1000,2000,2000,2000,2000,2000,
1000,1000,1000,1000,1000,2000,2000,2000,2000,2000,
};
void maw_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(MAW);
		else
			pc_sound(maw_data, 40);
    }
}

static int ping_data[10] = {
2000,2000,2000,2000,2000,2000,2000,2000,2000,2000,
};
void ping_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(PING);
		else
			pc_sound(ping_data, 10);
    }
}

void clunk_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(CLUNK);
		else
			pc_sound(ping_data, 10);
    }
}

void swlock_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(SWLOCK);
    }
}

void flare_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(FLARE);
    }
}

void chaff_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(CHAFF);
    }
}

void near_bullet_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(BULLET);
    }
}

void eject_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(EJECT);
    }
}

void stallwarn_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(STALL);
    }
}

void lowwarn_sound(void)
{
	static int timeout = 0;

    if (noise_on && timeout < simulation_time) {
		if (sound_driver_on) {
			play_sound(LOWWARN);
			timeout = simulation_time + SECS(5);
		}
    }
}

void glidewarn_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(GLIDE);
    }
}

void switch_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(SWITCH);
    }
}

void klaxon_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(KLAX);
		else
			pc_sound(ping_data, 10);
    }
}

void start_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(START);
    }
}

void die_sound(void)
{
    if (noise_on) {
		if (sound_driver_on)
			play_sound(DIE);
    }
}

/* -------------------------------------------
 * Sound driver functions
 * ------------------------------------------- */

/* frequency arg is 0 - 255 */

/* --------------------
 * LOCAL set_sound_freq
 *
 * Hook for sampled sound driver
 */
static void set_sound_freq(int freq)
{
}

/* volume arg is 0 - 63 */

/* --------------------
 * LOCAL set_sound_volume
 *
 * Hook for sampled sound driver
 */
static void set_sound_volume(int volume)
{
}

/* Used for sound cards */

/* --------------------
 * LOCAL play_sound
 *
 * Hook for sampled sound driver
 */
static void play_sound(int effect)
{
}


/* --------------------
 * LOCAL stop_sound
 *
 * Hook for sampled sound driver
 */
static void stop_sound(void)
{
}


/* -------------------
 * GLOBAL stop_sounds
 *
 * Stops all sounds
 */
void stop_sounds(void)
{
}


/* -------------------
 * GLOBAL setup_sound
 *
 * Setup sound samples
 */
void setup_sound(void)
{
}


/* -------------------
 * GLOBAL play_sound_sample
 *
 */
void play_sound_sample(int n)
{
	if (sound_driver_on)
		play_sound(n);
}


/* -------------------
 * GLOBAL exit_sound
 *
 * Close down sound drviers
 */
void exit_sound(void)
{
}


/* -------------------
 * GLOBAL update_sounds
 *
 * Update base sample
 */
void update_sounds(void)
{
	static int last_rpm = 0;

    if (noise_on) {
		jet_noise(((aircraft_rpm - 20) * 64) / 80);
	}
}
