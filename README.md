The NeoPixel Server
===================

## Summary
This is the source code for an Arduino project for a very small and bare webserver, controlling
a strip or ring of WS2812 LEDs with an integrated driver. For information on the build, check out project page: http://www.helgames.com/projects/arduino-neopixel-server

## Hardware
* An Arduino-Compatible microcontroller with at least 21.5kB of flash available for user code and 2kB of RAM. Arduino Leonardo and Sparkfun Fio v3 are tested and working with all features enabled, leaving ~100B of free program memory when using stripped versions of the driver libraries. The actual memory and flash requirements depend on the number of the LED and the configured feature-set.
* A strip or ring of WS2812 LEDs (Adafruit NeoPixels, if you will).
* The CC3000 breakout board or shield from Adafruit.
* The MAX4466 breakout board from Adafruit, if you want to test the as of yet flimsy and broken spectrum analyzer.

## Known Issues
* The spectrum analyzer code only works on Sparkfun Fio v3 with the microphone connected to A4, as that was done in a trial and error fasion during an 8h drive.
* If too many requests are received in a short period of time, the server tends to crash because of memory limitations. The server will try to reboot and recover without changing the lighting, but may take some time to recover (or fail to reboot in some circumstances)

## Build Requirements
* Adafruit CC3000 Library: https://github.com/adafruit/Adafruit_CC3000_Library
* Adafruit NeoPixel Library: https://github.com/adafruit/Adafruit_NeoPixel
* Fast Hartley Transform library: http://wiki.openmusiclabs.com/wiki/ArduinoFHT

Using the default libraries may result in a binary, that does not fit onto your Arduino. You will
likely end up with a project, that is 31k or more in binary size. If you want to fit the project
onto a microcontroller with only 32k flash, use these libraries:
* Forked Adafruit CC3000 Library: https://github.com/helgames/Adafruit_CC3000_Library
  - In "utility/cc3000_common.h": define CC3000_TINY_DRIVER, CC3000_TINY_SERVER, CC3000_TINY_EXPERIMENTAL, CC3000_STANDARD_BUFFER_SIZE, CC3000_SECURE and CC3000_NO_PATCH. Then comment out CC3000_MESSAGES_VERBOSE and CC3000_DHCP_INFO
* Forked Adafruit NeoPixel Library: https://github.com/helgames/Adafruit_NeoPixel
  - In "Adafruit_NeoPixel.h": define NEOPIXEL_TINY_DRIVER

## License
Copyright (c) 2014, Paul Schulze

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
