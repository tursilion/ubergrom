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
// UberGROM loader application

// uses libti99
#include "vdp.h"
#include "grom.h"
#include "kscan.h"
#include "files.h"
#include "string.h"

int vieweroff;
int vieweridx;
int grmbase;
struct PAB pabdat;

// buffer needs 8k+6 bytes for GRAM Kracker header
#define VDP_BUFFER 0x1000
// PAB needs to load into VDP too (overlaps end of char table)
#define VDP_PAB 0x0FD0

#ifndef bool
#define bool int
#define false 0
#define true 1
#define FALSE 0
#define TRUE 1
#endif
#ifndef NULL
#define NULL 0
#endif

// extra character data (20-31)
const unsigned char extrachars[] = {
	0x00,0x04,0x06,0x7F,0x06,0x04,0x00,0x00,
	0x10,0x38,0x7C,0x10,0x10,0x10,0x10,0x10,     
	0x08,0x08,0x08,0x08,0x08,0x3E,0x1C,0x08,     
	0x08,0x08,0x0C,0x0F,0x0C,0x08,0x08,0x08,     
	0x00,0x00,0x00,0xE0,0x10,0x08,0x08,0x08,     
	0x00,0x00,0x00,0x03,0x04,0x08,0x08,0x08,     
	0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,     
	0x08,0x08,0x10,0xE0,0x00,0x00,0x00,0x00,     
	0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,     
	0x00,0x20,0x60,0xFF,0x60,0x20,0x00,0x00,     
	0x00,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C,0x7C,     
	0x08,0x08,0x04,0x03,0x00,0x00,0x00,0x00
};
 
#ifndef LOADONLY
// help text (1,13 - 8,28)
const unsigned char *helptext[] = {
	"\x1d\x14 Change base",
	"\x15\x16 Scroll viewer",
	      "V  Select viewer",
		  "L  Load viewer",
		  "S  Save viewer",
		  "^L Load device",
		  "^S Save device",
		  "F7 Advanced"
};

// advanced menu text (same place as help text)
const unsigned char *advtext[] = {
	"  Advanced Menu",
	"",
	"^B Bases:",
	"^R Recovery:",
	"^G Rollover:",
	"^F Flash Dev:",
};

// select help (2,15 - 6,31)
const unsigned char *selecttext[] = {
	"Grom (15) Flash",
	"Eeprom    Timer",
	"I/o       Ram (2)",
	"Adc (4)",
	"Uart      None"
};

// names of the various devices, in correct indexed order
const unsigned char* devices[] = {
	"RAM",
	"GROM",
	"EEPROM",
	"I/O",
	"ADC",
	"UART",
	"Flash",
	"Timer"
};

// number of pages supported by each device, in the same order
const unsigned char pages[] = {
	2,
	15,
	1,
	1,
	4,
	1,
	1,
	1
};

const unsigned char *basestr = "   68ACE";
#endif

// draws a rounded box
void drawbox(int r1,int c1,int r2,int c2) {
	hchar(r1,c1,28,c2-c1);
	hchar(r2,c1,28,c2-c1);
	vchar(r1,c1,26,r2-r1);
	vchar(r1,c2,26,r2-r1);
	hchar(r1,c1,25,1);
	hchar(r1,c2,24,1);
	hchar(r2,c1,31,1);
	hchar(r2,c2,27,1);
}

// blanks an area of the screen
void clrregion(int r1,int c1,int r2,int c2) {
	int len=c2-c1+1;
	for (int i=r1; i<=r2; i++) {
		hchar(i,c1,32,len);
	}
}

// draws the help box
void showhelp() {
#ifndef LOADONLY
	drawbox(0,12,9,31);
	clrregion(1,13,8,30);
	for (int i=1; i<9; i++) {
		writestring(i,13,(char*)helptext[i-1]);
	}
#endif
}

// erase the help box
void erasehelp() {
#ifndef LOADONLY
	clrregion(0,12,9,31);
#endif
}

#ifndef LOADONLY

// erase the viewer area
void eraseviewer() {
	clrregion(11,0,22,31);
}

// write 4 digits of hex
void writehex(int row, int col, unsigned int x) {
	unsigned int tmp = VDP_SCREEN_POS(row,col) + gImage;
	VDP_SET_ADDRESS_WRITE(tmp);

	// use the fast byte translation
	tmp = byte2hex[x>>8];
	VDPWD = (tmp>>8);
	VDPWD = (tmp & 0xff);

	tmp = byte2hex[x&0xff];
	VDPWD = (tmp>>8);
	VDPWD = (tmp & 0xff);
}

// draws the GROM mapping box and viewer borders
void drawfixed() {
	char x[3];
	
	// GROM box
	drawbox(0,0,9,11);
	clrregion(1,1,8,10);
	writestring(1,1,"base:");
	x[1]=':';
	x[2]='\0';
	for (int idx=3; idx<8; idx++) {
		x[0]=basestr[idx];
		writestring(idx,1,x);
	}

	// viewer
	drawbox(10,0,23,31);
	eraseviewer();
	hchar(10,2,32,4);
	writestring(10,10,"Viewer");
	hchar(10,21,32,9);
}
#endif

unsigned char GetConfigByte() {
	unsigned char x,y;

	x = GromReadData(0xF800, 15);
	y = GromReadData(0xF801, 15);
	y = ~y;
	if (x != y) {
		x=0;
	}

	return x;
}

void WriteConfigByte(unsigned char x) {
	GromWriteData(0xf800,15,x);
	x=~x;
	GromWriteData(0xf801,15,x);
}

unsigned char GetLastBank() {
	unsigned char x,y;

	x = GromReadData(0xF8F9, 15);
	y = GromReadData(0xF8F9+8, 15);
	y = ~y;

	if (x != y) {
		x=0xff;
	}
	
	return x;
}

void lockeeprom() {
	GromWriteData(0xffff, 15, 0);		// writing non-0x5A to the lock byte ALWAYS locks the EEPROM
}

void unlockeeprom() {
	GromWriteData(0xffff, 15, 0x55);
	GromWriteData(0xffff, 15, 0xaa);
	GromWriteData(0xffff, 15, 0x5a);
}

// forced an assumed configuration
// We are allowed to manually set an INVALID config
// (that is our purpose in life!) But a valid config
// that just isn't what we want, we have to ask for.
// Pass 1 to force it, 0 to just check
// Returns:
// 0 - no changes were needed
// 1 - configuration was valid but unsupported for tool (go to advanced)
// -1 - configuration was invalid, defaults loaded
int forcedefaultcfg(int force) {
	unsigned char x,y;
	int ret = 0;
		
	unlockeeprom();

	// - set config byte (and mirror) to 01 if it's not (enable GROM ports, enable recovery program)
	// I read explicity here and not with GetConfigByte because I want to correct it if it's not matching
	x = GromReadData(0xF800, 15);
	y = GromReadData(0xF801, 15);
	y = ~y;
	if (force) y = ~x;	// make it look unconfigured

	if (x != y)  {
		// it's invalid (blank EEPROM?), just write it
		WriteConfigByte(0x01);
		ret = -1;		// remember we did this
	} else if ((x&0x01) == 0) {
		// bases are disabled, we need them
		ret = 1;
	}

	// - map the flash peripheral to >E000, port 15 (last slot)
	x = GromReadData(0xF8F9, 15);
	y = GromReadData(0xF8F9+8, 15);
	y = ~y;

	if ((x != y) && (ret == -1)) {
		// was unconfigured, so finish setting it up
		GromWriteData(0xF8F9, 15, 0x60);
		GromWriteData(0xF8F9+8, 15, ~0x60);
	} else if ((x != y) || (x != 0x60)) {
		// no flash device where we want it
		ret = 1;
	}

	lockeeprom();

	return ret;
}

void waitkeyfree() {
	volatile unsigned char x;

	if (KSCAN_KEY != 0xff) {
		do {
			x = kscan(5);
		} while (x != 0xff);
	}
}

void waiterror(int row) {
#ifdef LOADONLY
	writestring(row,9, "Press space.");
#else
	writestring(row,13, "Press space.");
#endif
	while (32 != kscan(5)) { 
		// briefly enable interrupts to allow QUIT
		VDP_INT_ENABLE;
		VDP_INT_DISABLE;
	}
	waitkeyfree();
	showhelp();
}

// find out what this device is set to in the EEPROM, current base
int grommap(int idx) {
	// configuration is always available at >F800 in port 15
	unsigned char x,y;
	// note the shift -- grom bases step by 4, and configuration is 16 bytes per base, so divide by 4 (>>2) and then multiply by 16 (<<4),
	// the overall math is to multiply by four (<<2)
	unsigned int adr = ((grmbase-0x9800)<<2)+0xf802+idx;		// base*16 + 2 == eeprom address of config, + 0xf800 for the config offset

	x=GromReadData(adr, 15);	// get byte
	y=GromReadData(adr+8, 15);	// get mirror
	y=~y;
	if (x != y) {
		return -1;
	} else {
		return x;
	}
}

#ifndef LOADONLY
// display the name associated with a map id
void drawmapname(int row, int col, int map) {
	if (map == -1) {
		writestring(row, col, "      ");
	} else {
		unsigned int dev = (map>>4)&0x0f;
		writestring(row, col, (char*)devices[dev]);
		// note: assumes that writestring leaves the VDP address at the next position
		if (pages[dev] > 1) {
			VDPWD = ':';
			map&=0x0f;
			if (map > 9) {
				VDPWD = map + ('A'-10);
			} else {
				VDPWD = map + '0';
			}
		}
	}
}

// draw the GROM table as mapped
void drawgroms() {
	clrregion(3,4,7,10);
	drawbox(0,0,9,11);

	// display the base
	VDP_SET_ADDRESS_WRITE(gImage + VDP_SCREEN_POS(1,6));
	faster_hexprint(grmbase >> 8);
	faster_hexprint(grmbase & 0xff);

	// display the mapping
	for (int idx=3; idx<8; idx++) {
		int map = grommap(idx);
		drawmapname(idx, 4, map);
	}
}

// draw the viewer display
void drawviewer() {
	char x[8];			// big enough for the text buffer

	writehex(10, 2, vieweroff);

	x[1]=':';
	x[2]='\0';
	x[0]=basestr[vieweridx];
	writestring(10, 21, x);

	unsigned int map = grommap(vieweridx);
	hchar(10,24,32,6);
	drawmapname(10, 24, map);
	if (map != -1) {
		// also read and draw the hex bytes
		// set base address first, rather than every byte
		// 0x402 is the offset between a GROM base and GROM Write Address
		{
			unsigned int address = ((vieweridx-3)<<13)+0x6000+vieweroff;
			volatile unsigned char *pDest = (volatile unsigned char *)(grmbase + 0x402);
			*pDest = address>>8;
			*pDest = address&0xff;
		}

		// set the start address
		VDP_SET_ADDRESS_WRITE(gImage + VDP_SCREEN_POS(11,0));

		for (int i = 0; i<12; i++) {
			// this loop outputs the hex bytes, and saves the characters for the end
			for (int j=0; j<8; j++) {
				unsigned char ch = *((volatile unsigned char*)grmbase);		// GRM READ DATA address
				if ((ch>=' ')&&(ch<127)) {
					x[j] = ch;	// ascii part
				} else {
					x[j] = '.';
				}

				int t = byte2hex[ch];
				VDPWD=(t>>8);
				VDPWD=(t&0xff);

				VDPWD = ' ';
			}
			// output the character bit
			for (int j=0; j<8; j++) {
				VDPWD = x[j];
			}
		}
	} else {
		eraseviewer();
	}
}

void resetviewer() {
	vieweroff=0;
	drawgroms();
	drawviewer();
}

unsigned char GetDevice(unsigned char x) {
	// get a grom slot selection
	// either pass in a key at x, or pass 0 to read a new key
	// note: if you pass an unknown key, it will wait for a new one

	for (;;) {
		switch (x) {
		case 15:	// back FCTN-9
			return 0xff;

		case '6':
			return 0;

		case '8':
			return 1;

		case 'A':
			return 2;
			
		case 'C':
			return 3;

		case 'E':
			return 4;
		}

		// briefly enable interrupts to allow QUIT
		VDP_INT_ENABLE;
		VDP_INT_DISABLE;

		x=kscan(5);
	}
}

unsigned char GetType() {
	// return a new device type - x is the row to draw at
	unsigned char x;

	waitkeyfree();

	for (;;) {
		// briefly enable interrupts to allow QUIT
		VDP_INT_ENABLE;
		VDP_INT_DISABLE;

		x=kscan(5);
		switch (x) {
		case 15:	// back FCTN-9
			return 0xff;

		case 'R':
			return 0;

		case 'G':
			return 1;

		case 'E':
			return 2;

		case 'I':
			return 3;

		case 'A':
			return 4;

		case 'U':
			return 5;

		case 'F':
			return 6;

		case 'T':
			return 7;

		case 'N':
			return 0x80;
		}
	}
}

unsigned char GetPage(unsigned char max) {
	// return a 0-F result, limited by max (0-(max-1))
	unsigned char x;

	waitkeyfree();

	for (;;) {
		// briefly enable interrupts to allow QUIT
		VDP_INT_ENABLE;
		VDP_INT_DISABLE;

		x=kscan(5);

		if (x == 15) {	// back FCTN-9
			return 0xff;
		}
		if ((x>='0')&&(x<='9')) {
			x-='0';
			if (x<max) {
				return x;
			}
		}
		if ((x>='A')&&(x<='F')) {
			x-=('A'-10);
			if (x<max) {
				return x;
			}
		}
	}
}

int advstatus() {
	// draw the advanced status information
	unsigned char x,y;
	int blocked=0;

	x=GetConfigByte();

	// GROM bases enable
	if (x&1) {
		writestring(3,26,"On ");
	} else {
		writestring(3,26,"Off");
		blocked=1;
	}

	// Recovery disable
	if (x&2) {
		writestring(4,26,"Off");
	} else {
		writestring(4,26,"On ");
	}

	// Rollover
	if (x&4) {
		writestring(5,26,"On ");
	} else {
		writestring(5,26,"Off");
	}

	// flash device
	x = GetLastBank();
	if (x != 0x60) {
		writestring(6,26,"Off");
		blocked=1;
	} else {
		writestring(6,26,"On ");
	}

	if (blocked) {
		writestring(8,13,"Back prohibited");
	} else {
		writestring(8,13,"F9 Back        ");
	}

	return blocked;
}

void advanced() {
	int blocked;
	unsigned char x,y;

	drawbox(0,12,9,31);
	clrregion(1,13,8,30);
	for (int i=1; i<7; i++) {
		writestring(i,13,(char*)advtext[i-1]);
	}
	blocked = advstatus();

	for (;;) {
		// briefly enable interrupts to allow QUIT
		VDP_INT_ENABLE;
		VDP_INT_DISABLE;

		waitkeyfree();
		x=kscan(5);

		// check for Back only if allowed
		if ((x == 15) && (!blocked)) {	// back FCTN-9
			return;
		}

		switch (x) {
		case 130:	// bases
			x=GetConfigByte();
			x^=0x01;
			unlockeeprom();
			WriteConfigByte(x);
			lockeeprom();
			blocked = advstatus();
			break;

		case 134:	// flash device
			x = GetLastBank();
			unlockeeprom();
			if (x != 0x60) {
				GromWriteData(0xF8F9, 15, 0x60);
				GromWriteData(0xF8F9+8, 15, ~0x60);
			} else {
				GromWriteData(0xF8F9, 15, 0xff);
			}
			lockeeprom();
			blocked = advstatus();
			break;

		case 135:	// rollover
			x=GetConfigByte();
			x^=0x04;
			unlockeeprom();
			WriteConfigByte(x);
			lockeeprom();
			blocked = advstatus();
			break;

		case 146:	// recovery
			x=GetConfigByte();
			x^=0x02;
			unlockeeprom();
			WriteConfigByte(x);
			lockeeprom();
			blocked = advstatus();
			break;
		}
	}
}
#endif

char getyn() {
	waitkeyfree();

	int k = 0;

	while ((k != 'Y') && (k != 'N')) {
		k = kscan(5);
		if (k >= 'a') k-=('a'-'A');	// make uppercase
	}

	return k;
}

char *getFilename(char *s, bool *gkracker) {
	static char szname[33];	// max filename length of 32 chars
	// note: static char filename! not that we have threads, but not thread safe
	// gkracker is updated to indicate whether or not to strip a 6 byte header

#ifdef LOADONLY
	writestring(10, 5, s);
	writestring(12, 5, "Enter filename:");
	unsigned int fnpos = VDP_SCREEN_POS(14,5);
#else
	erasehelp();
	writestring(1, 13, s);
	writestring(3, 13, "Enter filename:");
	unsigned int fnpos = VDP_SCREEN_POS(4,13);
#endif

	int pos = 0;

	volatile unsigned char k=0;
	waitkeyfree();
	while (k!=13) {
		k = kscan(5);
		if (k != 0xff) {
			switch (k) {
				case 8:		// backspace
					if (pos > 0) {
						--pos;
						vdpscreenchar(fnpos + pos, ' ');
					}
					break;

				case 15:	// fctn-9
					pos=0;
					k=13;	// enter
					break;

				case 13:	// enter
					break;

				default:
					if ((k>=' ') && (k<='z') && (pos < 18)) {
						vdpscreenchar(fnpos + pos, k);
						szname[pos++] = k;
					}
					break;
			}
			waitkeyfree();
		}
	}
	szname[pos] = '\0';

	if (pos == 0) {
		// was cancelled or nothing was entered
		return NULL;
	}

	if (*gkracker) {
		writestring(6,13,"Skip GK Header?");
		
		k = getyn();

		vdpscreenchar(VDP_SCREEN_POS(6,29),k);
		if (k == 'N') *gkracker = FALSE;
	}

	return szname;
}

#ifndef LOADONLY
void loadfile() {
	bool gk = true;

	// Set up the GROM Device
	unsigned int map = grommap(vieweridx);
	if (map == -1) {
		// no memory mapped
		erasehelp();
		writestring(3,13,"No memory mapped");
		waiterror(5);
		return;
	}

	unsigned int page = map & 0xf;
	map >>= 4;

	if ((map != 1) && (map != 2)) {
		// invalid device mapped
		erasehelp();
		writestring(3,13,"GROM or EEPROM");
		writestring(4,13,"only for load.");
		waiterror(6);
		return;
	}

	if (map != 2) {
		// double check GROM flash requirements
		unsigned char x,y;
		x=GromReadData(0xf8f9, 15);		// get byte for last slot on base 15
		y=GromReadData(0xf8f9+8, 15);	// get mirror
		y=~y;
		if (x != y) {
			x = -1;
		}

		// map must be 1 (GROM), so check for write protect
		// the flash controller should be mapped at >E000 on page 15,
		// so check that too.
		if (x != 0x60) {
			erasehelp();
			writestring(3,13,"Flash must be");
			writestring(4,13,"mapped at 983C-E");
			waiterror(6);
			return;
		}
		x = GromReadData(0xE103, 15);		// get the flash status byte
		if (x == 2) {
			erasehelp();
			writestring(3,13,"GROM memory is");
			writestring(4,13,"write-protected");
			waiterror(6);
			return;
		}
	}

	char *name = getFilename("Load Data", &gk);
	if (name == NULL) {
		waitkeyfree();
		showhelp();
		return;
	}

	pabdat.OpCode = DSR_LOAD;
	pabdat.Status = 0;
	pabdat.VDPBuffer = VDP_BUFFER;	// address of the data buffer in VDP memory
	pabdat.RecordLength = 0;		// size of records. Not used for PROGRAM type. >00 on open means autodetect
	pabdat.CharCount = 0;			// number of bytes read or number of bytes to write (per record)
	pabdat.RecordNumber = 8192+6;	// record number for normal files, available bytes (LOAD or SAVE) for PROGRAM type
	pabdat.ScreenOffset = 0;		// Used in BASIC for screen BIAS. Also returns file status on Status call. (DSR_STATUS_xxx)
	pabdat.NameLength = 0;			// for this implementation only, set to zero to read the length from the string
	pabdat.pName = name;			// for this implementation only, must be a valid C String even if length is set

	// zero the buffer
	vdpmemset(VDP_BUFFER, 0xff, 8192+6);

	if (dsrlnk(&pabdat, VDP_PAB)) {
		erasehelp();
		unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
		writestring(3, 13, "DSR Error ");
		faster_hexprint(x);							// relies on VDP address from writestring
		waiterror(5);
		return;
	}

	// got it!
	erasehelp();
	writestring(3,13,"Programming...");

	if (map == 2) {
		erasehelp();
		writestring(3,13,"Loading EEPROM");
		writestring(4,13,"may corrupt");
		writestring(5,13,"device mapping.");
		writestring(6,13,"Continue?");
		if (getyn() == 'Y') {
			erasehelp();
			writestring(3,13,"Programming...");

			// there's a catch with the EEPROM - writing the configuration space
			// will immediately change the configuration. So, we first write
			// from 0x07FF to 0x1000 in the specified space. Then we can write
			// from 0x0000 to 0x07FE up in the config space at 0xf800.

			// Set the VDP read address (skipping header if needed)
			if (gk) {
				VDP_SET_ADDRESS(VDP_BUFFER+6+0x7ff);
			} else {
				VDP_SET_ADDRESS(VDP_BUFFER+0x7ff);
			}

			// now we should be able to just write!
			// lots of volatiles to prevent optimization beyond
			// the timing requirements.
			unlockeeprom();		// corrupts the GROM address

			// 0x402 is the offset between a GROM base and GROM Write Address
			unsigned int address = ((vieweridx-3)<<13)+0x6000+0x7ff;		// start at 0x7ff
			volatile unsigned char *pDest = (volatile unsigned char *)(grmbase + 0x402);
			*pDest = address>>8;
			*pDest = address&0xff;
			pDest -= 2;		// write data address now

			for (int idx=0x7ff; idx<4096; idx++) {
				volatile unsigned char x = VDPRD;
				*pDest = x;
			}

			// fix the addresses back up to start at 2 (we never overwrite bytes 0 and 1)
			if (gk) {
				VDP_SET_ADDRESS(VDP_BUFFER+6+2);
			} else {
				VDP_SET_ADDRESS(VDP_BUFFER+2);
			}
			pDest = (volatile unsigned char *)(0x983c + 0x402);		// base 15
			*pDest = 0xf8;
			*pDest = 0x02;			// fixed config space at 0xf802
			pDest -= 2;				// write data

			// and do the copy
			for (int idx=0; idx<0x7ff-2; idx++) {
				volatile unsigned char x = VDPRD;
				*pDest = x;
			}

			// all done!
			lockeeprom();

			// EEPROM config bytes probably changed, so redraw
			drawgroms();
		}
	} else {
		// all looks good, so let's go ahead and write it
		// Each slot is 32 pages 

		// set the VDP Read address
		if (gk) {
			VDP_SET_ADDRESS(VDP_BUFFER+6);
		} else {
			VDP_SET_ADDRESS(VDP_BUFFER);
		}

		// From the manual, the steps per page are:
		//	1)	Check the result code for write protect (abort if set) (done)
		//	2)	Set the page select register (and slot register, why doesn't it say that?)
		GromWriteData(0xE104, 15, page);		// slot select register
		for (int idx = 0; idx<32; idx++) {		// going to write all 32 pages
			
			// no progress... should be fairly quick and a progress indicator will
			// blow the VDP address on us.

			GromWriteData(0xe100, 15, idx);		// page select register

			//	3)	Write the erase command to the command register (will delay until finished)
			GromWriteData(0xe101, 15, 0x31);
			GromWriteData(0xe102, 15, 0xce);	// erase block (256 bytes)

			//	4)	Check the result code for errors
			unsigned char x = GromReadData(0xe103, 15);		// status register
			if (x != 0) {
				erasehelp();
				writestring(3, 13, "Erase Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				return;
			}

			//	5)	Write the new page data to the write buffer
			// set the GROM address on base 15 (wrappers don't do this for ports)
			*((volatile unsigned char*)(GROMWA_0+0x3c)) = 0xe0;
			*((volatile unsigned char*)(GROMWA_0+0x3c)) = 0x00;
			// now write 256 bytes from the VDP buffer
			for (int i2 = 0; i2 < 256; i2++) {
				unsigned char x = VDPRD;
				*((volatile unsigned char*)(GROMWD_0+0x3c)) = x;
			}

			//	6)	Write the program command to the command register (will delay until finished)
			GromWriteData(0xe101, 15, 0xd2);
			GromWriteData(0xe102, 15, 0x2d);	// program block (256 bytes)

			//	7)	Check the result code for errors
			x = GromReadData(0xe103, 15);		// status register
			if (x != 0) {
				erasehelp();
				writestring(3, 13, "Write Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				return;
			}

			//	8)	Repeat until all pages are written
		}
	}

	waitkeyfree();
	showhelp();
	drawviewer();
	return;
}

void savefile() {
	bool gk = true;

	// zero the buffer
	vdpmemset(VDP_BUFFER, 0xff, 8192+6);

	// What are we saving then...
	unsigned int map = grommap(vieweridx);
	if (map == -1) {
		// no memory mapped
		erasehelp();
		writestring(3,13,"No memory mapped");
		waiterror(5);
		return;
	}

	unsigned int page = map & 0xf;
	map >>= 4;

	// work out length to save
	unsigned int len = 8192;
	if ((map == 0) && (page == 1)) {
		// does it make any sense to save RAM? Maybe, if the system is
		// not powered down....
		len = 7*1024;
	} else if (map == 2) {
		// EEPROM
		len = 4096;
	} else if (map > 2) {
		erasehelp();
		writestring(3,13,"Can't save");
		writestring(4,13,"devices - memory");
		writestring(5,13,"only!");
		waiterror(7);
		return;
	}

	// okay, ask the user for a filename then
	char *name = getFilename("Save Data", &gk);
	if (name == NULL) {
		waitkeyfree();
		showhelp();
		return;
	}

	// okay - copy the data into VDP. This one is easier than the norm, we just read and write
	erasehelp();
	writestring(5,13,"Reading...");

	VDP_SET_ADDRESS_WRITE(VDP_BUFFER);
	if (!gk) {
		// write a fakey GK header
		VDPWD = 0x00;			// no more files
		VDPWD = vieweridx+1;	// 1-based index
		VDPWD = len>>8;			// length
		VDPWD = 0x00;			// I know /I/ never set a non-multiple, so don't include the GK header length
		VDPWD = (vieweridx<<5);	// address
		VDPWD = 0x00;
		len+=6;
	}

	// now set the GROM read address - we can just read directly
	// 0x402 is the offset between a GROM base and GROM Write Address
	unsigned int address = ((vieweridx-3)<<13)+0x6000;
	volatile unsigned char *pDest = (volatile unsigned char *)(grmbase + 0x402);
	*pDest = address>>8;
	*pDest = address&0xff;
	pDest -= 0x402;		// read data address now

	for (int idx=0; idx<len; idx++) {
		volatile unsigned char x = *pDest;
		VDPWD = x;
	}

	erasehelp();
	writestring(5,13,"Saving...");

	pabdat.OpCode = DSR_SAVE;
	pabdat.Status = 0;
	pabdat.VDPBuffer = VDP_BUFFER;	// address of the data buffer in VDP memory
	pabdat.RecordLength = 0;		// size of records. Not used for PROGRAM type. >00 on open means autodetect
	pabdat.CharCount = 0;			// number of bytes read or number of bytes to write (per record)
	pabdat.RecordNumber = len;		// record number for normal files, available bytes (LOAD or SAVE) for PROGRAM type
	pabdat.ScreenOffset = 0;		// Used in BASIC for screen BIAS. Also returns file status on Status call. (DSR_STATUS_xxx)
	pabdat.NameLength = 0;			// for this implementation only, set to zero to read the length from the string
	pabdat.pName = name;			// for this implementation only, must be a valid C String even if length is set

	if (dsrlnk(&pabdat, VDP_PAB)) {
		erasehelp();
		unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
		writestring(3, 13, "DSR Error ");
		faster_hexprint(x);							// relies on VDP address from writestring
		waiterror(5);
		return;
	}

	waitkeyfree();
	showhelp();
	return;
}
#endif

void closefile() {
	pabdat.OpCode = DSR_CLOSE;
	pabdat.RecordLength = 128;	
	pabdat.CharCount = 0;

	if (dsrlnk(&pabdat, VDP_PAB)) {
		erasehelp();
		unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
		writestring(3, 13, "Close Error ");
		faster_hexprint(x);							// relies on VDP address from writestring
		waiterror(5);
	}
}

#ifndef LOADONLY
// Device files are app-specific DF128 files
// They contain the 120 GROM (slot/page by slot/page, ignoring configuration order)
// followed by the EEPROM skipping the first 256 bytes, and finally the last 256 bytes 
// of EEPROM. This flipped order for EEPROM allows loading the configuration space last and
// should in theory work regardless of configuration.
void loaddevice() {
	erasehelp();
	writestring(2,13,"This will overwrite");
	writestring(3,13,"the entire GROM and");
	writestring(4,13,"All configuration!");
	writestring(6,13,"Are you sure? Y/N");
	if ('N' == getyn()) {
		waitkeyfree();
		showhelp();
		return;
	}

	// force a valid configuration with the flash device in the last slot
	// it will all be overwritten anyway
	forcedefaultcfg(1);

	{
		unsigned char x = GromReadData(0xE103, 15);		// get the flash status byte
		if (x == 2) {
			erasehelp();
			writestring(2,13,"GROM memory is");
			writestring(3,13,"write-protected");
			writestring(4,13,"Advanced Config");
			writestring(5,13,"may have changed.");
			waiterror(6);
			return;
		}
	}

	bool gk = false;
	char *name = getFilename("Load Device", &gk);
	if (name == NULL) {
		waitkeyfree();
		showhelp();
		return;
	}
	
	erasehelp();
	writestring(3,13,"Opening...");

	pabdat.OpCode = DSR_OPEN;
	pabdat.Status = DSR_TYPE_INPUT | DSR_TYPE_DISPLAY | DSR_TYPE_FIXED | DSR_TYPE_SEQUENTIAL;
	pabdat.VDPBuffer = VDP_BUFFER;	// address of the data buffer in VDP memory
	pabdat.RecordLength = 128;		// size of records. Not used for PROGRAM type. >00 on open means autodetect
	pabdat.CharCount = 128;			// number of bytes read or number of bytes to write (per record)
	pabdat.RecordNumber = 0;		// record number for normal files, available bytes (LOAD or SAVE) for PROGRAM type
	pabdat.ScreenOffset = 0;		// Used in BASIC for screen BIAS. Also returns file status on Status call. (DSR_STATUS_xxx)
	pabdat.NameLength = 0;			// for this implementation only, set to zero to read the length from the string
	pabdat.pName = name;			// for this implementation only, must be a valid C String even if length is set

	if (dsrlnk(&pabdat, VDP_PAB)) {
		erasehelp();
		unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
		writestring(3, 13, "Open Error ");
		faster_hexprint(x);							// relies on VDP address from writestring
		waiterror(5);
		return;
	}

	erasehelp();
	writestring(3,13,"Reading GROM...");

	for (unsigned char idx = 0; idx < 15; idx++) {	// GROM slots
		vdpscreenchar(VDP_SCREEN_POS(3,28), 'A'+idx);

		GromWriteData(0xE104, 15, idx);		// slot select register 

		for (unsigned char i2=0; i2<32; i2++) {		// GROM flash pages
			GromWriteData(0xE100, 15, i2);	// page select register

			//	3)	Write the erase command to the command register (will delay until finished)
			GromWriteData(0xe101, 15, 0x31);
			GromWriteData(0xe102, 15, 0xce);	// erase block (256 bytes)

			//	4)	Check the result code for errors
			unsigned char x = GromReadData(0xe103, 15);		// status register
			if (x != 0) {
				erasehelp();
				writestring(3, 13, "Erase Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				closefile();
				return;
			}

			//	5)	Write the new page data to the write buffer
			for (int i3=0; i3<2; i3++) {	// two records per page
				pabdat.OpCode = DSR_READ;
				pabdat.VDPBuffer = VDP_BUFFER;
				pabdat.RecordLength = 128;	
				pabdat.CharCount = 0;

				if ((dsrlnk(&pabdat, VDP_PAB)) || (vdpreadchar(VDP_PAB+5) != 128)) {	// fault on DSR error or wrong # bytes read
					erasehelp();
					unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
					writestring(3, 13, "Read Error ");
					faster_hexprint(x);							// relies on VDP address from writestring
					waiterror(5);
					closefile();
					return;
				}

				pabdat.RecordNumber++;		// we have to handle the RecordNumber, the DSR only updated the VDP copy

				// not supposed to trust that the GROM address didn't get changed after a DSR
				// set the GROM address on base 15 (wrappers don't do this for ports)
				*((volatile unsigned char*)(GROMWA_0+0x3c)) = 0xe0;
				*((volatile unsigned char*)(GROMWA_0+0x3c)) = i3 ? 0x80 : 0x00;
				VDP_SET_ADDRESS(VDP_BUFFER);

				for (int i4 = 0; i4 < 128; i4++) {
					unsigned char x = VDPRD;
					*((volatile unsigned char*)(GROMWD_0+0x3c)) = x;
				}
			}

			//	6)	Write the program command to the command register (will delay until finished)
			GromWriteData(0xe101, 15, 0xd2);
			GromWriteData(0xe102, 15, 0x2d);	// program block (256 bytes)

			//	7)	Check the result code for errors
			x = GromReadData(0xe103, 15);		// status register
			if (x != 0) {
				erasehelp();
				writestring(3, 13, "Write Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				closefile();
				return;
			}

			//	8)	Repeat until all pages are written
		}
	}

	// write the EEPROM except the first 256 bytes - we can just mount it in the flash device slot
	erasehelp();
	writestring(3,13,"Reading EEPROM...");

	// EEPROM device mapping to >E000 on slot 15
	unlockeeprom();
	GromWriteData(0xF8F9, 15, 0x20);
	GromWriteData(0xF8F9+8, 15, ~0x20);
	lockeeprom();

	unsigned int adr = 0xE100;	// GROM address

	for (int idx = 0; idx < 32; idx++) {	// 32 records x 128 bytes = 4k
		vdpscreenchar(VDP_SCREEN_POS(3,30), 'A'+(idx>>1));

		pabdat.OpCode = DSR_READ;
		pabdat.VDPBuffer = VDP_BUFFER;
		pabdat.RecordLength = 128;	
		pabdat.CharCount = 0;

		if ((dsrlnk(&pabdat, VDP_PAB)) || (vdpreadchar(VDP_PAB+5) != 128)) {	// fault on DSR error or wrong # bytes read
			erasehelp();
			unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
			writestring(3, 13, "Read Error ");
			faster_hexprint(x);							// relies on VDP address from writestring
			waiterror(5);
			closefile();
			return;
		}

		pabdat.RecordNumber++;		// we have to handle the RecordNumber, the DSR only updated the VDP copy

		// not supposed to trust that the GROM address didn't get changed after a DSR
		// set the GROM address on base 15 (wrappers don't do this for ports)
		unlockeeprom();
		*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr>>8;
		*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr&0xff;
		VDP_SET_ADDRESS(VDP_BUFFER);

		for (int i4 = 0; i4 < 128; i4++) {
			unsigned char x = VDPRD;
			*((volatile unsigned char*)(GROMWD_0+0x3c)) = x;
		}
		lockeeprom();
		
		adr+=128;
		if (adr == 0xf000) {
			// wrap around to the beginning of the EEPROM cfg space)
			// and use the permanently mapped space for the last two entries
			adr = 0xf800;
		}
	}
	// all done - close the file
	closefile();

	waitkeyfree();
	showhelp();
	resetviewer();

	if (forcedefaultcfg(0) == 1) {
		advanced();
		showhelp();
		waitkeyfree();
	}
}

void savedevice() {
	// because GROM is 8k, we have to use the
	// second to last slot. So, we can just remember
	// the original value there.
	erasehelp();
	writestring(1,13,"This will save the");
	writestring(2,13,"entire device and");
	writestring(3,13,"All configuration!");
	writestring(5,13,"It will require");
	writestring(6,13,"~124k (no SSSD!)");
	writestring(8,13,"Are you sure? Y/N");
	if ('N' == getyn()) {
		waitkeyfree();
		showhelp();
		return;
	}

	unsigned char o1 = GromReadData(0xF8F8, 15);
	unsigned char o2 = GromReadData(0xF8F8+8, 15);

	bool gk = false;
	char *name = getFilename("Save Device", &gk);
	if (name == NULL) {
		waitkeyfree();
		showhelp();
		return;
	}
	
	erasehelp();
	writestring(3,13,"Opening...");

	pabdat.OpCode = DSR_OPEN;
	pabdat.Status = DSR_TYPE_OUTPUT | DSR_TYPE_DISPLAY | DSR_TYPE_FIXED | DSR_TYPE_SEQUENTIAL;
	pabdat.VDPBuffer = VDP_BUFFER;	// address of the data buffer in VDP memory
	pabdat.RecordLength = 128;		// size of records. Not used for PROGRAM type. >00 on open means autodetect
	pabdat.CharCount = 128;			// number of bytes read or number of bytes to write (per record)
	pabdat.RecordNumber = 0;		// record number for normal files, available bytes (LOAD or SAVE) for PROGRAM type
	pabdat.ScreenOffset = 0;		// Used in BASIC for screen BIAS. Also returns file status on Status call. (DSR_STATUS_xxx)
	pabdat.NameLength = 0;			// for this implementation only, set to zero to read the length from the string
	pabdat.pName = name;			// for this implementation only, must be a valid C String even if length is set

	if (dsrlnk(&pabdat, VDP_PAB)) {
		erasehelp();
		unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
		writestring(3, 13, "Open Error ");
		faster_hexprint(x);							// relies on VDP address from writestring
		waiterror(5);
		return;
	}

	erasehelp();
	writestring(3,13,"Writing GROM...");

	for (unsigned char idx = 0x10; idx < 0x1f; idx++) {	// GROM slots
		vdpscreenchar(VDP_SCREEN_POS(3,28), 'A'+(idx&0x0f));

		unlockeeprom();
		GromWriteData(0xF8F8, 15, idx);
		GromWriteData(0xF8F8+8, 15, ~idx);
		lockeeprom();

		// now it's mapped, we just need to read and write out 8k
		// that's 64 records
		unsigned int adr = 0xC000;
		for (int i2 = 0; i2<64; i2++) {
			// copy the data from GROM to VDP
			// set the GROM address on base 15 (wrappers don't do this for ports)
			*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr>>8;
			*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr&0xff;
			VDP_SET_ADDRESS_WRITE(VDP_BUFFER);

			for (int i4 = 0; i4 < 128; i4++) {
				unsigned char x = *((volatile unsigned char*)(GROMRD_0+0x3c));
				VDPWD = x;
			}

			adr+=128;

			// now write it out to the file
			pabdat.OpCode = DSR_WRITE;
			pabdat.VDPBuffer = VDP_BUFFER;
			pabdat.RecordLength = 128;	
			pabdat.CharCount = 128;

			if (dsrlnk(&pabdat, VDP_PAB)) {
				erasehelp();
				unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
				writestring(3, 13, "Write Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				closefile();
				return;
			}

			pabdat.RecordNumber++;		// we have to handle the RecordNumber, the DSR only updated the VDP copy
		}
	}

	// write the EEPROM except the first 256 bytes - we can just mount it in the flash device slot too
	erasehelp();
	writestring(3,13,"Writing EEPROM...");

	// EEPROM device mapping to >C000 on slot 15
	unlockeeprom();
	GromWriteData(0xF8F8, 15, 0x20);
	GromWriteData(0xF8F8+8, 15, ~0x20);
	lockeeprom();

	unsigned int adr = 0xC100;	// GROM address

	for (int idx = 0; idx < 32; idx++) {	// 32 records x 128 bytes = 4k
		vdpscreenchar(VDP_SCREEN_POS(3,30), 'A'+(idx>>1));

		// not supposed to trust that the GROM address didn't get changed after a DSR
		// set the GROM address on base 15 (wrappers don't do this for ports)
		*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr>>8;
		*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr&0xff;
		VDP_SET_ADDRESS_WRITE(VDP_BUFFER);

		for (int i4 = 0; i4 < 128; i4++) {
			unsigned char x = *((volatile unsigned char*)(GROMRD_0+0x3c));
			// check for the overridden EEPROM data
			if ((adr == 0xc100) && (i4 == 0)) x = o2;	// yeah, it's split like that
			if ((adr == 0xf880) && (i4 == 0x78)) x = o1;
			VDPWD = x;
		}
		
		adr+=128;
		if (adr == 0xd000) {
			// wrap around to the beginning of the EEPROM cfg space)
			// and use the permanently mapped space for the last two entries
			adr = 0xf800;
		}

		pabdat.OpCode = DSR_WRITE;
		pabdat.VDPBuffer = VDP_BUFFER;
		pabdat.RecordLength = 128;	
		pabdat.CharCount = 128;

		if (dsrlnk(&pabdat, VDP_PAB)) {
			erasehelp();
			unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
			writestring(3, 13, "Write Error ");
			faster_hexprint(x);							// relies on VDP address from writestring
			waiterror(5);
			closefile();
			return;
		}

		pabdat.RecordNumber++;		// we have to handle the RecordNumber, the DSR only updated the VDP copy
	}

	// all done - close the file
	erasehelp();
	writestring(3,13,"Closing...");

	closefile();
	GromWriteData(0xF8F8, 15, o1);
	GromWriteData(0xF8F8+8, 15, o2);

	waitkeyfree();
	showhelp();
}
#endif

#ifdef LOADONLY
// Device files are app-specific DF128 files
// They contain the 120 GROM (slot/page by slot/page, ignoring configuration order)
// followed by the EEPROM skipping the first 256 bytes, and finally the last 256 bytes 
// of EEPROM. This flipped order for EEPROM allows loading the configuration space last and
// should in theory work regardless of configuration.
// same as LOADDEVICE, but used in the case of a loader-only build
void dedicatedloaddevice() {
	// full screen box
	drawbox(0,2,23,29);
	clrregion(1,3,22,28);
	
	writestring(10,4,"This will overwrite the");
	writestring(11,4,"entire GROM and all");
	writestring(12,4,"configuration!");
	writestring(13,4,"Are you sure? Y/N");
	if ('N' == getyn()) {
		return;
	}
	clrregion(1,3,22,28);

	// force a valid configuration with the flash device in the last slot
	// it will all be overwritten anyway
	forcedefaultcfg(1);

	{
		unsigned char x = GromReadData(0xE103, 15);		// get the flash status byte
		if (x == 2) {
			clrregion(1,3,22,28);
			writestring(10,5,"GROM memory is"); 
			writestring(11,5,"write-protected.");
			writestring(12,5,"Advanced Config");
			writestring(13,5,"may have changed.");
			waiterror(15);
			return;
		}
	}

	bool gk = false;
	char *name = getFilename("Load Device", &gk);
	if (name == NULL) {
		waitkeyfree();
		showhelp();
		return;
	}
	
	clrregion(1,3,22,28);

	writestring(16,5,"Opening...");

	pabdat.OpCode = DSR_OPEN;
	pabdat.Status = DSR_TYPE_INPUT | DSR_TYPE_DISPLAY | DSR_TYPE_FIXED | DSR_TYPE_SEQUENTIAL;
	pabdat.VDPBuffer = VDP_BUFFER;	// address of the data buffer in VDP memory
	pabdat.RecordLength = 128;		// size of records. Not used for PROGRAM type. >00 on open means autodetect
	pabdat.CharCount = 128;			// number of bytes read or number of bytes to write (per record)
	pabdat.RecordNumber = 0;		// record number for normal files, available bytes (LOAD or SAVE) for PROGRAM type
	pabdat.ScreenOffset = 0;		// Used in BASIC for screen BIAS. Also returns file status on Status call. (DSR_STATUS_xxx)
	pabdat.NameLength = 0;			// for this implementation only, set to zero to read the length from the string
	pabdat.pName = name;			// for this implementation only, must be a valid C String even if length is set

	if (dsrlnk(&pabdat, VDP_PAB)) {
		clrregion(1,3,22,28);
		unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
		writestring(3, 13, "Open Error ");
		faster_hexprint(x);							// relies on VDP address from writestring
		waiterror(5);
		return;
	}

	clrregion(1,3,22,28);
	writestring(14,5,"Reading GROM...");

	for (unsigned char idx = 0; idx < 15; idx++) {	// GROM slots
		vdpscreenchar(VDP_SCREEN_POS(14,20), 'A'+idx);

		GromWriteData(0xE104, 15, idx);		// slot select register 

		for (unsigned char i2=0; i2<32; i2++) {		// GROM flash pages
			GromWriteData(0xE100, 15, i2);	// page select register

			//	3)	Write the erase command to the command register (will delay until finished)
			GromWriteData(0xe101, 15, 0x31);
			GromWriteData(0xe102, 15, 0xce);	// erase block (256 bytes)

			//	4)	Check the result code for errors
			unsigned char x = GromReadData(0xe103, 15);		// status register
			if (x != 0) {
				clrregion(1,3,22,28);
				writestring(17, 5, "Erase Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				closefile();
				return;
			}

			//	5)	Write the new page data to the write buffer
			for (int i3=0; i3<2; i3++) {	// two records per page
				pabdat.OpCode = DSR_READ;
				pabdat.VDPBuffer = VDP_BUFFER;
				pabdat.RecordLength = 128;	
				pabdat.CharCount = 0;

				if ((dsrlnk(&pabdat, VDP_PAB)) || (vdpreadchar(VDP_PAB+5) != 128)) {	// fault on DSR error or wrong # bytes read
					clrregion(1,3,22,28);
					unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
					writestring(17, 5, "Read Error ");
					faster_hexprint(x);							// relies on VDP address from writestring
					waiterror(5);
					closefile();
					return;
				}

				pabdat.RecordNumber++;		// we have to handle the RecordNumber, the DSR only updated the VDP copy

				// not supposed to trust that the GROM address didn't get changed after a DSR
				// set the GROM address on base 15 (wrappers don't do this for ports)
				*((volatile unsigned char*)(GROMWA_0+0x3c)) = 0xe0;
				*((volatile unsigned char*)(GROMWA_0+0x3c)) = i3 ? 0x80 : 0x00;
				VDP_SET_ADDRESS(VDP_BUFFER);

				for (int i4 = 0; i4 < 128; i4++) {
					unsigned char x = VDPRD;
					*((volatile unsigned char*)(GROMWD_0+0x3c)) = x;
				}
			}

			//	6)	Write the program command to the command register (will delay until finished)
			GromWriteData(0xe101, 15, 0xd2);
			GromWriteData(0xe102, 15, 0x2d);	// program block (256 bytes)

			//	7)	Check the result code for errors
			x = GromReadData(0xe103, 15);		// status register
			if (x != 0) {
				clrregion(1,3,22,28);
				writestring(17, 5, "Write Error ");
				faster_hexprint(x);							// relies on VDP address from writestring
				waiterror(5);
				closefile();
				return;
			}

			//	8)	Repeat until all pages are written
		}
	}

	// write the EEPROM except the first 256 bytes - we can just mount it in the flash device slot
	writestring(15,5,"Reading EEPROM...");

	// EEPROM device mapping to >E000 on slot 15
	unlockeeprom();
	GromWriteData(0xF8F9, 15, 0x20);
	GromWriteData(0xF8F9+8, 15, ~0x20);
	lockeeprom();

	unsigned int adr = 0xE100;	// GROM address

	for (int idx = 0; idx < 32; idx++) {	// 32 records x 128 bytes = 4k
		vdpscreenchar(VDP_SCREEN_POS(15,22), 'A'+(idx>>1));

		pabdat.OpCode = DSR_READ;
		pabdat.VDPBuffer = VDP_BUFFER;
		pabdat.RecordLength = 128;	
		pabdat.CharCount = 0;

		if ((dsrlnk(&pabdat, VDP_PAB)) || (vdpreadchar(VDP_PAB+5) != 128)) {	// fault on DSR error or wrong # bytes read
			clrregion(1,3,22,28);
			unsigned char x = vdpreadchar(VDP_PAB+1);	// changes VDP address
			writestring(17, 5, "Read Error ");
			faster_hexprint(x);							// relies on VDP address from writestring
			waiterror(5);
			closefile();
			return;
		}

		pabdat.RecordNumber++;		// we have to handle the RecordNumber, the DSR only updated the VDP copy

		// not supposed to trust that the GROM address didn't get changed after a DSR
		// set the GROM address on base 15 (wrappers don't do this for ports)
		unlockeeprom();
		*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr>>8;
		*((volatile unsigned char*)(GROMWA_0+0x3c)) = adr&0xff;
		VDP_SET_ADDRESS(VDP_BUFFER);

		for (int i4 = 0; i4 < 128; i4++) {
			unsigned char x = VDPRD;
			*((volatile unsigned char*)(GROMWD_0+0x3c)) = x;
		}
		lockeeprom();
		
		adr+=128;
		if (adr == 0xf000) {
			// wrap around to the beginning of the EEPROM cfg space)
			// and use the permanently mapped space for the last two entries
			adr = 0xf800;
		}
	}
	// all done - close the file
	closefile();

	writestring(17, 5, "Successful!");

	waiterror(19);
}
#endif

int main() {
	// init the screen
	{
		int x = set_graphics(VDP_SPR_8x8);		// set graphics mode with 8x8 sprites
		charsetlc();							// get character set including lowercase
		vdpmemset(gColor, 0x10, 32);			// all colors to black on transparent
		vdpchar(gSprite, 0xd0);					// all sprites disabled
		VDP_SET_REGISTER(VDP_REG_MODE1, x);		// enable the display
		VDP_REG1_KSCAN_MIRROR = x;				// must have a copy of VDP Reg 1 if you ever use KSCAN
	}
	vdpmemcpy(gPattern+(20*8), extrachars, sizeof(extrachars));
	VDP_SET_REGISTER(VDP_REG_COL, 0x07);

	vieweroff=0;
	vieweridx=0;
	grmbase=0x9800;

#ifdef LOADONLY
	dedicatedloaddevice();
#else

	drawfixed();
	showhelp();
	resetviewer();

	// display a build date in lieu of a version
	writestring(16,5,"Build date: 6/2/2015");

	if (forcedefaultcfg(0) == 1) {
		advanced();
		showhelp();
		waitkeyfree();
	}
	
	while (1) {
		unsigned char x;

		// briefly enable interrupts to allow QUIT
		VDP_INT_ENABLE;
		VDP_INT_DISABLE;

		x = kscan(5);

		switch (x) {
		case 8:	
			// left arrow
			if (grmbase > 0x9800) {
				grmbase -= 4;
				resetviewer();
			}
			waitkeyfree();
			break;

		case 9:
			// right arrow
			if (grmbase < 0x983C) {
				grmbase += 4;
				resetviewer();
			}
			waitkeyfree();
			break;

		case 11:
			// up arrow
			if (vieweroff > 0) {
				vieweroff -= 0x20;
				drawviewer();
			}
			break;

		case 10:
			// down arrow
			if (vieweroff < 8192-9*13) {
				vieweroff += 0x20;
				drawviewer();
			}
			break;

		case 'V':
			erasehelp();
			eraseviewer();
			hchar(10,21,32,9);
			hchar(10,21,30,1);
			hchar(5,11,23,1);
			hchar(5,12,28,8);
			hchar(5,20,24,1);
			hchar(6,20,26,1);
			hchar(7,20,22,1);
			writestring(8,13,"Select view (6-E)");
			waitkeyfree();
			x=GetDevice(0);
			if (x <= 4) {
				// valid value
				vieweridx=x+3;
			}
			drawviewer();
			hchar(5,11,26,1);
			showhelp();
			waitkeyfree();
			break;

		case '6':
		case '8':
		case 'A':
		case 'C':
		case 'E':
			{
				// parse the key using GetDevice (it should be valid!)
				x=GetDevice(x)+3;
				erasehelp();
				hchar(x,4,32,7);
				hchar(x,4,30,1);
				hchar(x,5,29,1);
				hchar(x,6,28,5);
				hchar(x,11,27,1);
				vchar(2,11,26,x-3);
				hchar(1,11,25,1);
				writestring(1,12,"\x1cSelect:");
				for (int idx=0; idx<5; idx++) {
					writestring(2+idx,15,(char*)selecttext[idx]);
				}
				unsigned char y = GetType();
				if (y < 8) {
					// valid type
					unsigned char z = 0;

					writestring(x,4,(char*)devices[y]);

					if ((y==0)||(y==1)||(y==4)) {
						VDPWD=':';
						VDPWD=30;
						VDPWD=29;
						hchar(1,11,26,1);
						erasehelp();
						writestring(2,11,"\x19\x1cSelect 0-");

						switch (y) {
						case 0:	// RAM
							VDPWD='1';
							z=GetPage(2);
							break;
						case 1:	// GROM
							VDPWD='E';
							z=GetPage(15);
							break;
						case 4: // ADC
							VDPWD='3';
							z=GetPage(4);
							break;
						}
					}

					if (z < 15) {	// highest legal value - 'back' sets 0xff so this is fine
						unlockeeprom();
						y=(y<<4)|z;
						z=~y;
						
						unsigned int adr = ((grmbase-0x9800)<<2)+0xf802+x;		// base*16 + 2 == eeprom address of config, + 0xf800 for the config offset
						GromWriteData(adr, 15, y);
						GromWriteData(adr+8, 15, z);

						lockeeprom();
					} else {
						x=0xff;
					}
				} else if (y == 0x80) {
					// user selected 'none'
					unlockeeprom();
						unsigned int adr = ((grmbase-0x9800)<<2)+0xf802+x;		// base*16 + 2 == eeprom address of config, + 0xf800 for the config offset
						GromWriteData(adr, 15, 0xff);
						GromWriteData(adr+8, 15, 0xff);
					lockeeprom();
				} else {
					x=0xff;
				}

				clrregion(3,4,7,10);
				vchar(1,11,26,7);
				if (x != 0xff) {
					vieweroff = 0;
					vieweridx = x;
				}
				drawgroms();
				showhelp();
				drawviewer();
			}
			waitkeyfree();
			break;

		// Loading and saving individual banks - we will support GRAM Kracker
		// format for this, just to make life easier for other people. We
		// Won't use the header, however. Since files from the PC will need
		// to be converted into PROGRAM type files anyway, this is a minor
		// issue (I can make a little tool to slap on the TI FILES and GK header)
		// So, these are expected to be 8198 byte PROGRAM image files (8k+6 bytes)
		// at maximum.
		case 'L':
			loadfile();
			break;

		case 'S':
			savefile();
			break;

		// loading and saving the entire device - for these we will use the
		// no-header-required DF128 format. We will store the 4k of
		// EEPROM (including configuration bytes) and 120k of flash.
		case 140:
			// control-L
			loaddevice();
			break;

		case 147:
			// control-S
			savedevice();
			break;

		case 1:
			// FCTN-7 (Aid) - secret advanced menu
			advanced();
			showhelp();
			waitkeyfree();
			break;

		}
	}
#endif

	return 0;
}
