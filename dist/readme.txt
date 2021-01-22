http://harmlesslion.com

"UberGROM" is a GROM simulator for the TI-99/4A that uses a large AVR to implement ROM, RAM, and peripherals, all configurable on the GROM space.

Source for the simulator and a configuration tool are both included.

To use: Program the AVR with either the .hex or .elf file, by your choice. I recommend leaving the EEPROM erased, on first powerup it will be automatically configured.

Then just run the configuration tool on the TI side to configure and load software. Please read the documentation.

It's also possible to configure and load the image before booting. The AVR firmware goes in the last 8k of the image, and every 8k before that is a bank of available GROM. The configuration in the EEPROM determines what address each bank appears at on the TI. Again, review the documentation.

Alternately, check the "formatter" folder for a small app (Windows, but should compile on Linux) that can take a file created with the configuration tool under Classic99 or another TI Emulator using "Save Entire Device" and split it into separate Flash and EEPROM files, allowing you to program the entire device in one shot outside of the TI.

The AVR should be configured for a bootloader in the last 8k, 5v operation with internal 8MHz clock. With these settings it will outperform the console GROMs (although there's no noticable impact from doing so, due to the speed of the GPL interpreter.)

The fuses are the configuration settings for the microcontroller. On my VS4800's "VSPEED" software it's under "Set->Config or Encrypt". Every AVR has a slightly different set, but the 1284P has the following settings which should be SOMEWHERE on the relevant dialog. The settings are just bytes, but most programmers split them up into checkboxes for ease of writing.

First is the lock bit byte, containing:

BLB01, BLB02, BLB11, BLB12 - These set the lock mode for the boot block
LB1, LB2 - These set the lock mode for the rest of the memory

These are used to secure the chip against accidental writes and against reading back the data. Since we need to be able to write to the chip and we aren't trying to hide the code, they should all be unset (and on the AVR, unset is a '1'). So nothing selected, and the final byte value should be 0xFF. (Some tools like the VSPEED I use writes hex with an 'H', so 'FFH' instead). 

Don't play with these settings to see how they work unless you are prepared to erase the full chip and start from blank again - they can not be unchecked without a chip erase. (They wouldn't be very good security otherwise! :) )

The "Extended Fuse Byte" has just three bits defined:

BODLEVEL0, BODLEVEL1, BODLEVEL2 - this sets the level of a Brown-out detector. It's purpose is to hold the chip in reset when the voltage is too low to operate. Setting 0 and 1 puts the brown out voltage at 4.3V, which I recommended just in case the TI power supply comes up unevenly. The AVR can run just fine down to 3v, so that level ensures power is stable when it starts. This results in a write value of 0xFC.

The "Fuse High Byte" might also be presented as the high byte of a 16-bit value. I'll split it into High and Low because that's what the datasheet does. The high byte sets 8 bits:

OCDEN - enables an on-chip debug mode that runs the clocks even in sleep mode. Should be DISABLED.
JTAGEN - enables the JTAG debug interface. MUST be DISABLED (we use the pins to talk to the TI).
SPIEN - enable serial programming mode. I often use this so I leave it ENABLED.
WDTON - Watchdog timer always on. The software doesn't use watchdog so this MUST be DISABLED.
EESAVE - preserves EEPROM during chip erase. For UberGROM leave it DISABLED.
BOOTSZ1,BOOTSZ0 - sets the size of the boot memory (firmware in this case). Both MUST be ENABLED which results in an 8k boot area.
BOOTRST - moves the reset vector to the start of the boot memory. MUST be ENABLED.

This results in a high byte value of 0xD8.


The "Fuse Low Byte" has these 8 bits:

CKDIV8 - divides the system clock by 8. Should be DISABLED unless you like a really slow system. ;)
CKOUT - outputs the clock on one of the pins. MUST be DISABLED (we use that pin to talk to the TI)
SUT1,SUT0 - start up time delay. Both should be ENABLED which is fastest start because the brown out detector will watch the power for us in hardware.
CLKSEL3,CKSEL2,CKSEL1,CKSEL0 - clock selection. All but CKSEL1 MUST be ENABLED, CKSEL1 MUST be DISABLED. This selects the internal 8MHz clock.

This results in a low byte value of 0xC2.

The CLKSEL bits give some people problems. If they are set to (almost) ANYTHING ELSE, then the chip has been told to use an external clock. With no clock, nothing happens, so it appears to be dead. Nothing is broken or bricked, but to recover you must attach an external clock! I've used a single "half-size oscillator" to feed clock in. It's three wires - power, ground, and clock out, and it goes to the XTAL1 pin on the chip. (Check your datasheet). It's more than enough to get in and fix the fuse setting anyway. ;)

Please review the license before modifying code. It's not GPL. :)

4/30/2015 - just a small cosmetic fix to the viewer - was a glitched byte on startup.
6/2/2015  - updated AVR code so that the hardware write protect pin will protect the EEPROM configuration space
