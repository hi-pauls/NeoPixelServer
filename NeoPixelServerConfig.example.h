// CC3000 hardware config
#define CC3000_PIN_IRQ   3
#define CC3000_PIN_VBAT  5
#define CC3000_PIN_CS    10

// Wifi setup
#define WLAN_SECURITY  WLAN_SEC_WPA2
#define WLAN_SSID      "ssid"
#define WLAN_PASS      "pass"

// NeoPixel hardware setup
#define NEOPIXEL_PIN            6
#define NEOPIXEL_OFFSET         0
#define NEOPIXEL_COUNT          16

// NeoPixel config
//#define NEOPIXEL_BRIGHTNESS     192
//#define NEOPIXEL_MAX_BRIGHTNESS 255
//#define NEOPIXEL_USB_BRIGHTNESS 8
//#define NEOPIXEL_COLOR_DELAY    40
//#define NEOPIXEL_RAINBOW_DELAY  20
//#define NEOPIXEL_FLICKER_DELAY  100

// Spectrum config
#define SPECTRUM_IN_PIN A5

// Server config
//#define WEBSERVER_PORT  80
//#define WEBSITE_TYPE    WEBSITE_NORMAL

// Start with the light enabled
//#define START_ENABLED

// Serial settings
// NO_SERIAL will save about 600b of flash
//#define NO_SERIAL
//#define SERIAL_WAIT

// Disable some or all effects
// NO_EFFECTS will save about 5.2kB of flash
// NO_SPECTRUM will save about 2.7kB of flash
//#define NO_EFFECTS
//#define NO_FADE
//#define NO_RAINBOW
//#define NO_FLICKER
//#define NO_FIREWORKS
//#define NO_RUN
//#define NO_CYLON
//#define NO_SPECTRUM
