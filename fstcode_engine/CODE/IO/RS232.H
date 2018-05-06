/* RS232 I/O */

extern volatile int rs232_total;
extern volatile int rs232_in;
extern volatile int rs232_out;
extern void (*rs232_rx_fn)(int code);
extern void put_rs232(int code);
extern int send_rs232(unsigned char *buffer, int n);
extern void setup_rs232(int port, int rate);
extern void tidyup_rs232(void);

