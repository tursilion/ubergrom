http://harmlesslion.com

"UberGROM" is a GROM simulator for the TI-99/4A that uses a large AVR to implement ROM, RAM, and peripherals, all configurable on the GROM space.

Source for the simulator and a configuration tool are both included.

To use: Program the AVR with either the .hex or .elf file, by your choice. I recommend leaving the EEPROM erased, on first powerup it will be automatically configured.

Then just run the configuration tool on the TI side to configure and load software. Please read the documentation.

It's also possible to configure and load the image before booting. The AVR firmware goes in the last 8k of the image, and every 8k before that is a bank of available GROM. The configuration in the EEPROM determines what address each bank appears at on the TI. Again, review the documentation.

Alternately, check the "formatter" folder for a small app (Windows, but should compile on Linux) that can take a file created with the configuration tool under Classic99 or another TI Emulator using "Save Entire Device" and split it into separate Flash and EEPROM files, allowing you to program the entire device in one shot outside of the TI.

The AVR should be configured for a bootloader in the last 8k, 5v operation with internal 8MHz clock. With these settings it will outperform the console GROMs (although there's no noticable impact from doing so, due to the speed of the GPL interpreter.)

Please review the license before modifying code. It's not GPL. :)

4/30/2015 - just a small cosmetic fix to the viewer - was a glitched byte on startup.
6/2/2015  - updated AVR code so that the hardware write protect pin will protect the EEPROM configuration space
