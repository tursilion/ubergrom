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
#include "eeprom.h"

// 1284 - 4k EEPROM
#define EEPROM_SIZE	(4*1024)

static unsigned char nLocked = 1;		// EEPROM is unlocked until unlock sequence is done

void EepromInit() {
	// configure EEPROM to be writable
	EECR = 0x00;		// erase&write in one 3.4ms operation, disable ready interrupt

	// check if the EEPROM is erased - if it's all 0xFF, then load a default configuration that
	// maps >6000 as the first page of GROM, with 
#ifdef SERIAL_DEBUG
	printserial("Checking EEPROM...\r\n");
#endif

	for (int idx=0; idx<EEPROM_SIZE; idx++) {
		if (read_eeprom(idx) != 0xff) return;		// should be very fast after it's first setup (the test program itself will do!)
	}

	// no systems initialized - set up a default to load the pre-programmed image
	// we load the first TWO grom banks, so up to 16k (cause my test is slightly too large for one.)
#ifdef SERIAL_DEBUG
	printserial("EEPROM loading default settings...\r\n");
#endif

	write_eeprom(0x00, 0x00);	// no bases
	write_eeprom(0x01, 0xff);	// confirm byte 0
	write_eeprom(0x05, 0x11);	// second bank of GROM to >6000 - AVR studio has a bug that makes linking to 0x0000 hard
	write_eeprom(0x0D, ~0x11);	// confirm
	write_eeprom(0x06, 0x12);	// third bank of GROM to >8000
	write_eeprom(0x0E, ~0x12);	// confirm
}

int EepromRead(unsigned char page, unsigned int address) {
	// special case - if we're in the base config block then we are offset by 0x1800
	if (page == 0xff) {
		page=0;
		address -= 0x1800;
	}

	if (page > 0) {
		// only one page
		return -1;
	}
	if (address >= EEPROM_SIZE) {
		// out of range
		return -1;
	}

	return read_eeprom(address);
}

void EepromWrite(unsigned char page, unsigned int address, unsigned char data) {
	// special case - if we're in the base config block then we are offset by 0x1800
	if (page == 0xff) {
		// check unlock sequence
		if (address == 0x1fff) {
			switch (nLocked) {
				case 0x01:	// nothing yet
					if (data == 0x55) {
						nLocked = data;
					} else {
						nLocked = 1;
					}
					break;
				case 0x55:	// step 1
					if (data == 0xaa) {
						nLocked = data;
					} else {
						nLocked = 1;
					}
					break;
				case 0xaa:	// step 2
					if (data == 0x5a) {
						nLocked = data;
					} else {
						nLocked = 1;
					}
					break;
				case 0x5a:	// unlocked
					if (data != 0x5a) {
						nLocked = 1;	// relock
					}
					break;
				default:
					// whatever it was, reset it
					nLocked = 1;
					break;
			}
			return;
		}

		page=0;
		address -= 0x1800;
	}

	if (page > 0) {
		// only one page
		return;
	}
	if (address >= EEPROM_SIZE) {
		// out of range
		return;
	}

	if (nLocked != 0x5a) {
		// locked - break any unlock sequence in progress
		nLocked = 1;
		return;
	}

	// if it's in the configuration space, check
	// the hardware write protect, and disallow it if set
	// note we just lock the whole space, ignoring bases config
	// If someone REALLY needs the extra 200 bytes, we can
	// just make a special build rather than slowing down
	// every write. ;)
	if (address < 0x0102) {
		// first check for write protection bit
		if ((PINC & _BV(7)) == 0) {
			// pin is ground, don't allow update
			return;		// write protect
		}
	}

	write_eeprom(address, data);
}

// assumes that a write is never in progress!
unsigned char read_eeprom(unsigned int index) {
	unsigned char nRet;

	cli();

		// set address register
		EEAR = index;
		// set EEPROM read bit
		EECR |= (1<<EERE);
		// return data (CPU halts for 4 cycles)
		nRet = EEDR;

	sei();

	return nRet;
}

// Blocking function - takes over 3ms!!
// Assumes a write is never in progress on
// entry. Also assumes a flash write never happens,
// as we can't write both at once!
void write_eeprom(unsigned int index, unsigned char data) {
	cli();
	
		// set address register
		EEAR = index;
		// set data register
		EEDR = data;
		// write to the master programming enable bit
		EECR |= (1<<EEMPE);
		// we have just 4 cycles now to write to the program enable bit! (CPU halts for 2 cycles)
		EECR |= (1<<EEPE);
	
	sei();

	// now wait for the program to finish - we are also holding the TI, so this is simplest and
	// ensures the operation will finish before anything else happens.
	while (EECR & (1<<EEPE)) { }
}


HANDLERS EepromHandlers = {
	EepromInit, EepromRead, EepromWrite
};	


