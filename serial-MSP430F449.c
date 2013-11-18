#include "serial.h"
#include "bsp-449STK2.h"


Q_DEFINE_THIS_FILE;


#define SEND_BUFFER_SIZE 120
static char sendbuffer[SEND_BUFFER_SIZE];
static uint8_t sendhead;
static uint8_t sendtail;

static uint8_t sendbuffer_space(void)
{
	if (sendhead == sendtail) {
		return SEND_BUFFER_SIZE - 1;
	} else if (sendhead > sendtail) {
		return SEND_BUFFER_SIZE - 1 - (sendhead - sendtail);
	} else {
		/* sendhead < sendtail */
		return sendtail - sendhead - 1;
	}
}


/**
 * Call this with interrupts off.
 */
static void put_into_buffer(char c)
{
	sendbuffer[sendhead] = c;
	sendhead++;
	if (sendhead >= SEND_BUFFER_SIZE)
		sendhead = 0;
	/* If the transmitter is empty, prime it with the next char.  The USART
	   only generates one interrupt as the tx register drains. */
	if (! (IE2 & UTXIE1)) {
		U1TXBUF = sendbuffer[sendtail++];
		if (sendtail >= SEND_BUFFER_SIZE) {
			sendtail = 0;
		}
	}
	IE2 |= UTXIE1;
}


/**
 * Set up the USART in UART mode.
 *
 * See MSP430x4xx Family User’s Guide, slau056l.pdf, 17.2.1 USART
 * Initialization and Reset, page 17-4.
 *
 * The required USART initialization/reconfiguration process is:
 * 1) Set SWRST (BIS.B #SWRST,&UxCTL)
 * 2) Initialize all USART registers with SWRST = 1 (including UxCTL)
 * 3) Enable USART module via the MEx SFRs (URXEx and/or UTXEx)
 * 4) Clear SWRST via software (BIC.B #SWRST,&UxCTL)
 * 5) Enable interrupts (optional) via the IEx SFRs (URXIEx and/or UTXIEx)
 * Failure to follow this process may result in unpredictable USART behavior.
 */
void serial_init(void)
{
	U1CTL |= SWRST;

	/* Turn off the interrupt, as there's no data yet. */
	IE2 &= ~(UTXIE1);

	U1CTL =
		(PENA & 0)
		| (PEV & 0)
		| (SPB & 0)
		| (CHAR)
		| (LISTEN & 0)
		| (SYNC & 0)
		| (MM & 0)
		| (SWRST);
	U1TCTL =
		(CKPL & 0)
		| (SSEL1 | SSEL0) /* SMCLK, 1048576Hz */
		| (URXSE)
		| (TXWAKE & 0);
	U1RCTL = 0; 		/* Nothing to set in U1RCTL */
	/* See MSP430x4xx Family User’s Guide, slau056l.pdf, Table 17-2, page
	   17-16, for baud rate calculations. */
	/* 1048576 Hz clock, 115200 baud */
	U1BR1 = 0;
	U1BR0 = 0x09;
	U1MCTL = 0x08;
	/* Get the buffer ready. */
	sendhead = sendtail = 0;
	/* Enable the transmitter only. */
	ME2 |= UTXE1;
	/* Enable the pin output. */
	P4SEL |= 1;
	P4DIR |= 1;

	U1CTL &= ~(SWRST);
}


int serial_send(const char *s)
{
	int sent = 0;
	uint16_t sr;

	sr = __read_status_register();
	_BIC_SR(GIE);
	while (*s) {
		if (serial_send_char(*s++)) {
			sent++;
		} else {
			break;
		}
	}
	if (sr & GIE)
		_BIS_SR(GIE);
	return sent;
}


int serial_send_char(char c)
{
	int available;
	int sent;
	uint16_t sr;

	sr = __read_status_register();
	_BIC_SR(GIE);

	available = sendbuffer_space();
	if (available >= 1) {
		if (available == 1) {
			put_into_buffer('!');
			sent = 0;
		} else {
			put_into_buffer(c);
			sent = 1;
		}
	} else {
		sent = 0;
	}
	if (sr & GIE)
		_BIS_SR(GIE);
	return sent;

}


int serial_send_int(unsigned int n)
{
	char buf[10];
	char *bufp;
	uint16_t sr;
	int sent;

	bufp = buf + 9;
	*bufp = '\0';
	if (0 == n) {
		bufp--;
		*bufp = '0';
	} else {
		while (n) {
			int nn = n % 10;
			bufp--;
			*bufp = (char)(nn + '0');
			n /= 10;
		}
	}
	/* Send with interrupts off so we get the whole string out in one
	   piece. */
	sr = __read_status_register();
	_BIC_SR(GIE);
	sent = serial_send(bufp);
	if (sr & GIE)
		_BIS_SR(GIE);
	return sent;
}


int serial_send_hex_int(unsigned int x)
{
	char buf[10];
	char *bufp;
	uint16_t sr;
	int sent;

	static const char hexchars[] = "0123456789ABCDEF";

	bufp = buf + 9;
	*bufp = '\0';
	if (0 == x) {
		bufp--;
		*bufp = '0';
	} else {
		while (x) {
			int xx = x & 0x0f;
			char c = hexchars[xx];
			bufp--;
			*bufp = c;
			x >>= 4;
		}
	}
	/* Send with interrupts off so we get the whole string out in one
	   piece. */
	sr = __read_status_register();
	_BIC_SR(GIE);
	sent = serial_send(bufp);
	if (sr & GIE)
		_BIS_SR(GIE);
	return sent;
}


void serial_drain(void)
{
	uint16_t sr;

	/* The thing is turned off! */
	if (U1CTL & SWRST) {
		return;
	}

	sr = __read_status_register();
	if (sr & GIE) {
		/* Interrupts are on, so wait for the interrupt code to send
		   everything. */
		uint8_t counter = 0;
		while (sendhead != sendtail) {
			/* A character takess ~1.04ms at 9600 baud, so delay
			   slightly longer than that so we can count
			   characters.  Assume a 1MHz clock. */
			__delay_cycles(1100);
			counter ++;
			/* If we have sent more than a buffer's worth of
			   characters, we've been her too long. */
			Q_ASSERT( counter < SEND_BUFFER_SIZE );
		}
	} else {
		/* Interrupts off, stuff characters in the transmit buffer. */
		char c;

		while (sendhead != sendtail) {
			c = sendbuffer[sendtail];
			sendtail++;
			if (sendtail >= SEND_BUFFER_SIZE)
				sendtail = 0;
			while ( ! (U1TCTL & TXEPT) )
				;       /* Wait for buffer ready. */
			U1TXBUF = c;
		}
	}

}


static void
__attribute__((__interrupt__(USART1TX_VECTOR)))
isr_USART1TX(void)
{
	char c;

	if (sendhead == sendtail) {
		// No more data, stop tx interrupts.
		IE2 &= ~(UTXIE1);
	} else {
		c = sendbuffer[sendtail];
		sendtail++;
		if (sendtail >= SEND_BUFFER_SIZE)
			sendtail = 0;
		U1TXBUF = c;
	}
	/* We're awake now, so let the CPU run quickly so QP can put us back to
	   sleep soon. */
	EXIT_LPM();
}
