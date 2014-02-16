The NeoPixel Server
===================

This is the source code for an Arduino project for a very small and bare webserver, controlling
a strip or ring of WS2812 LEDs with an integrated driver.

The hardware consists of a mircocontroller (tested with Arduino Leonardo and Sparkfun Fio v3),
a strip or ring of WS2812 LEDs (Adafruit NeoPixel, if you want to call them that) and an Adafruit
CC3000 breakout or Arduino Shield. It also kind of supports spectrum analysis using an electrit
microphone, connected to an analog input, but that particular feature is only working on the
Sparkfun Fio v3 on A4 at the moment, as that was done in a trial and error fasion during an 8h
drive.

To build this project, the following libraries are required:
* Adafruit CC3000 Library:
* Adafruit NeoPixel Library:
* Fast Hartley Transform library:

Using the default libraries may result in a binary, that does not fit onto your Arduino. You will
likely end up with a project, that is 31k or more in binary size. If you want to fit the project
onto a microcontroller with only 32k flash, use these libraries:
* Custom Adafruit CC3000 Library:
** "utility/cc3000_common.h": define CC3000_TINY_DRIVER, CC3000_TINY_SERVER, CC3000_STANDARD_BUFFER_SIZE, CC3000_SECURE and CC3000_NO_PATCH
** "utility/cc3000_common.h": comment out CC3000_MESSAGES_VERBOSE
* Custom Adafruit NeoPixel Library:
** "Adafruit_NeoPixel.h": define NEOPIXEL_TINY_DRIVER
