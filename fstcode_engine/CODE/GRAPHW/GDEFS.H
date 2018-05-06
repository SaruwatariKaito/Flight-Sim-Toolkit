/* Graphics library processor dependent data types */
/* For use with 32 bit compiler */

#ifndef gdefs_h
#define gdefs_h

#define MAX_SCREEN_X 1024
#define MAX_SCREEN_Y 768

#define VGA_320x200_256  1
#define VGA_640x480_16   2
#define VESA_640x480_256 3

#define EXPAND_COL(N) (N | (N << 8) | (N << 16) | (N << 24))

typedef unsigned char uchar;
typedef int int32;
typedef int Pos2d;
typedef int Pos3d;
typedef unsigned short Pindex;
typedef unsigned char sBitmap;

typedef struct {
    int x, y;
} sVector2;

typedef struct {
	int x, y, z;
} sVector;

typedef struct {
	int x, y, z;
} Aspect;

/* Angles are 16 bit scale 4 ie. 0 -> 4096 */

#ifndef PI
#define PI 3.141593
#endif
#define SSHIFT 12
#define SFACT (1 << SSHIFT)

#define TRIG_SHIFT(V) (((V) >= 0) ? ((V) >> SSHIFT) : -(-(V) >> SSHIFT))

#define DEG_1 182
#define DEG_MASK 0xFFF0
#define DEG(X) ((X) * DEG_1)
#define DEG_45 0x2000
#define DEG_90 0x4000
#define DEG_135 0x6000
#define DEG_180 0x8000
#define DEG_225 0xA000
#define DEG_270 0xC000
#define DEG_315 0xE000
#define DEG_360 0x10000
#define DEGTORAD (PI/DEG_180)
#define RADTODEG (DEG_180/PI)

#define ASHIFT 2
#define AMASK 0x0003
#define SIN(X) sctab[(unsigned short)(X) >> ASHIFT]
#define COS(X) sctab[((unsigned short)(X) >> ASHIFT) + 0x1000]
#define SLOWTAN(X) ((int)(tan((float)(X) * DEGTORAD) * SFACT))
#define TAN(X) fast_tan((short)X)
#define ACOS(X) (((X) >= 0) ? DEG_90 - acostab[X] : -DEG_90 + acostab[-(X)])

#define ADDANGLE(A, B) ((short)((A)+(B)))
#define SUBANGLE(A, B) ((short)((A)-(B)))
#define NEGANGLE(A) ((short)(-(A)))

#define MUL(X, Y) ((int)(X) * (int)(Y))
#define DIV(X, Y) ((int)(X) / (int)(Y))
#define MULDIV(X, Y, Z) (((int)(X) * (int)(Y)) / (int)(Z))

#endif
