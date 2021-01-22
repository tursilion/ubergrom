These files are provided as TIFILES and on a 180k disk image for convenience.

GROMCFG (and GROMCFH) - EA#5 program for the main configuration, loading, and saving of the UberGROM
GROMLOAD - EA#5 program to load a UberGROM backup image
BASICLOAD - TI BASIC Playground Loader to load GROMLOAD

The Playground loader was created by Tony Knerr with help from Senior Falcon. Thanks! The files must be on DSK1 for the Playground loader to work.

If you need to run GROMCFG from a blank AVR, I recommend using the powerup recovery method for a full E/A#5 loader. (Hold space bar on power up). This option may be disabled on some third-party carts but is always there on a newly programmed AVR. :)

In detail:

-Turn on the TI
-Press a key on the title page
-Press '1' for TI BASIC
-Type: OLD DSK1.BASICLOAD
-Type: RUN
-After it loads, it will ask you if you are sure you want to overwrite the entire GROM and all configuration. On N, it will reset. On Y, you will be prompted for the filename to load.
