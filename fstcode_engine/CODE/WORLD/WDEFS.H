#ifndef wdefs_h
#define wdefs_h

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "graph.h"

extern void werror(char *format, ...);

/* Variables/Function which must be provided by the
 * parent project
 */
extern int32 simulation_time;
extern int32 frame_number;
extern int  frame_time;
extern int  slow_cycle, fast_cycle;

extern int  view_base;

extern void stop(char *format, ...);
extern void debug(char *format, ...);

#define SaveVector(F, V) fprintf(fp, "%ld %ld %ld\n", (V).x, (V).y, (V).z);
#define LoadVector(F, V) fscanf(fp, "%ld %ld %ld\n", &((V).x), &((V).y), &((V).z));

#define BETWEEN(X,TOP,BOTTOM) (((X) < (TOP)) && ((X) > (BOTTOM)))

/* Position unit is (1 >> 10) = 1/1024 meter, roughly 1 mm */
/* Intermediate unit for rotations is (1 >> 3) ie 1/8 meter */

#define M(X) ((int32)(X) << 10)
#define INV_M(X) ((X) >> 10)
#define MM(X) ((int32)(X))
#define KM(X) (M(X) * 1000L)
#define Meter(X) ((int32)(X) << 3)
#define SPEED_SHIFT 7
#define KNOTS(X) (((X) >> 2) - ((X) >> 4))
#define FIXSPD(X) ((X) * 5)
#define GRAVITY 1
#define BULLET_GRAVITY 2
#define VIEW_MIN 32

#define SECS(X) ((X) * 100)
#define MINS(X) ((X) * 6000)

extern sVector  *rand_vect(void);

#endif
