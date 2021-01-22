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
#include "gpio.h"

// bits are hard coded. Sorry ;)
// But they are:
//
// PD6 - GPIO Pin 20 (GPIO0)
// PD7 - GPIO Pin 21 (GPIO1)
// PC4 - GPIO Pin 26 (GPIO2)
// PC5 - GPIO Pin 27 (GPIO3)
//

int GpioRead(unsigned char page, unsigned int address) {
#ifdef SERIAL_DEBUG
	printserial("GPIO Read entered\r\n");
#endif

	if (page > 0) {
		// only one page
#ifdef SERIAL_DEBUG
	printserial("GPIO Read failed for wrong page ");
	print_hexbyte(page);
	printserial("\r\n");
#endif
		return -1;
	}
	if (address < 0x20) {
#ifdef SERIAL_DEBUG
	printserial("GPIO Read failed for sub-0x20 address ");
	print_hexbyte(address&0xff);
	printserial("\r\n");
#endif
		return 0;
	}
	address-=0x20;

	if (address == 0x00) {
		// configuration byte - read back the /actual/ state rather than caching it
		unsigned char x;

		x=0;
		if (DDRD & _BV(6)) x|=0x01;
		if (DDRD & _BV(7)) x|=0x02;
		if (DDRC & _BV(4)) x|=0x04;
		if (DDRC & _BV(5)) x|=0x08;

#ifdef SERIAL_DEBUG
	printserial("GPIO Read returning configuration byte ");
	print_hexbyte(x);
	printserial("\r\n");
#endif

		return x;
	} else {
		// read the status of the pins
		unsigned char x;

		x=0;
		if (PIND & _BV(6)) x|=0x01;
		if (PIND & _BV(7)) x|=0x02;
		if (PINC & _BV(4)) x|=0x04;
		if (PINC & _BV(5)) x|=0x08;

#ifdef SERIAL_DEBUG
	printserial("GPIO Read returning pin status ");
	print_hexbyte(x);
	printserial("\r\n");
#endif

		return x;
	}
}

void GpioWrite(unsigned char page, unsigned int address, unsigned char data) {
#ifdef SERIAL_DEBUG
	printserial("GPIO write entered.\r\n");
#endif

	if (page > 0) {
		// only one page
#ifdef SERIAL_DEBUG
	printserial("GPIO write failed for bad page ");
	print_hexbyte(page);
	printserial("\r\n");
#endif
		return;
	}
	if (address < 0x20) {
#ifdef SERIAL_DEBUG
	printserial("GPIO write failed for sub 0x20 address ");
	print_hexbyte(address&0xff);
	printserial("\r\n");
#endif
		return;
	}
	address-=0x20;

	if (address == 0x00) {
#ifdef SERIAL_DEBUG
	printserial("GPIO write configuration byte ");
	print_hexbyte(data);
	printserial("\r\n");
#endif
		// configuration byte - configure the ports as requested
		if (data & 0x01) { DDRD |= _BV(6); } else { DDRD &= ~_BV(6); }
		if (data & 0x02) { DDRD |= _BV(7); } else { DDRD &= ~_BV(7); }
		if (data & 0x04) { DDRC |= _BV(4); } else { DDRC &= ~_BV(4); }
		if (data & 0x08) { DDRC |= _BV(5); } else { DDRC &= ~_BV(5); }
	} else {
#ifdef SERIAL_DEBUG
	printserial("GPIO write pins ");
	print_hexbyte(data);
	printserial("\r\n");
#endif
		// write the status of the pins
		if (data & 0x01) { PORTD |= _BV(6); } else { PORTD &= ~_BV(6); }
		if (data & 0x02) { PORTD |= _BV(7); } else { PORTD &= ~_BV(7); }
		if (data & 0x04) { PORTC |= _BV(4); } else { PORTC &= ~_BV(4); }
		if (data & 0x08) { PORTC |= _BV(5); } else { PORTC &= ~_BV(5); }
	}
}

HANDLERS GpioHandlers = {
	DummyInit, GpioRead, GpioWrite
};	

