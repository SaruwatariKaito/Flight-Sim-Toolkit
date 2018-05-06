/* Graphics library processor dependent data types */

#ifndef gdefs_h
#define gdefs_h

#define MAX_SCREEN_Y 200

typedef unsigned char uchar;
typedef long int32;
typedef int Angle;
typedef int Pos2d;
typedef long Pos3d;
typedef unsigned short Pindex;

typedef struct {
    int x, y;
} Vector2;

typedef struct {
    long x, y;
} Lvector2;

typedef struct {
	long x, y, z;
} Vector;

typedef struct {
	int x, y, z;
} Svector;

typedef struct {
	int x, y, z;
} Scoord;

typedef struct {
	int x, y, z;
} Aspect;

/* Angles are 16 bit scale 4 ie. 0 -> 4096 */

#ifndef PI
	#define PI 3.14159265358979
#endif

#define SSHIFT 10
#define SFACT (1 << SSHIFT)

#define TRIG_SHIFT(V) (((V) >= 0) ? ((V) >> SSHIFT) : -(-(V) >> SSHIFT))

#define DEG_1 		182	//.0416666667
#define DEG_MASK	0xFFF0
#define DEG(X)		((X) * DEG_1)
#define DEG_45		0x2000
#define DEG_90		0x4000
#define DEG_135	0x6000
#define DEG_180	0x8000
#define DEG_225	0xA000
#define DEG_270	0xC000
#define DEG_315	0xE000
#define DEG_360	0x10000
#define DEGTORAD	(PI/DEG_180)
#define RADTODEG	(DEG_180/PI)

#define ASHIFT 4
#define AMASK 0x000f
#define SIN(X) sctab[(unsigned short)(X) >> ASHIFT]
#define COS(X) sctab[((unsigned short)(X) >> ASHIFT) + 1024]
#define SLOWTAN(X) ((int)(tan((float)(X) * DEGTORAD) * SFACT))
#define TAN(X) fast_tan((short)X)

#define ADDANGLE(A, B) ((short)((A)+(B)))
#define SUBANGLE(A, B) ((short)((A)-(B)))
#define NEGANGLE(A) ((short)(-(A)))

#define MUL(X, Y) ((long)((long)(X) * (long)(Y)))
#define DIV(X, Y) ((long)(X) / (long)(Y))
#define MULDIV(X, Y, Z) (((long)(X) * (long)(Y)) / (long)(Z))

#endif
