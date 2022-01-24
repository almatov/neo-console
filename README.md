# neo-console - NanoPi NEO/NEO2 console daemon

This is a C++ written program that uses 3 buttons and the display of the
NanoHat OLED board as a console. F1 shows the IP list, this is the default
mode. F2 shows the clock. F3 clears the screen.

The package has NanoHat class and you can easily customize the console for
your own purposes. The display shows a 16x4 text and uses Spleen font. The
package has a bdfparser font convertor to the SSD1306 OLED display format,
and you can use any other monospace 8x16 BDF font.

The utility was compiled and tested under the Armbian Linux. But it should
work under any other Linux system.

## How to install

Software requirements:

	g++-6.3.0+	(apt install build-essential)

Run this

	make
	sudo make install
	sudo systemctl start neo-console
