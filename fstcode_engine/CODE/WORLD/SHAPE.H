#ifndef shape_h
#define shape_h

#include "graph.h"
#include "wdefs.h"

typedef struct sShape_def {
    char   name[32];
    sShape *s;
    int32  visible_dist;
	int    flags;
	int n_first_polys;
	sPolygon **first_polys;
	int n_xp_yp_zp, n_xp_yp_zm, n_xp_ym_zp, n_xp_ym_zm;
    int n_xm_yp_zp, n_xm_yp_zm, n_xm_ym_zp, n_xm_ym_zm;
    sPolygon **xp_yp_zp, **xp_yp_zm, **xp_ym_zp, **xp_ym_zm;
    sPolygon **xm_yp_zp, **xm_yp_zm, **xm_ym_zp, **xm_ym_zm;
    struct sShape_def *down;
} sShape_def;

extern void init_shape(void);
extern void setup_shape(void);
extern void reset_shape(void);
extern void tidyup_shape(void);

extern sShape_def *load_shape_file(char *name);
extern sShape_def *get_shape(char *name);
extern int draw_shape(sShape *s, sVector *sp, sVector *sr, int shaded);
extern int draw_shape_def(sShape_def *s, sVector *sp, sVector *sr, int shaded);
extern int draw_shadow(sShape_def *sd, sVector *sp, sVector *sr);
extern int32 find_size(sShape_def *s, sVector *min_size, sVector *max_size);
extern void recolour_shape(sShape *s, sVector *r);

extern int sorting_algorithm;
extern int binary_shape_file;
extern int presort_on, fast_presort_on;

#endif
