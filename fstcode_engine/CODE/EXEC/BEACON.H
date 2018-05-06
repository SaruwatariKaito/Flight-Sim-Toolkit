#ifndef beacon_h
#define beacon_h

#include "wdefs.h"
#include "world.h"

/* Class      : Beacon
 * Superclass : World
 *
 */

/* Define Class and Instance Templates */

#define Beacon_Methods World_Methods

#define Beacon_Vars World_Vars \
  long int freq;


/* Declare Class Structure typedefs */
typedef struct {
    Beacon_Methods
} BeaconClass;

typedef struct {
    Beacon_Vars
} Beacon;


/* Class Externs */
extern BeaconClass *Beacon_class;
extern void register_beacon(void);
#endif
