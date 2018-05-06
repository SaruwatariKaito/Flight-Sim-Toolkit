/* Watcom 32 bit version */

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

static char buffer[256];

static union REGS r;

static int get_info(char *buff)
{
	struct SREGS sr;

	segread(&sr);
	sr.fs = 0;
	sr.gs = 0;
	r.x.eax = 0x4f00;
	r.x.edi = 0xa0000;
	sr.es = sr.ds;
	int386x(0x10, &r, &r, &sr);
	if (r.x.eax != 0x004f) {
		printf("get VESA info failed: %4x\n", r.x.eax);
		return 0;
	}
	return 1;
}

static void get_mode(int n, char *buff)
{
	struct SREGS sr;

	segread(&sr);
	r.x.eax = 0x4f01;
	r.x.ecx = n;
	r.x.edi = FP_OFF(buff);
	sr.es = FP_SEG(buff);
	int386x(0x10, &r, &r, &sr);
	if (r.x.eax != 0x004f) {
		printf("get mode failed: %4x\n", r.x.eax);
	}
}

static void show_mode(int mode)
{
	int i;
	char *b_info;
	short *w_info;

	printf("DATA FOR MODE %X\n", mode);
	for (i = 0; i < 256; i++)
		buffer[i] = 0;
	get_mode(mode, buffer);
	b_info = (char *)buffer;
	w_info = (short *)buffer;
	printf("Attributes: mode=%x, WinA=%x, WinB=%x\n", w_info[0], b_info[2], b_info[3]);
	printf("Win granularity: %4x\n", w_info[2]);
	printf("Win size: %4x\n", w_info[3]);
	printf("Win segments: A=%x, B=%x\n", w_info[4], w_info[5]);
	printf("Win func ptr: 1=%x, 2=%x\n", w_info[6], w_info[7]);
	printf("Bytes per scan line: %d\n", w_info[8]);
	if (w_info[0] & 0x0002) {
		printf("resolution: X=%d, Y=%d\n", w_info[9], w_info[10]);
		printf("char size: X=%d, Y=%d\n", b_info[22], b_info[23]);
		printf("Number of planes: %d\n", b_info[24]);
		printf("Bits per pixel: %d\n", b_info[25]);
		printf("Number of banks: %d\n", b_info[26]);
		printf("Memory model: %2x\n", b_info[27]);
		printf("Bank size: %2x\n", b_info[28]);
		printf("Number of image pages: %2x\n", b_info[29]);
	}
}

static int show_info(int mode)
{
	int i, found;
	char *b_info;
	short *w_info;
	short far *modes;
	char s[64];
	char far *p;

	found = 0;
	for (i = 0; i < 256; i++)
		buffer[i] = 0;
	if (get_info(buffer)) {
		b_info = (char *)buffer;
		w_info = (short *)buffer;
		for (i = 0; i < 4; i++) {
			s[i] = b_info[i];
		}
		s[4] = '\0';
		printf("Signature: %s, Version: %d\n", s, w_info[2]);
		p = MK_FP(w_info[4], w_info[3]);
		printf("CARD ID: ");
		for (i = 0; (*p != '\0') && (i < 60); i++) {
			printf("%c", *p++);
		}
		printf("\n");
		if (b_info[10] & 0x01)
			printf("Programmable 8 bit palette\n");
		else
			printf("Fixed 6 bit palette\n");
		printf("Total memory: %d Kbytes\n", w_info[9] * 64);
		printf("Supported modes: ");
		modes = MK_FP(w_info[8], w_info[7]);
		for (i = 0; (modes[i] >= 0) && (i < 20); i++) {
			printf("%x, ", modes[i]);
			if (modes[i] == mode)
				found = 1;
		}
		printf("\n");
	}
	return found;
}

int main(int argc, char** argv)
{
	int mode;

	mode = 0x101;
	if (argc == 2)
		sscanf(argv[1], "%X", &mode);
	printf("\n");
	if (show_info(mode)) {
		printf("\n");
		show_mode(mode);
	} else {
		printf("\n");
		printf("Mode %X not supported", mode);
		printf("\n");
	}
	return 0;
}

