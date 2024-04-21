// This is NOT GPL or any other ""free"" software license. 
// If you want to create any form of derived work or otherwise use my code, 
// you MUST contact me and ask. I don't consider this a huge obstacle in
// the internet age. ;)
//
//
// (C) 2015 Mike Brent aka Tursi aka HarmlessLion.com
// This software is provided AS-IS. No warranty
// express or implied is provided.
//
// This notice defines the entire license for this code.
// All rights not explicity granted here are reserved by the
// author.
//
// You may redistribute this software provided the original
// archive is UNCHANGED and a link back to my web page,
// http://harmlesslion.com, is provided as the author's site.
// It is acceptable to link directly to a subpage at harmlesslion.com
// provided that page offers a URL for that purpose.
//
// It is NOT okay to add files to the archive, including those identifying
// any site other than Harmlesslion.com as the source. Note that
// HarmlessLion.com, Mike Brent, Tursi, and any other authorized party
// will disclaim any site other than Harmlesslion.com as a recognized
// download site and will discourage such downloads by users.
//
// Source code, if available, is provided for educational purposes
// only. You are welcome to read it, learn from it, mock
// it, and hack it up - for your own use only (and of course valid
// fair use under Copyright law - reviews, mocking, and the like. ;) )
//
// Please contact me before distributing derived works or
// ports so that we may work out terms. I don't mind people
// using my code but it's been outright stolen before. In all
// cases the code must maintain credit to the original author(s).
//
// -COMMERCIAL USE- Contact me first. I didn't make
// any money off it - why should you? ;) If you just learned
// something from this, then go ahead. If you just pinched
// a routine or two, let me know, I'll probably just ask
// for credit. If you want to derive a commercial tool
// or use large portions, we need to talk. ;)
//
// If this, itself, is a derived work from someone else's code,
// then their original copyrights and licenses are left intact
// and in full force.
//
// http://harmlesslion.com - visit the web page for contact info
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "main.h"
#include "uart.h"

volatile unsigned char nTxBufStart, nTxBufEnd;	// these MUST be char!
volatile unsigned char nRxBufStart, nRxBufEnd;
volatile unsigned char BufErrBits;				// bits for buffered overflows, etc
volatile unsigned char CacheBaudLSB;			// so that we can enforce write order

inline void EnableTxInterrupts();
inline void DisableTxInterrupts();
inline void EnableRxInterrupts();
inline void DisableRxInterrupts();

volatile unsigned char RXBuffer[256], TXBuffer[256];

void UartInit() {
	// configure USART the serial
	// calculation is UBRRn = ( fOSC / (8 * BAUD) ) - 1 
	// when double bit is on (16*BAUD when off, double bit on is less reliable)
	// To go the other way: BAUD = fOSC / ((UBBRR+1)*8)  -- *16 when U2X is off

	UBRR0H = 0;		// writes UBRRH
	UBRR0L = 8;		// set to 115,200 baud (8Mhz clock, double mode - really 111,111)

	UCSR0A = (1<<U2X0);					// clear all errors and flags, set 2x transmission mode
	UCSR0B = (1<<TXEN0) | (1<<RXEN0); 	// TX and RX enable
	UCSR0C = (0x3<<UCSZ00);				// sets 8N1
    
#ifdef SERIAL_DEBUG
	// also configure serial port 1
	UBRR1H = 0;		// writes UBRRH
	UBRR1L = 1;		// set to 500,000 baud (8Mhz clock, double mode)

	UCSR1A = (1<<U2X0);					// clear all errors and flags, set 2x transmission mode
	UCSR1B = (1<<TXEN0) | (1<<RXEN0); 	// TX and RX enable
	UCSR1C = (0x3<<UCSZ00);				// sets 8N1
#endif

	cli();
		nTxBufStart = 0;
		nTxBufEnd = 0;
		nRxBufStart = 0;
		nRxBufEnd = 0;
		BufErrBits = 0;
		CacheBaudLSB = 8;
	sei();

	EnableRxInterrupts();
	// don't enable Tx, those come on only when needed

#ifdef SERIAL_DEBUG
	printserial("Serial debug enabled\r\n");
#endif
}

int UartRead(unsigned char page, unsigned int address) {
	if (page != 0) {
		return -1;		// only one page
	}
	if (address < 0x20) {
		return 0;
	}

	// most useful one first for performance :)
	if (address >= 0x1000) {
		// read buffer
		// return 0 if the buffer is empty (not -1!)
		int x;

		cli();		// no interrupts while we mess with the pointers!
			if (nRxBufStart == nRxBufEnd) {
				// empty buffer
				x=0;
			} else {
				// get the next buffer character (wraparound is automatic now!)
				x=RXBuffer[nRxBufStart++];
			}
		sei();		// okay, back on

		return x;
	}

	if (address == 0x20) {
		// status/configuration flags
		int x;
		unsigned char reg = UCSR0A;
		// after use, try to clear the error flags (they are supposedly read-only)
		UCSR0A = reg & 0xE2;

		// allow to overwrite
		x=0;

		// first, the bits that don't depend on buffering (though they won't be useful in buffered mode, I think)
		if (reg & 0x10) {
			// frame error
			x|=0x10;
		}
		if (reg & 0x04) {
			// parity error
			x|=0x40;
		}

#ifdef SERIAL_DEBUG
		printserial("Cfg: ");
#endif
		// we are buffered, so refer to the buffer
		cli();
			if (nRxBufStart != nRxBufEnd) {
				// rx data available
				x|=0x01;
			}
			unsigned char n = nTxBufEnd+1;
			if (n!=nTxBufStart) {
				// tx buffer available
				x|=0x02;
			}
			if (nTxBufEnd == nTxBufStart) {
				x|=0x04;
			}
			if (BufErrBits) {
				// buffer overrun detected
				x|=0x20;
				BufErrBits=0;
			}
		sei();


#ifdef SERIAL_DEBUG
		printserial("x: ");
		print_hexbyte(x);
		printserial("  RS: ");
		print_hexbyte(nRxBufStart);
		printserial(" RE: ");
		print_hexbyte(nRxBufEnd);
		printserial("  TS: ");
		print_hexbyte(nTxBufStart);
		printserial(" TE: ");
		print_hexbyte(nTxBufEnd);
		printserial("\r\n");
#endif

		return x;
	}

	if (address == 0x21) {
		// line setting flags
		int x;
		unsigned char reg = UCSR0C;

		// word size (and initializer for x)
		x = (reg & 0x06)>>1;

		// parity
		x |= (reg & 0x30)>>2;

		// stop bits
		x |= (reg & 0x08)<<1;

		// 2X mode
		x |= (UCSR0A & 0x02)<<4;

		return x;
	}

	if (address == 0x22) {
		// baud rate LSB
		return ((int)UBRR0L)&0xff;
	}
	if (address == 0x23) {
		// baud rate MSB (note! only 4 bits!)
		return ((int)UBRR0H)&0xff;
	}
	if (address == 0x24) {
		int x;
		// number of bytes available to RX
		cli();
			x=(nRxBufEnd-nRxBufStart)&0xff;
#ifdef SERIAL_DEBUG						
			printserial("RS: ");		
			print_hexbyte(nRxBufStart);	
			printserial(" RE: ");
			print_hexbyte(nRxBufEnd);
			printserial(" Size: ");
			print_hexbyte(x);
			printserial("\r\n");
#endif
		sei();
		return x;
	}
	if (address == 0x25) {
		int x;
		// number of free bytes in TX buffer
		cli();
			x=(nTxBufStart-nTxBufEnd-1)&0xff;
#ifdef SERIAL_DEBUG
			printserial("TS: ");
			print_hexbyte(nTxBufStart);
			printserial(" TE: ");
			print_hexbyte(nTxBufEnd);
			printserial(" Size: ");
			print_hexbyte(x);
			printserial("\r\n");
#endif
		sei();
		return x;
	}

	// what was it then?
	return -1;
}

void UartWrite(unsigned char page, unsigned int address, unsigned char data) {
	if (page != 0) {
		return;		// only one page
	}
	if (address < 0x20) {
		return;
	}

	// most useful one first for performance :)
	if ((address >= 0x100) && (address < 0x1000)) {
		// else, we are using the buffer! set overrun bit if the buffer is full
		cli();		// no interrupts while we mess with the pointers!
			if (((unsigned char)(nTxBufEnd+1)) == nTxBufStart) {
				// full buffer - just ignore it (no indication! User was supposed to check!)
#ifdef SERIAL_DEBUG
				printserial("Full buffer, dropping data\r\n");
#endif
				return;					
			} else {
				// store it in the buffer
			 	TXBuffer[nTxBufEnd++] = data;
				// and check whether we need to send a character (to trigger interrupts if needed)
				EnableTxInterrupts();	// let it start going
			}
		sei();		// okay, back on
		
		return;
	}

	if (address == 0x20) {
		// status/configuration flags - these are all read-only
		return;
	}

	if (address == 0x21) {
		// line setting flags

		cli();
#ifdef SERIAL_DEBUG
			printserial("App changing serial parameters:");
			print_hexbyte(data);
			printserial("\r\n");
#endif
			// zero bits we are going to change here
			UCSR0C &= ~(0x3E);

			// word size
			UCSR0C |= (data&0x03)<<1;

			// parity
			UCSR0C |= (data&0x0c)<<2;

			// stop bits
			UCSR0C |= (data&0x10)>>1;

			// 2X mode
			if (data & 0x20) {
				UCSR0A |= 0x02;
			} else {
				UCSR0A &= ~0x02;
			}
		sei();

		return;
	}

	if (address == 0x22) {
#ifdef SERIAL_DEBUG
		printserial("App changing serial baud lsb: ");
		print_hexbyte(data);
		printserial("\r\n");
#endif
		// don't change it till we have the MSB
		CacheBaudLSB = data;
	}
	if (address == 0x23) {
#ifdef SERIAL_DEBUG
		printserial("App changing serial baud msb - setting to ");
		print_hexbyte(data&0x0f);
		print_hexbyte(CacheBaudLSB);
		printserial("\r\n");
#endif
		// baud rate MSB
		UBRR0H = data & 0x0f;;
		// baud rate LSB - this write updates the prescaler
		UBRR0L = CacheBaudLSB;
	}

	// read buffer, byte counts - nothing to do.
}

void EnableTxInterrupts() {
	UCSR0B |= (0x20);
}

void DisableTxInterrupts() {
	UCSR0B &= ~(0x20);
}

void EnableRxInterrupts() {
	UCSR0B |= (0x80);
}

void DisableRxInterrupts() {
	UCSR0B &= ~(0x80);
}

ISR(USART0_RX_vect) {
	unsigned char x = UDR0;		// read the char either way

#ifdef SERIAL_DEBUG
	printserial("RxInt\r\n");
#endif

	// store it into the buffer, but check if it's full
	if (((unsigned char)(nRxBufEnd+1)) == nRxBufStart) {
		BufErrBits = 1;
	} else {
		RXBuffer[nRxBufEnd++] = x;
	}
}

ISR(USART0_UDRE_vect) {
#ifdef SERIAL_DEBUG
	printserial("TxInt\r\n");
#endif
	if (nTxBufStart == nTxBufEnd) {
		DisableTxInterrupts();
		return;		// no data to send
	}

	UDR0 = TXBuffer[nTxBufStart++];
}


// The following are DEBUG functions and operate on serial port 1, not 0!
void putserialchar(unsigned char c) {
	// wait for the chip to mark ready...
	while ( !( UCSR1A & (1<<UDRE1)) ) { }
	// write the data
	UDR1=c;
}

void printserial(char *s) {
	while (*s) {
		putserialchar(*(s++));
	}
}

void print_hexbyte(unsigned char i)
{
	unsigned char h, l;

	h = i & 0xF0; // High nibble
	h = h>>4;
	h = h + '0';
	if (h > '9')
	h = h + 7;
	l = (i & 0x0F)+'0'; // Low nibble
	
	if (l > '9')
		l = l + 7;
	
	putserialchar(h);
	putserialchar(l);
}

HANDLERS UartHandlers = {
	UartInit, UartRead, UartWrite
};	

