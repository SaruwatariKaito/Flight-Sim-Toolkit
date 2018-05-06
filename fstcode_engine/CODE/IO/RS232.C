/* Interrupt driven RS232 I/O */

#include <stdlib.h>
#include <stdio.h>

#include <dos.h>

#include "graph.h"
#include "rs232.h"

#define PIC_MASK 0x21
#define PIC_EOI 0x20

#define COM1_PORT_ADDRESS 0x3f8
#define COM1_INT 0x0c
#define INT1_MASK 0x10

#define COM2_PORT_ADDRESS 0x2f8
#define COM2_INT 0x0b
#define INT2_MASK 0x08

static int com_port = 2;
static int com_port_addr = COM2_PORT_ADDRESS;

#define COM_DATA           (com_port_addr+0)
#define COM_IER            (com_port_addr+1)
#define COM_IRQ_TYPE       (com_port_addr+2)
#define COM_LINE_CNTRL     (com_port_addr+3)
#define COM_MODEM_CNTRL    (com_port_addr+4)
#define COM_LINE_STATUS    (com_port_addr+5)
#define COM_MODEM_STATUS   (com_port_addr+6)

volatile int rs232_total = 0;
volatile int rs232_in = 0;
volatile int rs232_out = 0;

static int rs232_setup_done = FALSE;
static int interrupt_enabled = 0;

#define TX_MAX 64
static uchar tx_buffer[TX_MAX];
static volatile int tx_out_index = 0;
static volatile int tx_in_index = 0;

void (*rs232_rx_fn)(int code) = NULL;


void put_rs232(int code)
{
	int status;

	status = inp(COM_LINE_STATUS);
	while ((status & 0x20) == 0) {
		status = inp(COM_LINE_STATUS);
	}
	outp(COM_DATA, code);
}

/*int send_rs232(uchar *buffer, int n)
{
	while (n-- > 0) {
		put_rs232(*buffer++);
	}
	return TRUE;
}*/

int send_rs232(uchar *buffer, int n)
{
	put_rs232(*buffer++);
	n--;
	_disable();
	while (n-- > 0) {
		if (tx_in_index >= TX_MAX)
			tx_in_index = 0;
		tx_buffer[tx_in_index++] = *buffer++;
	}
	_enable();
	return TRUE;
}

static void _interrupt _far rs232_int(void)
{
	int type, code;

	rs232_total++;
	type = inp(COM_IRQ_TYPE);
	/* While interrupt pending */
	while ((type & 0x01) == 0) {
		type >>= 1;
		switch (type) {
			case 2:		/* Rx int */
				rs232_in++;
				code = inp(COM_DATA);
				if (rs232_rx_fn != NULL)
					(*rs232_rx_fn)(code);
				break;
			case 1:		/* Tx int */
				rs232_out++;
				if (tx_out_index >= TX_MAX)
					tx_out_index = 0;
				if (tx_out_index != tx_in_index) {
					outp(COM_DATA, tx_buffer[tx_out_index++]);
				}
				break;
			default:
				inp(COM_LINE_STATUS);	/* clears status irq */
				inp(COM_MODEM_STATUS);	/* clear modem irq   */
				break;
		}
		type = inp(COM_IRQ_TYPE);
	}
	outp(PIC_EOI, 0x20);
	_enable();
}

/* no parity, 1 stop, 8 data */
static void configure_rs232(int rate)
{
	int base, divisor;

	if (com_port == 1)
		base = 0x3f0;
	else
		base = 0x2f0;
	switch (rate) {
		case 1200: divisor = 0x60; break;
		case 2400: divisor = 0x30; break;
		case 4800: divisor = 0x18; break;
		case 9600: divisor = 0x0c; break;
		case 19200: divisor = 0x06; break;
		case 38400: divisor = 0x03; break;
		case 57600: divisor = 0x02; break;
		case 115200: divisor = 0x01; break;
		default: divisor = 0x0c; break;
	}
	outp(base + 0x0b, 0x80 | 0x03);
	outp(base + 0x08, divisor);
	outp(base + 0x09, 0x00);
	outp(base + 0x0b, 0x03);
}

void (_interrupt _far *prev_ser_int)(void);

void setup_rs232(int port, int rate)
{
	int mask, int_mask, int_number;

	com_port = port;
	configure_rs232(rate);
	_disable();
	if (port == 1) {
		com_port_addr = COM1_PORT_ADDRESS;
		int_mask = INT1_MASK;
		int_number = COM1_INT;
	} else {
		com_port_addr = COM2_PORT_ADDRESS;
		int_mask = INT2_MASK;
		int_number = COM2_INT;
	}
	outp(COM_LINE_STATUS, 0);
	outp(COM_MODEM_CNTRL, 0x0b);
	outp(COM_IER, 0x03);
	mask = inp(PIC_MASK);
	mask &= ~int_mask;
	outp(PIC_MASK, mask);
	prev_ser_int = _dos_getvect(int_number);
	_dos_setvect(int_number, rs232_int);
	interrupt_enabled = TRUE;
	_enable();
	rs232_setup_done = TRUE;
}

void tidyup_rs232(void)
{
	int mask, int_mask, int_number;

	if (rs232_setup_done) {
		if (com_port == 1) {
			int_mask = INT1_MASK;
			int_number = COM1_INT;
		} else {
			int_mask = INT2_MASK;
			int_number = COM2_INT;
		}
		if (interrupt_enabled) {
			outp(COM_IER, 0x00);
			mask = inp(PIC_MASK);
			mask |= int_mask;
			outp(PIC_MASK, mask);
			_dos_setvect(int_number, prev_ser_int);
		}
	}
}

