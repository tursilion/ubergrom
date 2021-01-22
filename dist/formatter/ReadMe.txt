Extract from an UberGROM dump file the flash and EEPROM components.

"formatter <input file> <flash output> <eeprom output>"

The input file is normally generated using a TI Emulator. It is expected to
be in TIFILES format - Classic99 will write the file natively, other
emulators may require conversion using a utility such as TI99Dir.

The two output files will be in binary. This may be enough for some
tools. If your programmer requires a hex file instead, you can generate
one using the tool "objcopy" (GNU available for all operating systems).

objcopy -I binary -O ihex flash.bin flash.ihex
objcopy -I binary -O ihex eeprom.bin eeprom.ihex

As this program is very simple (checks the header, then strips out 120k
of flash, and re-assembles the split EEPROM data), it's shipped as a
single C file for easy porting.

License-free - enjoy at will!

NOTE: Classic99 configuration. 

To be useful, you will need to be able to simulate an UberGROM in emulation.
Classic99 has had limited support for a few years now, enough to run this,
anyway. Add the following configuration to your Classic99.ini. When you
select this cartridge, you will get my EA Complete hack in place of TI BASIC.
The UberGROM support is not sufficient to load any cartridges yet, although
if load them with GROMCFG they should work until you reset the emulator
(so exit with QUIT, not file->reset).

(Change the group index if you're already using some of the groups, of course).
You can download EA Complete from http://www.harmlesslion.com/software/complete

---------------------- 8< ------------------------

[CartGroups]
Group0=Test

[test1]
name=Ubergrom test
; The UberGROM code load doesn't work today, but the U is needed
; to activate the emulation. Included is an E/A GROM relocated to
; >2000 so it can replace TI BASIC. A future version of Classic99
; will fix the UberGROM powerup emulation so that you can properly
; get into it. Copy the EA2K.BIN into the Classic99 mods folder.
; Do not load editor or assembler from this, they will not function.
; This is actually a part of my MPD project. ;) Just use it to load
; up GROMCFG.
rom0=U|6000|2000|C:\classic99\MODS\EA2k.BIN
rom1=G|2000|2000|C:\classic99\MODS\EA2k.BIN
