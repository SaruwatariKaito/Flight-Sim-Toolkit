#ifndef explo_h
#define explo_h

#include "wdefs.h"
#include "mobile.h"

/* Class      : Explo
 * Superclass : Mobile
 *
 */

#define SHAPE2_MAXPOINTS 4
/* Internally linked structure for a shape with 2 polygons */
typedef struct {
    int npts;
    Vertex *pts;
    int npolygons;
    sPolygon **polygons;
	sVector min_size, max_size;
    int scale, radius;

    /* Array of points */
    Vertex point_data[SHAPE2_MAXPOINTS];
    /* Array of 2 polygons and actual polygon data */
    sPolygon *polygon_pointers[2];
    sPolygon polygon_data1;
    sPolygon polygon_data2;
    /* Point data for polygons */
    Pindex points1[SHAPE2_MAXPOINTS];
    Pindex points2[SHAPE2_MAXPOINTS];
} sShape2;


/* Define Class and Instance Templates */

#define Explo_Methods Mobile_Methods

#define Explo_Vars Mobile_Vars \
  sShape2 explo_shape;


/* Declare Class Structure typedefs */
typedef struct {           \
    Explo_Methods          \
} ExploClass;              \
                           \
typedef struct {           \
    Explo_Vars             \
} Explo;


/* Class Externs */
extern ExploClass *Explo_class;
extern void register_explo(void);

extern void create_explo(sVector p, sVector wv, sShape_def *sd, int speed);
#endif
