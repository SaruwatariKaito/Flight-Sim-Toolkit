/* --------------------------------------------------------------------------
 * EXEC 		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		civil.c
 * Ver:			1.00
 * Desc:		Civil aircraft class
 *
 * Inherits all of Plane class behaviour
 * 
 * Civil class objects loaded from world.fst
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include "civil.h"

/* Declare Class Pointer */
CivilClass *Civil_class = NULL;


void register_civil(void)
{
    Civil_class = new_class(sizeof(CivilClass), "Civil", sizeof(Civil), Plane_class);
}

