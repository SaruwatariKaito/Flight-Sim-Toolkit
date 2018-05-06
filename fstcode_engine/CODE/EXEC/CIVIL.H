#ifndef civil_h
#define civil_h

#include "wdefs.h"
#include "plane.h"

/* Class      : Civil
 * Superclass : Plane
 * 
 */

/* Define Class and Instance Templates */

#define Civil_Methods Plane_Methods

#define Civil_Vars Plane_Vars


/* Declare Class Structure typedefs */ 
typedef struct {            
    Civil_Methods           
} CivilClass;               
                            
typedef struct {            
    Civil_Vars              
} Civil;


/* Class Externs */ 
extern CivilClass *Civil_class;
extern void register_civil(void);
#endif
