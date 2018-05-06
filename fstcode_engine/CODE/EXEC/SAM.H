#ifndef sam_h
#define sam_h

#include "wdefs.h"
#include "cultural.h"

/* Class      : SAM
 * Superclass : Cultural
 *
 */

/* Define Class and Instance Templates */

#define SAM_Methods Cultural_Methods

#define SAM_Vars Cultural_Vars \
  long int detect, floor;		\
  int      kp;


/* Declare Class Structure typedefs */
typedef struct {
    SAM_Methods
} SAMClass;

typedef struct {
    SAM_Vars
} SAM;


/* Class Externs */
extern SAMClass *SAM_class;
extern void register_sam(void);
#endif
