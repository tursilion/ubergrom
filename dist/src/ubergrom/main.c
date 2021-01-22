// UberGROM simulation - run on internal clock, ATMEGA1284P
// This version supports multiple GROM bases and various
// hardware as described in the GROM+ Interface manual
//

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

// Pin assignments
// 
// PA0 - ADC0
// PA1 - ADC1
// PA2 - ADC2
// PA3 - ADC3
// PA4 - TI Data 3			36
// PA5 - TI Data 2			35
// PA6 - TI Data 1			34
// PA7 - TI Data 0 (MSB)	33
//
// PB0 - TI Data 7 (LSB)	1
// PB1 - TI Data 6			2
// PB2 - TI Data 5			3
// PB3 - TI Data 4			4
// PB4 - SPIO *SS  (for ISP)
// PB5 - SPIO MOSI
// PB6 - SPIO MISO
// PB7 - SPIO SCK
// 
// PC0 - DBIN/MDIR in		22
// PC1 - GREADY out			23
// PC2 - *GSEL in			24
// PC3 - A14/Mode in 		25
// PC4 - GPIO Pin 26	
// PC5 - GPIO Pin 27
// PC6 - GCLK in			
// PC7 - Tie to ground for write protect (so use internal pull-up)
// 
// PD0 - RXD0				14
// PD1 - TXD0				15
// PD2 - A13	/ RXD1		16		- RXD1 is used in debug mode, number of bases is reduced to 4!
// PD3 - A12	/ TXD1		17		- TXD1 is used in debug mode, number of bases is reduced to 4!
// PD4 - A11				18
// PD5 - A10				19
// PD6 - GPIO Pin 20
// PD7 - GPIO Pin 21

// Note: this version treats all blocks as 8k, not 6k,

// There is no guarantee of sane behaviour if you map two devices to the same
// space, even if you think it should work. You have lots of address space,
// please don't attempt deliberate collisions.

// the GROM bus is pulled up in the console. To reduce conflicts with future devices,
// this system will not allow you to conflict with the console GROM space (0000,2000,4000)
// and it will not. Change this behaviour at your own risk but please do not distribute
// cartridges that do so (my own MPD will use a variant of this code inside the console,
// the conflict could damage it, your cartridge, or both). If you must, though, search
// for OVERRIDE_CONSOLE_GROM below to find the code to remove.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sfr_defs.h>
#include <string.h>

// note: although I've broken the usage up a bit, all the hardware init happens
// in this file, so to change the pins around you have to modify both places!
// in addition, the actual GROM emulation happens in this file, the grom.c/h
// files have to do soley with returning the correct data. To port to a smaller
// chip, it should be easy to remove unneeded functionality, and then of course
// handle the pin remap if necessary.

#include "main.h"
#include "adc.h"
#include "eeprom.h"
#include "flash.h"
#include "gpio.h"
#include "grom.h"
#include "ram.h"
#include "timer.h"
#include "uart.h"
#include "boothack.h"

// our local data
unsigned int GRMADD;						// address counter
unsigned char GRMBASE;						// address bits on access, which GROM base!
unsigned char HandlerPage;					// which page is being accessed on a handler
unsigned char Override=0xff;				// detected loader override to enable a built-in EasyBug/#5 Loader - hold space at POWER UP (not reset)
unsigned char Rollover=0;					// whether to allow address rollover

const HANDLERS *HandlerType[] = {			// do NOT reorder these!
	&RamHandlers,
	&GromHandlers,
	&EepromHandlers,
	&GpioHandlers,
	&AdcHandlers,
	&UartHandlers,
	&FlashHandlers,
	&TimerHandlers
};

// TI D0 is LSB, not MSB - so watch that in the datasheets!

inline void GromNotReady() {
	// Set not ready when we are not selected and during processing of commands
	// Set GREADY to output, data low
	// GREADY = C1
	DDRC |= _BV(1);		// Port to output	- docs says that GCC should optimize these to the single-instruction opcodes
	PORTC &= ~(_BV(1));	// data low
}

inline void GromReady() {
	// Set ready when we have completed processing the current command - either the
	// address is loaded, or data is on the output ports
	// Set GREADY to input, floating (no pullup)
	DDRC &= ~(_BV(1));	// port to input
	PORTC &= ~(_BV(1));	// turn off pullup (should be anyway)
}

// only test against 0 (0 == write, ~0 == read)
inline unsigned char GetMDIR() {
	return (PINC & _BV(0));
}

// only test against 0 (0 == data, ~0 == address)
inline unsigned char GetMODE() {
	return (PINC & _BV(3));
}

// only test against 0 (0 == active)
inline unsigned char GetGSEL() {
	return (PINC & _BV(2));
}

// returns 0-15
inline unsigned char GetGromBase() {
	// TI bit ordering is reversed, but
	// we did the reversal in hardware like
	// we should have...
#ifdef SERIAL_DEBUG
	// if serial debug is on, we lose the top two pins.
	// to make the rest of the code behave better, we map the four available
	// Port specific code will still fail, though
	// ports to 0,1,14,15
	unsigned char x = PIND & 0x30;
	if (x&0x20) {
		return 0xE | ((x>>4)&1);
	} else {
		return ((x>>4)&1);
	}
#else
	// all 16 ports available otherwise
	return (PIND & 0x3c)>>2;
#endif
}

inline void GromSetData(unsigned char nVal) {
	// We have the data lines nicely reversed, but they
	// are split among two ports. Yuck. 
	// They seem to line up at least!

	// PA4 - TI Data 3
	// PA5 - TI Data 2
	// PA6 - TI Data 1
	// PA7 - TI Data 0 (MSB)
	DDRA |= 0xF0;		// data bits to output
	PORTA = (PORTA & 0x0F) | (nVal & 0xF0);

	// PB0 - TI Data 7 (LSB)
	// PB1 - TI Data 6
	// PB2 - TI Data 5
	// PB3 - TI Data 4
	DDRB |= 0x0F;		// data bits to output
	PORTB = (PORTB & 0xF0) | (nVal & 0x0F);
}

inline void GromUnsetData() {
	// return the data lines to floating, as quickly as possible
	DDRA &= ~0xF0;				// data bits to input
	DDRB &= ~0x0F;				// data bits to input
	PORTA = (PORTA & 0x0F);		// no pullups
	PORTB = (PORTB & 0xF0);		// no pullups
}

inline unsigned char GromReadData() {
	// assumes the data lines are in input mode
	return ((PINA & 0xF0) | (PINB & 0x0F));
}

inline void GromAddressIncrement() {
	if (!Rollover) {
		// don't increment the top three bits, wrap around within the GROM
		GRMADD = (GRMADD&0xe000) | ((GRMADD+1)&0x3FFF);
	} else {
		// if the user does want rollover, do a little more work.
		// for safety, I don't want to allow rollover to or from
		// the console GROMs.
		// OVERRIDE_CONSOLE_GROM
		// if you are overriding the console GROMs, you might care
		// about this. If you don't know if you care, you probably
		// should not change it.
		// OVERRIDE_CONSOLE_GROM
		
		if ((GRMADD < 0x6000) || (GRMADD == 0xffff)) {
			// don't increment the top three bits, wrap around within the GROM
			// this keeps us in sync with the console GROMs
			GRMADD = (GRMADD&0xe000) | ((GRMADD+1)&0x3FFF);
		} else {
			// user wants it, and it's all in our space. Note
			// that the moment we leave a GROM space, our address
			// counter will be out of sync. But since we don't emulate
			// the prefetch it's out of sync anyway.
			GRMADD++;
		}
	}
}

int low_level_init(void)
{
	// disable interrupts
	cli();

	// Port A: 0-3 = ADC, 4-7 = data (tristate)
	DDRA 	= 0x00;		// all input
	PORTA	= 0x00;		// all pullups off
	
	// Port B: 0-3 = data (tristate), 
	// 			4 - SPIO *SS
	// 			5 - SPIO MOSI
	// 			6 - SPIO MISO
	// 			7 - SPIO SCK
	DDRB	= 0x80;		// all input
	PORTB	= 0x00;		// all pullups off (is this correct?)

	// Port C: 
	//			0 - DBIN/MDIR in - input floating, console has 3.3k pullup
	//			1 - GREADY out - open collector output (default to output low), console has 4.7k pullup and gates this line
	//			2 - *GSEL in - input floating, console has 2.2k pullup
	//			3 - A14/Mode in - input floating, console has 2.2k pullup
	//			4 - GPIO pin 26 - default to input with pullup
	//			5 - GPIO pin 27 - default to input with pullup
	//			6 - GCLK in - input floating, but not used, console has 1k pullup
	//			7 - input for GROM write protect (use pullup)
	DDRC	= 0x02;		// all input for now except GREADY
	PORTC	= 0xB0;		// pullup on Write Protect and GPIO, output data low

	// Port D:
	//			0 - RXD0
	//			1 - TXD0 - USART connection
	//			2 - TI A13
	//			3 - TI A12
	//			4 - TI A11
	//			5 - TI A10 - base port selection (remember A10 is most significant bit, A13 is least)
	//			6 - GPIO pin 20
	//			7 - GPIO pin 21
	DDRD	= 0x40;		// all input except for TXD
	PORTD	= 0x03;		// pullups on GPIO lines

/////////////////////////////////////////////////////////////////////
	
	// Make sure GROM is set not ready as soon as possible
	GromNotReady();

	// configure power registers (a tiny power savings)
	// Note we are using Timer1, SPI, USART0, USART1, and ADC
	SMCR=0;			// no sleep
	PRR1=0x01;		// disable TIMER3
	PRR0=0xE0;		// disable TWI (80), Timer2 (40), timer0 (20)

	// set up our emulated devices
	RamHandlers.Init();
	GromHandlers.Init();
	EepromHandlers.Init();
	GpioHandlers.Init();
	AdcHandlers.Init();
	UartHandlers.Init();
	TimerHandlers.Init();
	BoothackHandlers.Init();

	// point vector table into bootloader space
	MCUCR = (1<<IVCE);		// enable change of vector pointer
	MCUCR = (1<<IVSEL);		// move to boot space - thse must be distinct, and they must be side-by-side!

	// enable interrupts
	sei();

//	putserialchar('!');

	return 1;
}

// NOTE: This is called for EVERY ACCESS. Don't waste time! (Reading EEPROM should be quick enough!)
const HANDLERS *GetDevicePointer() {
	// read the configuration against the GROM address to decide which device needs to handle this request
	// Return NULL if there is no handler for that address.
	// we just access the global, no need to pass it around in this file...
	unsigned char x, y, nType;
	unsigned int nAddress;
	unsigned char nBase;

	// OVERRIDE_CONSOLE_GROM
	// this code disallows override of the console GROMs
	// simply remove this check to make it legal
	if (GRMADD < 0x6000) return NULL;
	// end OVERRIDE_CONSOLE_GROM

	nBase = GetGromBase();

	// special case 1 - if the address is >F800->FFFF, on base 15, it is ALWAYS EEPROM
	if ((nBase == 0x0F) && (GRMADD >= 0xF800) && (GRMADD <= 0xFFFF)) {
#ifdef SERIAL_DEBUG
		printserial("Special case configuration EEPROM access\r\n");
		print_hexbyte(GRMADD>>8);
		print_hexbyte(GRMADD&0xff);
		printserial("\r\n");
#endif
		HandlerPage = 0xff;							// 'special case' flag
		return &EepromHandlers;
	}

	x=read_eeprom(0);								// global configuration byte, to determine whether to look at bases
	y=read_eeprom(1);								// get inverse
	y=~y;

	if (x != y) {
#ifdef SERIAL_DEBUG
		putserialchar('?');
#endif
		x=0;										// reset if invalid
	}
 
	if (Override != 0) {
		// handling the built-in override. Since this happens on EVERY access, we want to
		// try and be as quick as we can to rule it out
		if (x&2) {
			Override=0;	// never, so speed up later accesses
		} else {
			// see if we are still detecting powerup routine on >Exxx
			if ((Override&0x80) && ((GRMADD&0xE000) == 0xE000) && (nBase == 0)) {
				// this is the first pass test for >Exxx, map through to our code so that we can check the keyboard and set the result
				HandlerPage=(GRMADD&0xE000)>>8;		// save off the high bits for the hack to re-use
				return &BoothackHandlers;
			}
			// only after the mode is set do we check for override of >6000
			// by returning for bases 0 and 1 we don't get the 'review module library' option
			if ((Override == 0x01) && ((GRMADD&0xE000) == 0x6000) && ((nBase == 0)||(nBase == 1))) {
				// this is the first pass test for >Exxx, map through to our code so that we can check the keyboard and set the result
				HandlerPage=(GRMADD&0xE000)>>8;		// save off the high bits for the hack to re-use
				return &BoothackHandlers;
			}
		}
	}

	if (x&1) {
		nAddress=(nBase<<4)+(GRMADD>>13)+2;	// address of configuration byte to read
	} else {
		nAddress=(GRMADD>>13)+2;			// no bases, address of configuration byte to read (ADD/8192)
	}

	// save off the rollover status so we don't have to read it again
	if (x&4) {
		Rollover = 1;
	} else {
		Rollover = 0;
	}

	x=read_eeprom(nAddress);				// get config
	y=read_eeprom(nAddress+8);				// get verification
	y=~y;


#ifdef SERIAL_DEBUG
	if (GetMDIR()) {
		printserial("Reading");
	} else {
		printserial("Writing");
	}

	printserial(" address ");
	print_hexbyte(GRMADD>>8);
	print_hexbyte(GRMADD&0xff);
	printserial(" base ");
	print_hexbyte(nBase);
	printserial(" config byte ");
	print_hexbyte(x);
	printserial(" confirm ");
	print_hexbyte(y);
#endif

	if (x != y) {
#ifdef SERIAL_DEBUG
		printserial("\r\n");
#endif
		return NULL;						// no valid handler configured
	}

	nType=(x>>4)&0xf;						// get the actual handler type configured
	HandlerPage=x&0xf;						// save this for later

	if (nType >= (sizeof(HandlerType)/sizeof(const HANDLERS*))) {
		// invalid type configured
#ifdef SERIAL_DEBUG
		printserial("\r\n");
#endif
		return NULL;
	}

	return HandlerType[nType];
}

extern int GromRead(unsigned char page, unsigned int address);

int main() {
	const HANDLERS *pHandler = NULL;

	low_level_init();
	GRMADD=0;
	GromNotReady();		// GROM's are not ready by default. They are only ready briefly after each operation.

#ifdef SERIAL_DEBUG
	printserial("GO->\r\n");
#endif

	// off we go!
	for (;;) {
		// first, wait for GSEL to go low 
		while (GetGSEL() != 0)	{}

		unsigned char nAct = (GetMODE() ? 2 : 0) | (GetMDIR() ? 1 : 0);


#ifdef SERIAL_DEBUG
//		putserialchar(nAct+'0');
//		putserialchar('-');
//		print_hexbyte(GRMADD>>8);
//		print_hexbyte(GRMADD&0xff);
#endif

		// check what we are doing
		switch (nAct) {		// 0 - write data, 1 - read data, 2 - write address, 3 - read address
			case 0:	// write data
				pHandler = GetDevicePointer();
				if (NULL != pHandler) {
					pHandler->Write(HandlerPage, GRMADD&0x1fff, GromReadData());

#ifdef SERIAL_DEBUG
					putserialchar('{');
					print_hexbyte(GromReadData());
					putserialchar('}');
					printserial("\r\n");
#endif
				}
				GromAddressIncrement();
				break;

			case 1:	// read data
				pHandler = GetDevicePointer();
				if (NULL != pHandler) {
					int nVal=pHandler->Read(HandlerPage, GRMADD&0x1fff);
					if (nVal != -1) {					// int return of -1 (not char) means don't return the value to the console
						GromSetData(nVal);
#ifdef SERIAL_DEBUG
						putserialchar('{');
						print_hexbyte(nVal);
						putserialchar('}');
						printserial("\r\n");
#endif
					}
				}
				GromAddressIncrement();
				break;

			case 2:	// write address
				// we default to input on PortA, so we can just read PINA
				GRMADD<<=8;
				GRMADD|=GromReadData();
				break;

			case 3:	// read address (this function actually can not work without proper prefetch emulation)
					// it will work 90% of the time, but there are certain cases that happen in actual software
					// in which it will fall out of sync, and the software will crash.
					// because we are not doing that (to simplify greatly the hardware interface), you will
					// always need at least one REAL GROM in your system. This particular system is intended
					// for use in a cartridge, so that should be acceptable.
					// Note that this means this address CAN BE WRONG. But only if software reads the address
					// without then setting it, which is an illegal operation anyway.
//				GromSetData((GRMADD&0xff00)>>8);					// set the top data byte (note our tracked address is off by 1)
				GRMADD = ((GRMADD&0xff)<<8) | (GRMADD&0xff);		// destructive read - copy the low byte (modified) to the high byte
				break;
		}

		// all done - tell the console we are ready - block interrupts
		// interrupts occuring while we were 'ready' were causing us to miss cycles!
		cli();
			GromReady();

			// wait for the select to go away - allow interrupts in the middle of it
			// this ensures they are defintely off when we detect the signal is cleared
			while (GetGSEL() == 0) 	{	sei(); cli(); }		// hopefully this is okay... should be cause it should only let one interrupt through?

			// release data bus immediately - the console does not wait for us
			GromUnsetData();

			GromNotReady();		// go back to not ready

		// now interrupts are okay again
		sei();
	}
}

// this function is used to save some memory not having to define empty functions for all the classes,
// they can all just share these empty instances
void DummyInit() { }
void DummyWrite(unsigned char page, unsigned int address, unsigned char data) { }

