20181204

AVR-based GROM Simulator for the TI-99/4A
-----------------------------------------
by Mike Brent aka Tursi

This program demonstrates a GROM simulation for the TI-99/4A. The simulation
is capable of multiple GROM bases and can expose as much memory as you choose
to make available. Although this demonstration code is written for the 
Atmega1284, it will run on any AVR that has at least 11 I/O pins (8 data and 3
control), although that configuration allows only one base (up to 30k).

This is finally the fully realized version I described years ago, with
various peripherals active. Please see the documentation for how to work
with it.

Please remember, if you choose to modify the code, that you need to ask
me before you can release your modified code. If you do not modify the
code but simply create a cartridge that uses it, you may release your
cartridge without checking with me, but I would appreciate a mention in
the software or the documentation, either as Tursi or using UberGROM.

Thanks!

AVR Memory Usage
----------------
Device: atmega1284p

Program:    8144 bytes (6.2% Full)
(.text + .data + .bootloader)

Data:      15960 bytes (97.4% Full)
(.data + .bss + .noinit)

