// formatter.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <memory.h>
#include <string.h>

FILE *fIn, *fOut;
unsigned char buf[256];
int filepos;

int main(int argc, char *argv[])
{
	int cnt;

	// enough arguments?
	if (argc < 4) {
		printf("formatter <input file> <flash output> <eeprom output>\n");
		return 1;
	}

	// open input file
	fIn = fopen(argv[1], "rb");
	if (NULL == fIn) {
		printf("Failed to open input file %s\n", argv[1]);
		return 1;
	}

	// check header
	if (128 != fread(buf, 1, 128, fIn)) {
		printf("Incomplete header on input file.\n");
		return 1;
	}

	if (0 != memcmp(buf, "\x7TIFILES", 8)) {
		printf("Input file must have TIFILES header.\n");
		return 1;
	}

	//h[0] = 7;
	//h[1] = 'T';
	//h[2] = 'I';
	//h[3] = 'F';
	//h[4] = 'I';
	//h[5] = 'L';
	//h[6] = 'E';
	//h[7] = 'S';
	//h[8] = tmpInfo.LengthSectors>>8;			// length in sectors HB
	//h[9] = tmpInfo.LengthSectors&0xff;		// LB 
	//h[10] = tmpInfo.FileType;					// File type 
	//h[11] = tmpInfo.RecordsPerSector;			// records/sector
	//h[12] = tmpInfo.BytesInLastSector;		// # of bytes in last sector
	//h[13] = tmpInfo.RecordLength;				// record length 
	//h[14] = tmpInfo.NumberRecords&0xff;		// # of records(FIX)/sectors(VAR) LB 
	//h[15] = tmpInfo.NumberRecords>>8;			// HB
	//#define TIFILES_VARIABLE		0x80		// else Fixed
	//#define TIFILES_PROTECTED		0x08		// else not protected
	//#define TIFILES_INTERNAL		0x02		// else Display
	//#define TIFILES_PROGRAM		0x01		// else Data

	if ((buf[13] != 128)||((buf[10]&0x83) != 0)) {
		printf("Input file must be DF128.\n");
		return 1;
	}

	// open flash output file
	fOut = fopen(argv[2], "wb");
	if (NULL == fOut) {
		printf("Can't open flash output file %s\n", argv[2]);
		return 1;
	}

	printf("Writing 120k of flash to %s\n", argv[2]);
	cnt=120*1024;

	while (cnt > 0) {
		if (256 != fread(buf, 1, 256, fIn)) {
			printf("Premature end of file with %d bytes left!\n", cnt);
			return 1;
		}
		if (256 != fwrite(buf, 1, 256, fOut)) {
			printf("Error writing output file!\n");
			return 1;
		}
		cnt-=256;
	}

	fclose(fOut);

	// open eeprom output file
	fOut = fopen(argv[3], "wb");
	if (NULL == fOut) {
		printf("Can't open EEPROM output file %s\n", argv[3]);
		return 1;
	}

	printf("Writing 4k of EEPROM to %s\n", argv[3]);
	cnt=4*1024;

	// we have to seek a little - the first 256 bytes are
	// actually at the end of the file
	filepos = ftell(fIn);
	fseek(fIn, -256, SEEK_END);

	while (cnt > 0) {
		if (256 != fread(buf, 1, 256, fIn)) {
			printf("Premature end of file with %d bytes left!\n", cnt);
			return 1;
		}
		if (256 != fwrite(buf, 1, 256, fOut)) {
			printf("Error writing output file!\n");
			return 1;
		}
		if (cnt == 4*1024) {
			fseek(fIn, filepos, SEEK_SET);
		}
		cnt-=256;
	}

	fclose(fOut);

	printf("** DONE **\n");

	return 0;
}

