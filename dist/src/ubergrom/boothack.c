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

// Implementation file for Boothack data
// This handles first-boot mapping so that we get a EA loader and Easy Bug if you hold space at powerup

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "main.h"
#include "grom.h"

// cartrdige data used if we are enabled after all
// the main code decides if we are enabled and need to be called, so
// we can just check addresses
extern const unsigned char GROM6000[] PROGMEM;
extern const unsigned char GROM70A0[] PROGMEM;
extern unsigned char Override;

extern const HANDLERS *GetDevicePointer();

const unsigned char minirom[] PROGMEM = {
	0xaa,0x01, 		// valid id
	0x00,0x00,		// programs
	0xe0,0x0c,		// power up address (our hook!)
	0x00,0x00,		// program list
	0x00,0x00,		// DSR list
	0x00,0x00,		// subprogram list

// powerup header - 0xE00C
	0x00,0x00,		// link to next powerup -- we have to override this with the user's data!
	0xe0,0x10,		// address of this one

// powerup GPL code - 0xE010 - test keyboard for space bar - we monitor the code path to enable/disable the hack
	0xbe,0x74,0x05,	// ST >05,@>8374
	0x03,			// SCAN
	0xd6,0x75,0x20,	// CEQ >20,@>8375
	0x40,0x1A,		// BR >E01A
	
// E019
	0x0A,			// GT	-- key was pressed if this reached >E019 (GT is to have something to jump over)
					// Continue as per Thierry's site
// E01A
	0xbd,0x90,0x73,
	0x90,0x72,		// DST *>8372,*>8373	Transfer address from data stack (72) to sub stack (73)
	0x96,0x72,		// DECT @>8372			decrement data stack pointer

// E021
	0x00			// RTN					branches to next powerup routine
};

int BoothackRead(unsigned char page, unsigned int address) {
	unsigned char x=0;		// default return is 0
	unsigned int adr=0;

	// the page contains the high order bits of the actual address in this case (not true anywhere else)
	address |= (page<<8);

	if (address >= 0xe000) {
		// anything else in the minirom is a direct dump
		// we're all in the second block
		adr = (unsigned int)&minirom[address-0xe000];
	} else if (address >= 0x70a0) {
		// if it's in one of the >6000 ranges, then return the big ROM
		adr = (unsigned int)&GROM70A0[address-0x70a0];
	} else if (address >= 0x6000) {
		adr = (unsigned int)&GROM6000[address-0x6000];
	}
	if (adr > 0) {
		x = pgm_read_byte_far((uint32_t)0x10000 + adr);
	}

	// check for the magic vectors
	if (Override == 0xff) {
		// whichever one we see first, basically!
		if (address == 0xe019) {
			Override = 0x81;		// we will be active!
		}
		if (address == 0xE01A) {	// this one is always hit, so we have to check for hitting e019 first.	
			Override = 0x80;		// we will not be active
		} 
	} else if (Override & 0x80) {
		if (address == 0xE00D) {	// leave it up till the next powerup link vector is read - see below
			Override&=0x0f;
		}
	}
	
	return x;
}


HANDLERS BoothackHandlers = {
	DummyInit, BoothackRead, DummyWrite
};	

// This code does assume a standard startup sequence (would be break under the ROS menu?)
// The expected flow is as follows:
//
// First action we care about, is the system enumerates power-up routines, and finds the
// temporarily active block of code at >E000. It executes the Powerup function, which
// calls SCAN and checks against the space bar. If space is not pressed, it skips over a
// GT instruction which serves no purpose other than to occupy a memory address.
//
// The uC monitors the GROM addresses accessed. If the GT instruction at E019 is requested,
// we set the activate flag. If we reach E01A first, then we assume it was jumped over, and
// set the de-activate flag. The high bit of Override remains set because we must continue
// to prove valid data until the console is done with it.
// 
// The standard 'next link' code is then executed, and RTN branches ahead. The console then
// requests the 'next powerup link' vector at >E00C, which we set to >0000. When the second
// half of it at >E00D is requested, we turn off the high bit of Override, and the >Exxx override
// block is now permanently disabled. Whether >6000 is overridden depends on whether Space
// was detected earlier.

