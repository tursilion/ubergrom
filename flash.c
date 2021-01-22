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
#include <avr/boot.h>
#include <string.h>
#include "main.h"
#include "flash.h"
#include "grom.h"

unsigned char nBase = 0;	// which 8k GROM base we are using (0-14)
unsigned char nPage = 0;	// subpage within an 8k GROM block (0-31)
unsigned char ByteBuffer;	// used to make words for the programming
unsigned char nError;

void FlashInit() {
	nError = 0;
}

int FlashRead(unsigned char page, unsigned int address) {
	// registers are all shared, so, we don't care about the page index here
	if (address > 0x0104) {
		return -1;		// invalid address
	}
	if (address < 0x0100) {
		// instead of undefined, return hard-coded 0 for the buffer
		// (helps with the console multiple GROM bases bug)
		return 0;
	}

	if (address == 0x0100) {
		// subpage select
		return nPage;
	}
	if (address == 0x0103) {
		// result code
		// first check for write protection bit
		if ((PINC & _BV(7)) == 0) {
			// pin is ground, don't allow flash
			return 0x02;		// write protect
		}

		if (SPMCSR & 0x01) {
			return 1;			// busy (I don't think it's possible to see this.... maybe it will be with bootloader)
		}

		return nError;
	}
	if (address == 0x0104) {
		return nBase;
	}

	// nothing else to read!
	return -1;
}

// We live in the bootloader section to allow writes to flash
void FlashWrite(unsigned char page, unsigned int address, unsigned char data) {
	uint32_t flash_page;

	// what will the flash page address be?
	// they are 256 bytes long
	flash_page = ((uint32_t)(((unsigned int)nBase*32)+nPage))*256;

	if (address > 0x0104) {
		nError = 3;
		return;		// invalid address
	}
	if ((PINC & _BV(7)) == 0) {
		// pin is ground, don't allow flash
		nError = 2;
		return;		// write protected
	}

	if (address == 0x0100) {
		// subpage select
		nPage = data;
		return;
	}

	if (address == 0x0101) {
		ByteBuffer = data;		// save the requested command
		nError = 0;
		return;
	}

	if (address == 0x0102) {
		data = ~data;
		if (data == ByteBuffer) {
			// this is a command
			if (data == 0x31) {
				// ERASE BLOCK
				nError = 0;
				cli();
					boot_page_erase_safe(flash_page);	// will this hang the processor? which in turn will probably hang the TI. Okay.
					boot_spm_busy_wait();				// either way we wait till it's done, in theory.
				sei();
			} else if (data == 0xd2) {
				// WRITE BUFFER
				nError = 0;
				cli();
					boot_page_write_safe(flash_page);
					boot_rww_enable_safe();
				sei();
			} else {
				nError = 3;
			}
		} else {
			nError = 3;
		}
		return;
	}

	if (address == 0x0104) {
		if (data >= GROMPAGES) {
			nError = 3;	// bad address
			return;
		}
		nBase = data;
		return;
	}

	if (address < 0x100) {
		// writing to the flash buffer
		if (address & 0x01) {
			// odd value - combine with the byte buffer and write it
			unsigned int nVal;
			// The example code uses little-endian words, so be it!
			nVal = ((unsigned int)data<<8) | ByteBuffer;
			// WRITE nVal to the buffer
			boot_page_fill(flash_page + address-1, nVal);	// byte offset, but you have to write words?
		} else {
			// even value, just remember it
			ByteBuffer=data;
		}
		return;
	}
}

HANDLERS FlashHandlers = {
	FlashInit, FlashRead, FlashWrite
};	

