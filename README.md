# emurom-arm

Atari 2600 cartridge emulator targeting Texas Instrument's Stellaris Launchpad kit: http://www.ti.com/tool/ek-lm4f120xl

Requires the StellarisWare® Driver Library Standalone Package: http://www.ti.com/tool/sw-drl

Requires the arm-none-eabi-gcc toolchain: https://launchpad.net/gcc-arm-embedded/+download

## Building

Unpack the StellarisWare® package into ../stellarisware

Install gcc-arm-embedded.

Run "make"

## Running

This is a cartridge emulator, so the plan is to plug it into an Atari 2600, read the address lines and return the correct program byte quickly enough that the system doesn't crash.  See the implementation_notes file for the mapping between the pins on the Stellaris board and the pins on the edge of the Atari cartridge's board.

I had luck with buying a cheap collection of poor condition Atari games off of eBay and desoldering the ROM chips to get a board that had the right thickness and pitch between pins.  I suspect there are other sources as well.

## Bugs & assorted glitches

The board takes forever to start up, or at least longer than the handful of milliseconds the Atari's 1.2-ish MHz processor is willing to wait.  It would be nice to figure out how to power the board directly from the Atari, but that was impractical given the fact that the Atari is expecting a ROM chip to be there from the get-go and has no concept of essentially waiting for the user to insert a cartridge.

It seems like there should be a way to just feed it a reset command when the board is ready since 6507 assembly for the Atari is well understood (eg: http://atarihq.com/danb/files/stella.pdf) but a handful of experiments failed to yield reasonable results on real hardware.  My best guess is that feeding the chip essentially random inputs runs a pretty good risk of feeding it illegal opcodes and sending it spinning off into oblivion (see: http://www.pagetable.com/?p=39).

The current workaround involves powering up the board first, waiting for the light to turn green, and then turning on the Atari.  This always seemed like the prelude to a fascinating short circuit, so be careful with how you're powering the board.
