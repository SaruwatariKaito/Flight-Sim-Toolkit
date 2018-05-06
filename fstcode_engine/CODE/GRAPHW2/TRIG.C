/* Tables for graphics library */

#include <math.h>
#include <stdlib.h>

#include "graph.h"

#define TABSIZE 0x4000

int sctab[TABSIZE + (TABSIZE / 4)];

int acostab[SFACT + 1];

int tantab[TABSIZE / 4];

int atantab[256] = {
0,41,81,122,163,204,244,285,326,367,407,448,489,529,570,610,
651,692,732,773,813,854,894,935,975,1015,1056,1096,1136,1177,1217,1257,
1297,1337,1377,1417,1457,1497,1537,1577,1617,1656,1696,1736,1775,1815,1854,1894,
1933,1973,2012,2051,2090,2129,2168,2207,2246,2285,2324,2363,2401,2440,2478,2517,
2555,2594,2632,2670,2708,2746,2784,2822,2860,2897,2935,2973,3010,3047,3085,3122,
3159,3196,3233,3270,3307,3344,3380,3417,3453,3490,3526,3562,3599,3635,3670,3706,
3742,3778,3813,3849,3884,3920,3955,3990,4025,4060,4095,4129,4164,4199,4233,4267,
4302,4336,4370,4404,4438,4471,4505,4539,4572,4605,4639,4672,4705,4738,4771,4803,
4836,4869,4901,4933,4966,4998,5030,5062,5094,5125,5157,5188,5220,5251,5282,5313,
5344,5375,5406,5437,5467,5498,5528,5559,5589,5619,5649,5679,5708,5738,5768,5797,
5826,5856,5885,5914,5943,5972,6000,6029,6058,6086,6114,6142,6171,6199,6227,6254,
6282,6310,6337,6365,6392,6419,6446,6473,6500,6527,6554,6580,6607,6633,6660,6686,
6712,6738,6764,6790,6815,6841,6867,6892,6917,6943,6968,6993,7018,7043,7068,7092,
7117,7141,7166,7190,7214,7238,7262,7286,7310,7334,7358,7381,7405,7428,7451,7475,
7498,7521,7544,7566,7589,7612,7635,7657,7679,7702,7724,7746,7768,7790,7812,7834,
7856,7877,7899,7920,7942,7963,7984,8005,8026,8047,8068,8089,8110,8131,8151,8172,
};

static int fast_atan_sub(int32 x, int32 y)
{
    if (x == 0)
        return(0);
    if (y == 0)
        return(DEG_90);
    if (y == x)
        return(DEG_45);
    if (y > x) {
        if (y < 0x10000L) {
            y <<= 7;
            x <<= 15;
            x /= y;
            return(atantab[x]);
        }
        else {
            y >>= 8;
            x /= y;
            return(atantab[x]);
        }
    }
    else {
        if (x < 0x10000L) {
            x <<= 7;
            y <<= 15;
            y /= x;
            return(DEG_90 - atantab[y]);
        }
        else {
            x >>= 8;
            y /= x;
            return(DEG_90 - atantab[y]);
        }
    }
}

/* y = 0 degrees, x = 90 degrees, angle + clockwise, -180 -> +180 */
int fast_atan(int32 x, int32 y)
{
    if (y >= 0) {
        if (x >= 0)
            return(fast_atan_sub(x, y));
        else
            return(-fast_atan_sub(-x, y));
    }
    else {
        if (x >= 0)
            return(DEG_90 + (DEG_90 - fast_atan_sub(x, -y)));
        else
            return(-DEG_90 - (DEG_90 - fast_atan_sub(-x, -y)));
    }
}

int fast_tan(int a)
{
	unsigned a1;
	int v1, v2;

    if (a < 0) {
        if (a >= DEG(-50)) {
            return -tantab[-a >> ASHIFT];
        } else if (a >= DEG(-88)) {
			a = -a;
			a1 = a >> ASHIFT;
			v1 = tantab[a1];
			v2 = tantab[a1 + 1];
			a &= AMASK;
			if (a == 0)
				return -v1;
			else
				return(-(v1 + DIV((v2 - v1) << ASHIFT, a)));
        } else {
            return SLOWTAN(a);
        }
    } else if (a <= DEG(50)) {
        return tantab[a >> ASHIFT];
    } else if (a <= DEG(88)) {
		a1 = a >> ASHIFT;
		v1 = tantab[a1];
		v2 = tantab[a1 + 1];
		a &= AMASK;
		if (a == 0)
			return v1;
		else
			return(v1 + DIV((v2 - v1) << ASHIFT, a));
    } else {
        return SLOWTAN(a);
    }
}


void setup_trig(void)
{
    unsigned i, scsize, tansize, acossize;
    float conv, v;

    scsize = TABSIZE + (TABSIZE / 4);
    tansize = TABSIZE / 4;
	acossize = SFACT;
    conv = (float)(PI / (0.5 * TABSIZE));
    for (i = 0; i < scsize; i++) {
        v = (float)sin(i * conv) * SFACT;
        if (v >= 0.0)
            sctab[i] = (int)(v + 0.5);
        else
            sctab[i] = (int)(v - 0.5);
    }
    for (i = 0; i < tansize; i++) {
        v = (float)tan(i * conv) * SFACT;
        tantab[i] = (unsigned int)(v + 0.5);
    }
    for (i = 0; i < acossize; i++) {
        v = (float)acos((float)i / (float)SFACT) * ((float)DEG_180 / PI);
        acostab[i] = (unsigned int)(v + 0.5);
    }
    acostab[acossize] = 0;
    for (i = 0; i < 256; i++)
        atantab[i] = (int)((RADTODEG * atan2((float)i, 256.0)) + 0.5);
}

/*

#include <stdio.h>

static void print_tables()
{
    FILE *fp;
    int i, j, n;

    fp = fopen("tables", "w");
    fprintf(fp, "int sctab[TABSIZE + (TABSIZE / 4)] = {\n");
    for (n = 0, i = 0; i < ((TABSIZE + (TABSIZE / 4)) >> 4); i++) {
        for (j = 0; j < 16; j++) {
            fprintf(fp, "%d,", sctab[n++]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "};\n");
    fprintf(fp, "int acostab[SFACT + 1] = {\n");
    for (n = 0, i = 0; i < (SFACT >> 4); i++) {
        for (j = 0; j < 16; j++) {
            fprintf(fp, "%ld,", (long)acostab[n++]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "0};\n");
    fprintf(fp, "int tantab[TABSIZE / 4] = {\n");
    for (n = 0, i = 0; i < ((TABSIZE / 4) >> 4); i++) {
        for (j = 0; j < 16; j++) {
            fprintf(fp, "%ld,", (long)tantab[n++]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "};\n");
    fprintf(fp, "int atantab[256] = {\n");
    for (n = 0, i = 0; i < (256 >> 4); i++) {
        for (j = 0; j < 16; j++) {
            fprintf(fp, "%d,", atantab[n++]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "};\n");
    fclose(fp);
}

int main()
{
    setup_trig();
    print_tables();
    return 0;
}
*/
