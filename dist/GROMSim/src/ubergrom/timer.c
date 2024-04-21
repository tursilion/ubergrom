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
#include "timer.h"
#include "uart.h"

// We will be using Timer1
unsigned char nTimerValue;		// cached MSB

void TimerInit() {
	nTimerValue=0;

	TCCR1A = 0x00;		// no special counting modes
	TCCR1C = 0x00;		// no compare
	TCNT1 = 0x00;		// reset to zero
	TIMSK1 = 0x00;		// no interrupts

	TCCR1B = 0x05;		// CLK / 1024 (== 7812.5Hz) (TODO: this is from the I/O clock -- is that scaled down from the CPU clock?)
}

int TimerRead(unsigned char page, unsigned int address) {
	if (page != 0) {
		return -1;		// only 1 page
	}
	if (address < 0x20) {
		return 0;
	}
	address-=0x20;

	if (address & 0x01) {
		// MSB
		return nTimerValue;
	} else {
		// LSB - update value and cache	
		unsigned int nVal;
		nVal = TCNT1;
		nTimerValue = nVal>>8;
		return nVal&0xff;
	}

	return 0;
}

HANDLERS TimerHandlers = {
	TimerInit, TimerRead, DummyWrite
};	

