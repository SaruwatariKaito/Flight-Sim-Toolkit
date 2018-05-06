/* Machine specific functions */

#ifndef io_h
#define io_h

extern void debug_value(int val, int x, int y);
extern void debug(char *, ...);

#include "gdefs.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0L
#endif

#ifndef ABS
#define ABS(X) (((X) < 0) ? (-(X)) : (X))
#endif

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2

/* bits for use in key table */
#define KEY_DOWN 1
#define KEY_PRESSED 2

#define SHIFT_OFFSET 0x100
#define CTRL_OFFSET 0x200
#define ALT_OFFSET 0x400

/* keyboard functions */
extern volatile int key_events; 	/* count of key presses */
extern unsigned char key_table[128];	/* indexed by scan code */
extern void setup_keyboard(void);
extern void tidyup_keyboard(void);
extern int key_down(int code);
extern void flush_keyboard(void);
extern int wait_ascii(void);		/* waits for key press */
extern int wait_keypress(void);		/* waits for key press */
extern int check_keycode(void);		/* returns 0 or code immediately */
extern void enable_keyboard(void);
extern void update_keyboard(void);

/* control functions */
extern void setup_mouse(int x_range, int y_range);
extern void tidyup_mouse(void);
extern void set_mouse(int x, int y);
extern void reset_mouse(int x_range, int y_range);
extern int read_mouse(int *mx, int *my);
extern void setup_analog(void);
extern void reset_analog(void);
extern int read_analog(int modes[4], int values[4]);
extern int read_analog2(int values[4]);
extern void set_analog_limits(int min[4], int max[4]);
extern void get_analog_limits(int min[4], int max[4]);
extern void calibrate_analog(int modes[4], int col1, int col2, int col3, int col4);
extern int check_analog(void);
extern void use_bios_gameport(int state);

extern unsigned read_raw_adc(int num_channels, int *values);
extern int read_adc(int modes[4], int values[4]);
extern void reset_adc(void);

/* menu and display functions */
extern void set_pointer(int x, int y);
extern int get_pointer(int *key, int *px, int *py);

/* utility functions */
#define read_timer() total_time
extern volatile long total_time;
extern int escape_flag;
extern long disk_free_space(void);
extern int set_path(char *path);
extern void setup_program(long memory_needed);
extern void tidyup_program(void);

/* music functions */
extern int music_on;
extern void init_music(void);
extern void term_music(void);
extern void play_music(char *file_name);
extern void stop_music(void);

/* special purpose functions */
extern void cursor(int c);
extern void pc_sound(int *buffer, int length);
extern void tab(int x, int y);

#endif
