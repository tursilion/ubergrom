Intended to compile with the TMS9900 port of GCC, patch 1.8 or higher.
Also requires libti99!

This is now fully tested.

The config stuff was always there, but the idea is that you select the GROM base 
you want to configure with left and right arrows.

Then you select the GROM address you want to configure by pressing the first 
character (6000, 8000, A000, C000, E000). 

Then you select the device to place at that GROM slot – there’s an onscreen menu.

This all takes effect immediately on the chip, so you can quit out at any time.

To load data into a GROM slot (or the EEPROM – but be aware that loading EEPROM will 
corrupt your configuration – load EPROM data first THEN configure the layout), get 
the desired page visible in the viewer by moving to a mapped base (left and right), 
then pressing ‘V’ and selecting the desired address. (If you configure an address, 
that will also update the viewer).

Once in the viewer, ‘L’ will load. The file format is a PROGRAM image file, and it 
may optionally have a 6 byte GRAM Kracker header which will be ignored. It will ask 
whether to skip the header or not – you have to know (but you can look at the hex 
bytes and make sure after it loads).

‘S’ will save the current viewed slot to a file, assuming it is GROM, EEPROM or RAM. 
(You can’t load RAM, but it seemed saving it might be useful for debugging things). 
If you do not skip the GK Header, it will write something that might work based on 
where you have it configured to appear, no promises though. It will always write 8k 
files except for the second RAM bank (7k) and EEPROM (4k).

Arguably the most useful, Control-L and Control-S will load and save the entire device. 
This requires a disk with at least 124k of storage (plus one sector), so SSSD disks will not work.

The file format for this is DF128 records. The internal memory of the AVR is saved 
without regard for the configuration – first comes the 120k of flash data (GROM), followed 
by the 4k of EEPROM. The EEPROM is offset slightly – the first address is address 256, 
and then it wraps around and stores the first 256 bytes at the end of the file. This just 
makes it easier to leave the configuration data till the last.

It’s built with the current version of the GCC patches, and requires my libti99 from 
Github to build.
