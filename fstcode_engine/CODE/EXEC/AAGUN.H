#ifndef aagun_h
#define aagun_h

#include "wdefs.h"
#include "cultural.h"

/* Class      : AAGun
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */
/*
 * burst_rate, burst_time, reload_time; initialisers
 * burst;		                        number of rounds left in burst
 * reloading;	                        time left for reload
 */

#define AAGun_Methods Cultural_Methods

#define AAGun_Vars Cultural_Vars            \
  sObject *target;							\
  int32 next_target_time;					\
  int fixed;								\
  int burst_delay, burst_size, reload_time; \
  int count, reload, rate;


/* Declare Class Structure typedefs */
typedef struct {
    AAGun_Methods
} AAGunClass;

typedef struct {
    AAGun_Vars
} AAGun;


/* Class Externs */
extern AAGunClass *AAGun_class;
extern void register_aagun(void);
#endif
